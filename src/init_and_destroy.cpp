#include <vector>
#include <pthread.h>
#include "headers/ThreadArgs.h"

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
                  const char* tipoEntregador, 
                  void* (*function)(void*)){
    for (size_t i = 0; i < threads.size(); i++){
        ThreadArgs* args = new ThreadArgs{tipoEntregador, (int) i + 1};
        pthread_create(&threads.at(i), NULL, function, (void*) args);
    }
}

void joinThreads(std::vector<pthread_t>& threads){
    for (size_t i = 0; i < threads.size(); i++){
        pthread_join(threads.at(i), NULL);
    }
}