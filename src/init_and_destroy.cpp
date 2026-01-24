#include <vector>
#include <pthread.h>

// AINDA TEM QUE TRATAR OS IFS CASO CRIAR A THREADS (e etc) FALHAR

void initMutexes(std::vector<pthread_mutex_t>& mutexes){
    for (size_t i = 0; i < mutexes.size(); i++){
        pthread_mutex_init(&mutexes[i], NULL);
    }    
}

void destroyMutexes(std::vector<pthread_mutex_t>& mutexes){
    for (size_t i = 0; i < mutexes.size(); i++){
        pthread_mutex_destroy(&mutexes[i]);
    }    
}

void spawnThreads(std::vector<pthread_t>& threads, 
                  const char* tipoEntrega, 
                  void* (*function)(void*))
{
    for (size_t i = 0; i < threads.size(); i++){
        pthread_create(&threads[i], NULL, function, (void*) tipoEntrega);
    }
}

void joinThreads(std::vector<pthread_t>& threads){
    for (size_t i = 0; i < threads.size(); i++){
        pthread_join(threads[i], NULL);
    }
}