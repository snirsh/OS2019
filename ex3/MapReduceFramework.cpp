#include <pthread.h>
#include <cstdio>
#include "MapReduceClient.h"
#include "Barrier.h"

// DEFS
typedef void* JobHandle;
enum stage_t {UNDEFINED_STAGE=0, MAP_STAGE=1, REDUCE_STAGE=2};

struct JobState {
	stage_t stage;
	float percentage;
};
struct JobContext {
	int level;
	pthread_t* threads;
	JobState* state;
	Barrier* barrier;
	const MapReduceClient* client;
	const InputVec* input_vec;
	IntermediateVec* inter_vec;
	OutputVec* output_vec;
};
struct ThreadContext {
	int tid;
	JobContext* jc;
};

// FUNCS
void* do_work(void* arg)
{
	ThreadContext* tc = (ThreadContext*) arg;
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
	pthread_t threads[multiThreadLevel];
	ThreadContext t_con[multiThreadLevel];
	Barrier barrier(multiThreadLevel);
	JobState js = {stage_t(0),0};
	IntermediateVec* inter_vec = new IntermediateVec();
	JobContext jc = {multiThreadLevel, threads, &js, &barrier, &client, &inputVec, inter_vec, &outputVec};

	for (int i = 0; i < multiThreadLevel; ++i) {
		t_con[i] = {i, &jc};
	}
	for (int i = 0; i < multiThreadLevel; ++i) {
		pthread_create(&threads[i], NULL, do_work, &t_con[i]);
	}
	return &jc;
}

void waitForJob(JobHandle job) {
	int level = ((JobContext*)job)->level;
	pthread_t* threads = ((JobContext*)job)->threads;
	for (int i = 0; i < level; ++i) {
		pthread_join(threads[i], NULL);
	}
}
void emit2 (K2* key, V2* value, void* context)
{
	IntermediatePair p = new IntermediatePair(key, value);
	pthread_mutex_lock(*context.mutex1);
	context.inter_vec.insert(p);
	pthread_mutex_unlock(*context.mutex1);
}

void emit3 (K3* key, V3* value, void* context){
	IntermediatePair p = new IntermediatePair(key, value);
	pthread_mutex_lock(*context.mutex2);
	context.output_vec.insert(p);
	pthread_mutex_unlock(*context.mutex2);
}
void getJobState(JobHandle job, JobState* state);
void closeJobHandle(JobHandle job);
	