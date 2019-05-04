#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <atomic>
#include <algorithm>
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
	std::vector<pthread_t> threads;
	ThreadContext** t_cons;
	JobState* state;
	Barrier* barrier;
	const MapReduceClient* client;
	const InputVec* input_vec;
	OutputVec* output_vec;
	std::vector<IntermediateVec>* inter_vecs;
	pthread_mutex_t *mutex1, *mutex2;
	sem_t *sema;
	std::atomic<int>* atomic_counter, *atomic_done;
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

	MSG("working - tid: " << tid)

	// map
	int input_size = jc->input_vec->size();
	while (*(jc->atomic_counter) <= input_size)
	{
		int i = *(jc->atomic_counter)++;
		InputPair ip = jc->input_vec->at(i);
		jc->client->map(ip.first, ip.second, arg);
		*(jc->atomic_done)++;
	}
	// sort
	std::sort(tc->inter_vec->begin(), tc->inter_vec->end());

	MSG("reached barrier - tid: " << tid)
	jc->barrier->barrier();
	jc->state->stage = REDUCE_STAGE;
	MSG("crossed barrier - tid: " << tid)

	if (tid == 0)
	{	
		*(jc->atomic_done) = 0;
		for (int i=0; i < tc->inter_vec->size(); i++)
		{
			IntermediateVec* temp = new IntermediateVec();
			CHECK_NULLPTR(temp, "temp vector")
			for (int j=0; j < jc->level; j++) {
				IntermediatePair ip = *(jc->t_cons[j]->inter_vec)->end();
				temp->push_back(ip);
				jc->t_cons[j]->inter_vec->pop_back();
			}
			jc->inter_vecs->push_back(*temp);
			sem_post(jc->sema);
			temp->clear();
		}
	}
	while(*jc->atomic_done < jc->inter_vecs->size())
	{
		sem_wait(jc->sema);
		pthread_mutex_lock(jc->mutex1);
		IntermediateVec* iv = &jc->inter_vecs->at(0);
		jc->inter_vecs->erase(jc->inter_vecs->begin());
		pthread_mutex_unlock(jc->mutex1);
		jc->client->reduce(iv, &tc);
		*(jc->atomic_done)++;
	}
}

JobHandle startMapReduceJob(const MapReduceClient& client,
							const InputVec& inputVec, OutputVec& outputVec,
							int multiThreadLevel)
{
	JobState js = {stage_t(0),0};
	std::vector<pthread_t> threads(multiThreadLevel);
	ThreadContext* t_cons[multiThreadLevel];
	Barrier* barrier = new Barrier(multiThreadLevel);
	CHECK_NULLPTR(barrier, "Barrier init")

	pthread_mutex_t mutex1, mutex2;
	if (pthread_mutex_init(&mutex1, NULL) || pthread_mutex_init(&mutex2, NULL))
	{
		ERR("mutex init")
	}

	sem_t sema;
	if (sem_init(&sema, 0, 0))
	{
		ERR("semaphore init")
	}

    std::atomic<int> atomic_counter(0);
    std::atomic<int> atomic_done(0);
	
	auto inter_vecs = new std::vector<IntermediateVec>();
	CHECK_NULLPTR(inter_vecs, "inter_vecs init")

	JobContext* jc = new JobContext({multiThreadLevel, threads, t_cons, &js,
									 barrier, &client, &inputVec, &outputVec, inter_vecs, &mutex1,
									 &mutex2, &sema, &atomic_counter});

	jc->state->stage = MAP_STAGE;
	for (int i = 0; i < multiThreadLevel; i++)
	{
		IntermediateVec* inter_vec = new IntermediateVec();
		CHECK_NULLPTR(inter_vec, "inter_vec init, tid=" << i)
		t_cons[i] = new ThreadContext({i, inter_vec, jc});
		CHECK_NULLPTR(t_cons[i], "ThreadContext init, tid=" << i)
		pthread_t new_thread;
		pthread_create(&new_thread, NULL, do_work, t_cons[i]);
		threads.push_back(new_thread);
	}
	return jc;
}

void waitForJob(JobHandle job)
{
	int level = ((JobContext*)job)->level;
	std::vector<pthread_t> threads = ((JobContext*)job)->threads;
	for (int i = 0; i < level; ++i) {
		pthread_join(threads.at(i), NULL);
	}
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
	pthread_mutex_lock(jc->mutex1);
	auto it = vec->begin();
	vec->insert(it, *p);
	pthread_mutex_unlock(jc->mutex1);
}

void getJobState(JobHandle job, JobState* state)
{	
	JobContext* jc = (JobContext*)job;
	JobState* js = jc->state;
	state->stage = js->stage;
	int total;
	if (js->stage == MAP_STAGE) {
		total = jc->input_vec->size();
	}
	else if (js->stage == REDUCE_STAGE) {
		total = jc->inter_vecs->size();
	}
	state->percentage = (*jc->atomic_done / total) * 100;
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
		delete t_cons[i]->inter_vec;
		delete t_cons[i];
	}

	delete jc->barrier;
	delete jc;
}
	