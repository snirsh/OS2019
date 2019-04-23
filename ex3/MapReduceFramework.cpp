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
	JobState state;
	Barrier* barrier;
	const MapReduceClient* client;
	const InputVec* input_vec;
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
	ThreadContext contexts[multiThreadLevel];
	Barrier barrier(multiThreadLevel);

	for (int i = 0; i < multiThreadLevel; ++i) {
		contexts[i] = {i, &barrier, &client, &inputVec, &outputVec};
	}
	for (int i = 0; i < multiThreadLevel; ++i) {
		pthread_create(&threads[i], NULL, do_work, &contexts[i]);
	}
	for (int i = 0; i < multiThreadLevel; ++i) {
		pthread_join(threads[i], NULL);
	}
	return 0;
}

void emit2 (K2* key, V2* value, void* context);
void emit3 (K3* key, V3* value, void* context);
void waitForJob(JobHandle job);
void getJobState(JobHandle job, JobState* state);
void closeJobHandle(JobHandle job);
	