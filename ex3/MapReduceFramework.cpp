#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <atomic>
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
	std::vector<IntermediateVec>* inter_vecs;
	pthread_mutex_t *mutex1, *mutex2;
	sem_t *sema;
	std::atomic<double>* atomic_state;
	std::atomic<int>* atomic_counter;
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
	}
	// sort
	std::sort(tc->inter_vec->begin(), tc->inter_vec->end());

	MSG("reached barrier - tid: " << tid)
	jc->barrier->barrier();
	MSG("crossed barrier - tid: " << tid)

	if (tid == 0)
	{
		// shuffle
	} else {
		// reduce
	}
	return 0;
}

JobHandle startMapReduceJob(const MapReduceClient& client,
							const InputVec& inputVec, OutputVec& outputVec,
							int multiThreadLevel)
{
	JobState js = {stage_t(0),0};
	pthread_t* threads[multiThreadLevel];
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
    std::atomic<double> atomic_state(0);
	unsigned int* temp = (unsigned int*) &atomic_state;
	*temp = inputVec.size() * 2;
	auto inter_vecs = new std::vector<IntermediateVec>();
	CHECK_NULLPTR(inter_vecs, "inter_vecs init")

	JobContext* jc = new JobContext({multiThreadLevel, threads, t_cons, &js,
									 barrier, &client, &inputVec, &outputVec, inter_vecs, &mutex1,
									 &mutex2, &sema, &atomic_state, &atomic_counter});

	for (int i = 0; i < multiThreadLevel; i++)
	{
		IntermediateVec* inter_vec = new IntermediateVec();
		CHECK_NULLPTR(inter_vec, "inter_vec init, tid=" << i)
		t_cons[i] = new ThreadContext({i, inter_vec, jc});
		CHECK_NULLPTR(t_cons[i], "ThreadContext init, tid=" << i)
		pthread_create(threads[i], NULL, do_work, t_cons[i]);
	}
	return jc;
}

void waitForJob(JobHandle job)
{
	int level = ((JobContext*)job)->level;
	pthread_t** threads = ((JobContext*)job)->threads;
	for (int i = 0; i < level; ++i) {
		pthread_join(*threads[i], NULL);
	}
}

void emit2 (K2* key, V2* value, void* context)
{
	ThreadContext* tc = (ThreadContext*)context;
	IntermediatePair p = IntermediatePair(key, value);
	IntermediateVec* vec = tc->inter_vec;
	auto it = vec->begin();
	vec->insert(it, p);
}

void emit3 (K3* key, V3* value, void* context)
{	
	ThreadContext* tc = (ThreadContext*)context;
	OutputPair p = OutputPair(key, value);
	JobContext* jc = tc->jc;
	OutputVec* vec = jc->output_vec;
	pthread_mutex_lock(jc->mutex2);
	auto it = vec->begin();
	vec->insert(it, p);
	pthread_mutex_unlock(jc->mutex2);
}

void getJobState(JobHandle job, JobState* state)
{	
	JobContext* jc = (JobContext*)job;
	unsigned int* temp = (unsigned int*) jc->atomic_state;
	unsigned int total = *temp;
	temp++;
	unsigned int done = *temp;

	if (done == 0) {
		jc->state->stage = UNDEFINED_STAGE;
	} else if (done <= total / 2) { 
			jc->state->stage = MAP_STAGE;
	} else if (done > total / 2) { 
			jc->state->stage = REDUCE_STAGE;
	}

	if (done == 0) {
		jc->state->percentage = 0;
	} else{
		jc->state->percentage = done / (total / 2);
	}

	state->stage = jc->state->stage;
	state->percentage = jc->state->percentage;
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

	delete jc->barrier;
}
	