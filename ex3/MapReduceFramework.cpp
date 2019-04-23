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
	pthread_mutex_t mutex1, mutex2;
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
	pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
	IntermediateVec* inter_vec = new IntermediateVec();
	JobContext jc = {multiThreadLevel, threads, &js, &barrier, &client, &inputVec, inter_vec, &outputVec, mutex1, mutex2};

	for (int i = 0; i < multiThreadLevel; ++i) {
		t_con[i] = {i, &jc};
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
	IntermediatePair p = IntermediatePair(key, value);
	JobContext* jc = (JobContext*)context;
	IntermediateVec vec = *(jc->inter_vec);
	pthread_mutex_lock(&(jc->mutex1));
	auto it = vec.begin();
	vec.insert(it, p);
	pthread_mutex_unlock(&(jc->mutex1));
}

void emit3 (K3* key, V3* value, void* context){
	OutputPair p = OutputPair(key, value);
	JobContext* jc = (JobContext*)context;
	OutputVec vec = *(jc->output_vec);
	pthread_mutex_lock(&(jc->mutex2));
	auto it = vec.begin();
	vec.insert(it, p);
	pthread_mutex_unlock(&(jc->mutex2));
}
void getJobState(JobHandle job, JobState* state);
void closeJobHandle(JobHandle job);
	