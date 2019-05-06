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
	sem_t *sema, *sema_lock_shuffle, *sema_lock_inter_vec;
	std::atomic<unsigned int>* atomic_done;
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
    auto * tc = (ThreadContext*)arg;
	JobContext* jc = tc->jc;
	int tid = tc->tid;

	MSG("working - tid: " << tid)

	// map
	unsigned long input_size = jc->input_vec->size();
	unsigned int temp = jc->atomic_done->load();
	while (temp < input_size)
	{
		temp = (*(jc->atomic_done))++;
		InputPair ip = jc->input_vec->at(temp);
		jc->client->map(ip.first, ip.second, arg);
		MSG("tid "<<tid<< " mapping i="<<temp)
		temp = jc->atomic_done->load();
	}
	// sort
	std::sort(tc->inter_vec->begin(), tc->inter_vec->end());

	MSG("reached barrier - tid: " << tid)
	jc->barrier->barrier();
	jc->state->stage = REDUCE_STAGE;
	MSG("crossed barrier - tid: " << tid)

	//shuffle
	if (tid == 0)
	{	
		(*(jc->atomic_done)) = 0;
		/**
		 *  added a semaphore called sema_lock_shuffle,
		 *  this semaphore will lock the do_work until thread0 finished shuffling
		 */
		for(int i=0; i < jc->level; i++){
            sem_post(jc->sema_lock_shuffle);
        }
		K2* max_key;
		K2* temp_key;
		IntermediatePair* ip;
		IntermediateVec* temp_inter_vec;
		int total_size;

		#define KEY_FROM_BACK(i) jc->t_cons[i]->inter_vec->back().first
		#define INTER_VEC_SIZE(i) jc->t_cons[i]->inter_vec->size()

		while (true)
		{
			// find max key from back of all inter_vecs
			max_key = KEY_FROM_BACK(0);
			for (int i=1; i < jc->level; i++) {
				if (jc->t_cons[i]->inter_vec->empty()) {
					continue;
				}
				if (KEY_FROM_BACK(i) > max_key) {
					max_key = KEY_FROM_BACK(i);
				}
			}

			// take pairs with key value of max_key from all inter_vecs
			// and put inside new vector
            temp_inter_vec = new IntermediateVec();
			CHECK_NULLPTR(temp_inter_vec, "temp vector")
			for (int i=0; i < jc->level; i++)
			{
				if (jc->t_cons[i]->inter_vec->empty()) {
					continue;
				}
				temp_key = KEY_FROM_BACK(i);
				while (!(temp_key < max_key || temp_key > max_key))
				{
					ip = &(jc->t_cons[i]->inter_vec->back());
                    temp_inter_vec->push_back(*ip);
					jc->t_cons[i]->inter_vec->pop_back();
					if (jc->t_cons[i]->inter_vec->empty()) {
						break;
					}
					temp_key = KEY_FROM_BACK(i);
					MSG("[size] " << INTER_VEC_SIZE(i) << " [tid] "<< i)
				}
			}
			// add new vector to the batch to be processed by other threads
			jc->inter_vecs->push_back(*temp_inter_vec);
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
    }
	// reduce
	/**
	 * added a semaphore that is jc->level (#threads) size
	 * should get to 0 only when shuffle is done.
	 * each thread is decrementing it here
	 */
	sem_wait(jc->sema_lock_shuffle);
	while(jc->atomic_done->load() < jc->inter_vecs->size())
	{
		sem_wait(jc->sema);
//		pthread_mutex_lock(jc->mutex1);
        /**
         * added a semaphore named sema_lock_inter_vec
         * should replace mutex1 which is used here and also in reduce (via emit3)
         */
        sem_post(jc->sema_lock_inter_vec);
		IntermediateVec* iv = &jc->inter_vecs->at(0);
        jc->inter_vecs->erase(jc->inter_vecs->begin());
        sem_wait(jc->sema_lock_inter_vec);
//		pthread_mutex_unlock(jc->mutex1);
		jc->client->reduce(iv, &tc);
		(*(jc->atomic_done))++;
    }
    return nullptr;
}

JobHandle startMapReduceJob(const MapReduceClient& client,
							const InputVec& inputVec, OutputVec& outputVec,
							int multiThreadLevel)
{
	JobState* js = new JobState({stage_t(0),0});
	std::vector<pthread_t> threads(static_cast<unsigned long>(multiThreadLevel));
    auto ** t_cons = new ThreadContext*[multiThreadLevel];
    auto * barrier = new Barrier(multiThreadLevel);
	CHECK_NULLPTR(barrier, "Barrier init")

	pthread_mutex_t mutex1, mutex2;
	if (pthread_mutex_init(&mutex1, nullptr) || pthread_mutex_init(&mutex2, nullptr))
	{
		ERR("mutex init")
	}

	sem_t sema, sema_lock_shuffle, sema_lock_inter_vec;
	if (sem_init(&sema, 0, 0) || sem_init(&sema_lock_shuffle, 0 ,0) || sem_init(&sema_lock_inter_vec, 0 ,0))
	{
		ERR("semaphore init")
	}

    auto * atomic_done = new std::atomic<unsigned int>;
	
	auto inter_vecs = new std::vector<IntermediateVec>();
	CHECK_NULLPTR(inter_vecs, "inter_vecs init")

	JobContext* jc = new JobContext({multiThreadLevel, threads, t_cons, js,
									 barrier, &client, &inputVec, &outputVec, inter_vecs, &mutex1,
									 &mutex2, &sema, &sema_lock_shuffle, &sema_lock_inter_vec, atomic_done});

	jc->state->stage = MAP_STAGE;
	for (int i = 0; i < multiThreadLevel; i++)
	{
        auto * inter_vec = new IntermediateVec();
		CHECK_NULLPTR(inter_vec, "inter_vec init, tid=" << i)
		t_cons[i] = new ThreadContext({i, inter_vec, jc});
		CHECK_NULLPTR(t_cons[i], "ThreadContext init, tid=" << i)
		pthread_t new_thread;
		pthread_create(&new_thread, nullptr, do_work, t_cons[i]);
		threads.push_back(new_thread);
	}
	return jc;
}

void waitForJob(JobHandle job)
{
	int level = ((JobContext*)job)->level;
	std::vector<pthread_t> threads = ((JobContext*)job)->threads;
	for (int i = 0; i < level; ++i) {
		pthread_join(threads.at(static_cast<unsigned long>(i)), nullptr);
	}
}

void emit2 (K2* key, V2* value, void* context)
{
    auto * tc = (ThreadContext*)context;
    auto * p = new IntermediatePair(key, value);
	IntermediateVec* vec = tc->inter_vec;
	auto it = vec->begin();
	vec->insert(it, *p);
}

void emit3 (K3* key, V3* value, void* context)
{
    auto * tc = (ThreadContext*)context;
    auto * p = new OutputPair(key, value);
	JobContext* jc = tc->jc;
	OutputVec* vec = jc->output_vec;
	pthread_mutex_lock(jc->mutex1);
	auto it = vec->begin();
	vec->insert(it, *p);
	pthread_mutex_unlock(jc->mutex1);
}

void getJobState(JobHandle job, JobState* state)
{
    auto * jc = (JobContext*)job;
	JobState* js = jc->state;
	state->stage = js->stage;
	int total = 0;
	if (js->stage == MAP_STAGE) {
		total = static_cast<int>(jc->input_vec->size());
	}
	else if (js->stage == REDUCE_STAGE) {
		total = static_cast<int>(jc->inter_vecs->size());
	}
	state->percentage = (jc->atomic_done->load() / (float)total) * 100;
	js->percentage = state->percentage;
}

void closeJobHandle(JobHandle job)
{
    auto * jc = (JobContext*)job;
	if (pthread_mutex_destroy(jc->mutex1) || pthread_mutex_destroy(jc->mutex2)) {
		ERR("mutex destroy")
	}
	if (sem_destroy(jc->sema) || sem_destroy(jc->sema_lock_shuffle)) {
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
	delete jc->state;
	delete jc;
}
	