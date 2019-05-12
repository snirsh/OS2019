#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <atomic>
#include <algorithm>
#include <unistd.h>
#include "MapReduceClient.h"
#include "Barrier.h"

// DEFS
#define ERR(msg) std::cerr << "error: " << msg << std::endl; exit(1);
#define MSG(msg) std::cout << msg << std::endl;
#define CHECK_NULLPTR(ptr, msg) if (ptr == nullptr) {ERR(msg)}

typedef void* JobHandle;
enum stage_t {UNDEFINED_STAGE=0, MAP_STAGE=1, REDUCE_STAGE=2};

// STRUCTS
struct JobState
{
	stage_t stage;
	float percentage;
};
struct ThreadContext;
struct JobContext
{
	int level;
	pthread_t** threads;
	ThreadContext** t_cons;
	JobState* state;
	Barrier* barrier;
	const MapReduceClient* client;
	const InputVec* input_vec;
	OutputVec* output_vec;
	std::vector<IntermediateVec*>* inter_vecs;
	pthread_mutex_t *mutex1, *mutex2;
	sem_t *sema;
	std::atomic<unsigned int>* atomic_done;
	bool joined, shuffled;
	int total;
};
struct ThreadContext
{
	int tid;
	IntermediateVec* inter_vec;
	JobContext* jc;
};

// FUNCS
void* do_work(void* arg)
{
	ThreadContext* tc = (ThreadContext*)arg;
	JobContext* jc = tc->jc;
	int tid = tc->tid;

	// map
	unsigned int input_size = jc->input_vec->size();
	unsigned int cur_index = jc->atomic_done->load();
	InputPair ip = InputPair();
	while (cur_index < input_size)
	{
		cur_index = (*(jc->atomic_done))++;
		ip = jc->input_vec->at(cur_index);
		jc->client->map(ip.first, ip.second, arg);
		cur_index = jc->atomic_done->load();
	}
	// sort
	std::sort(tc->inter_vec->begin(), tc->inter_vec->end());

	jc->barrier->barrier();
	jc->state->stage = REDUCE_STAGE;

	//shuffle
	if (tid == 0)
	{	
		(*(jc->atomic_done)) = 0;

		K2* max_key;
		K2* temp_key;
		IntermediatePair* ip;
		IntermediateVec* temp_iv;
		int total_size;

		#define KEY_FROM_BACK(i) jc->t_cons[i]->inter_vec->back().first
		#define INTER_VEC_SIZE(i) jc->t_cons[i]->inter_vec->size()
		#define INTER_VEC_EMPTY(i) jc->t_cons[i]->inter_vec->empty()

		while (true)
		{
			// find max key from back of all inter_vecs
			max_key = KEY_FROM_BACK(0);
			for (int i=1; i < jc->level; i++) {
				if (INTER_VEC_EMPTY(i)) {
					continue;
				}
				if (*max_key < *KEY_FROM_BACK(i)) {
					max_key = KEY_FROM_BACK(i);
				}
			}

			// take pairs with key value of max_key from all inter_vecs
			// and put inside new vector
			temp_iv = new IntermediateVec();
			CHECK_NULLPTR(temp_iv, "temp vector")
			for (int i=0; i < jc->level; i++)
			{
				if (INTER_VEC_EMPTY(i)) {
					continue;
				}
				temp_key = KEY_FROM_BACK(i);
				while (!((*temp_key < *max_key) || (*max_key < *temp_key)))
				{
					ip = &(jc->t_cons[i]->inter_vec->back());
					temp_iv->push_back(*ip);
					jc->t_cons[i]->inter_vec->pop_back();
					if (INTER_VEC_EMPTY(i)) {
						break;
					}
					temp_key = KEY_FROM_BACK(i);
				}
			}
			// add new vector to the batch to be processed by other threads
			jc->total++;
			jc->inter_vecs->push_back(temp_iv);
			sem_post(jc->sema);

			// check if all inter_vecs are empty
			total_size = 0;
			for (int i=0; i < jc->level; i++) {
				total_size += INTER_VEC_SIZE(i);
			}
			if (total_size == 0) {
				break;
			}
		}
		jc->shuffled = true;
	}

	// reduce
	while (true)
	{
		if (jc->shuffled && jc->inter_vecs->empty()) {
			break;
		}
		sem_wait(jc->sema);
		pthread_mutex_lock(jc->mutex1);
		IntermediateVec* iv = jc->inter_vecs->back();
		jc->inter_vecs->pop_back();
		pthread_mutex_unlock(jc->mutex1);
		jc->client->reduce(iv, tc);
		(*(jc->atomic_done))++;
	}
	return nullptr;
}

JobHandle startMapReduceJob(const MapReduceClient& client,
							const InputVec& inputVec, OutputVec& outputVec,
							int multiThreadLevel)
{
	JobState* js = new JobState({stage_t(0),0});
	pthread_t** threads = new pthread_t*[multiThreadLevel];
	ThreadContext** t_cons = new ThreadContext*[multiThreadLevel];
	Barrier* barrier = new Barrier(multiThreadLevel);
	CHECK_NULLPTR(js, "JobState init")
	CHECK_NULLPTR(threads, "threads init")
	CHECK_NULLPTR(t_cons, "t_cons init")
	CHECK_NULLPTR(barrier, "Barrier init")

	pthread_mutex_t* mutex1 = new pthread_mutex_t();
	pthread_mutex_t* mutex2 = new pthread_mutex_t();
	CHECK_NULLPTR(mutex1, "mutex1 init")
	CHECK_NULLPTR(mutex2, "mutex2 init")
	if (pthread_mutex_init(mutex1, NULL) || pthread_mutex_init(mutex2, NULL))
	{
		ERR("mutex init")
	}

	sem_t* sema = new sem_t();
	CHECK_NULLPTR(sema, "sema init")
	if (sem_init(sema, 0, 0))
	{
		ERR("sem_init")
	}

    std::atomic<unsigned int>* atomic_done = new std::atomic<unsigned int>;
	CHECK_NULLPTR(atomic_done, "atomic_done init")
	
	auto inter_vecs = new std::vector<IntermediateVec*>();
	CHECK_NULLPTR(inter_vecs, "inter_vecs init")

	JobContext* jc = new JobContext({multiThreadLevel, threads, t_cons, js,
									 barrier, &client, &inputVec, &outputVec, inter_vecs, mutex1,
									 mutex2, sema, atomic_done, false, false, 0});

	jc->state->stage = MAP_STAGE;
	for (int i = 0; i < multiThreadLevel; i++)
	{
		IntermediateVec* inter_vec = new IntermediateVec();
		CHECK_NULLPTR(inter_vec, "inter_vec init, tid=" << i)
		t_cons[i] = new ThreadContext({i, inter_vec, jc});
		CHECK_NULLPTR(t_cons[i], "ThreadContext init, tid=" << i)
		pthread_t* new_thread = new pthread_t();
		pthread_create(new_thread, NULL, do_work, t_cons[i]);
		threads[i] = new_thread;
	}
	return jc;
}

void waitForJob(JobHandle job)
{
	if (((JobContext*)job)->joined) {
		return;
	}
	int level = ((JobContext*)job)->level;
	for (int i = 0; i < level; ++i) {
		pthread_join(*((JobContext*)job)->threads[i], NULL);
	}
	((JobContext*)job)->joined = true;
}

void emit2 (K2* key, V2* value, void* context)
{
	ThreadContext* tc = (ThreadContext*)context;
	IntermediatePair* p = new IntermediatePair(key, value);
	IntermediateVec* vec = tc->inter_vec;
	auto it = vec->begin();
	vec->insert(it, *p);
}

void emit3 (K3* key, V3* value, void* context)
{	
	ThreadContext* tc = (ThreadContext*)context;
	OutputPair* p = new OutputPair(key, value);
	JobContext* jc = tc->jc;
	OutputVec* vec = jc->output_vec;
	pthread_mutex_lock(jc->mutex2);
	auto it = vec->begin();
	vec->insert(it, *p);
	pthread_mutex_unlock(jc->mutex2);
}

void getJobState(JobHandle job, JobState* state)
{	
	JobContext* jc = (JobContext*)job;
	JobState* js = jc->state;
	state->stage = js->stage;
	int total = 1;
	if (js->stage == MAP_STAGE) {
		total = jc->input_vec->size();
	}
	else if (js->stage == REDUCE_STAGE) {
		total = jc->total;
	}
	state->percentage = (jc->atomic_done->load() / (float)total) * 100;
	js->percentage = state->percentage;
}

void closeJobHandle(JobHandle job)
{
	JobContext* jc = (JobContext*)job;
	if (pthread_mutex_destroy(jc->mutex1) || pthread_mutex_destroy(jc->mutex2)) {
		ERR("mutex destroy")
	}
	if (sem_destroy(jc->sema)) {
		ERR("semaphore destroy")
	}

	ThreadContext** t_cons = jc->t_cons;
	for (int i = 0; i < jc->level; ++i) {
		for (IntermediatePair it : *(t_cons[i]->inter_vec)) {
			delete &it;
		}
		delete jc->threads[i];
		delete t_cons[i]->inter_vec;
		delete t_cons[i];
	}

	delete jc->barrier;
	delete jc->threads;
	delete jc->state;
	delete jc->mutex1;
	delete jc->mutex2;
	delete jc->sema;
	delete jc;
}
	