#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <cstdlib>
#include <atomic>
#include "MapReduceClient.h"
#include "Barrier.h"

// DEFS
#define ERR(msg) std::cerr << "error: " << msg << std::endl; exit(1);
typedef void* JobHandle;
enum stage_t {UNDEFINED_STAGE=0, MAP_STAGE=1, REDUCE_STAGE=2};

struct JobState
{
	stage_t stage;
	float percentage;
};
struct JobContext
{
	int level;
	pthread_t* threads;
	JobState* state;
	Barrier* barrier;
	const MapReduceClient* client;
	const InputVec* input_vec;
	IntermediateVec* inter_vec;
	OutputVec* output_vec;
	pthread_mutex_t *mutex1, *mutex2;
	sem_t *sema;
	std::atomic<double>* atomic_state;
};
struct ThreadContext
{
	int tid;
	JobContext* jc;
};

// FUNCS
void* do_work(void* arg)
{
	ThreadContext* tc = (ThreadContext*)arg;
	int tid = tc->tid;
	JobContext* jc = tc->jc;

	// map
	// sort

	jc->barrier->barrier();

	if (tid == 0) {
		// shuffle
	}
	else {
		// reduce
	}
	return 0;
}

JobHandle startMapReduceJob(const MapReduceClient& client,
							const InputVec& inputVec, OutputVec& outputVec,
							int multiThreadLevel)
{
	JobState js = {stage_t(0),0};
	pthread_t threads[multiThreadLevel];
	ThreadContext t_con[multiThreadLevel];
	Barrier barrier(multiThreadLevel);

	pthread_mutex_t mutex1, mutex2;
	if (pthread_mutex_init(&mutex1, NULL) || pthread_mutex_init(&mutex2, NULL)) {
		ERR("mutex init")
	}

	IntermediateVec* inter_vec = new IntermediateVec();
	if (inter_vec == nullptr) {
		ERR("inter_vec init")
	}

	sem_t sema;
	if (sem_init(&sema, 0, 0)) {
		ERR("semaphore init")
	}

    std::atomic<double> atomic_state(0);
	int* temp = (int*) &atomic_state;
	*temp = inputVec.size();

	JobContext jc = {multiThreadLevel, threads, &js, &barrier, &client,
					 &inputVec, inter_vec, &outputVec, &mutex1, &mutex2, &sema, &atomic_state};

	for (int i = 0; i < multiThreadLevel; ++i) {
		t_con[i] = {i, &jc};
		pthread_create(&threads[i], NULL, do_work, &t_con[i]);
	}
	return &jc;
}

void waitForJob(JobHandle job)
{
	int level = ((JobContext*)job)->level;
	pthread_t* threads = ((JobContext*)job)->threads;
	for (int i = 0; i < level; ++i) {
		pthread_join(threads[i], NULL);
	}
}

void emit2 (K2* key, V2* value, void* context)
{
	IntermediatePair p = IntermediatePair(key, value);
	JobContext* jc = (JobContext*)context;
	IntermediateVec vec = *(jc->inter_vec);
	pthread_mutex_lock(jc->mutex1);
	auto it = vec.begin();
	vec.insert(it, p);
	pthread_mutex_unlock(jc->mutex1);
}

void emit3 (K3* key, V3* value, void* context)
{
	OutputPair p = OutputPair(key, value);
	JobContext* jc = (JobContext*)context;
	OutputVec vec = *(jc->output_vec);
	pthread_mutex_lock(jc->mutex2);
	auto it = vec.begin();
	vec.insert(it, p);
	pthread_mutex_unlock(jc->mutex2);
}

void getJobState(JobHandle job, JobState* state)
{	
	JobContext* jc = (JobContext*)job;
	int* temp = (int*) jc->atomic_state;
	int total = *temp;
	temp++;
	int done = *temp;
	if (total >= 0) {
		if (done >= 0) {
			jc->state->stage = UNDEFINED_STAGE;
		} else {
			jc->state->stage = MAP_STAGE;
		}
	} else {
			jc->state->stage = REDUCE_STAGE;
		}
	jc->state->percentage = abs(total) / abs(done);
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
	jc->barrier->~Barrier();

	for (auto it = jc->inter_vec->begin(); it != jc->inter_vec->end(); ++it) {
		it->first->~K2();
		it->second->~V2();
	}
	jc->inter_vec->clear();
	jc->inter_vec->~vector();
}
	