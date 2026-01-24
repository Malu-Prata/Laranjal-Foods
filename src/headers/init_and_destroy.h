#include <vector>
#include <pthread.h>

void initMutexes(std::vector<pthread_mutex_t>& mutexes);
void destroyMutexes(std::vector<pthread_mutex_t>& mutexes);
void spawnThreads(std::vector<pthread_t>& threads, const char* tipoEntrega, void* (*function)(void*));
void joinThreads(std::vector<pthread_t>& threads);