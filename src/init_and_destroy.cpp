#include <vector>
#include <pthread.h>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include "headers/ThreadArgs.h"

void initMutexes(std::vector<pthread_mutex_t>& mutexes){
    for (size_t i = 0; i < mutexes.size(); i++){
        int codigo_retorno = pthread_mutex_init(&mutexes[i], NULL);
        if (codigo_retorno != 0){
            std::cerr << "Erro ao inicializar mutex " << i << ": " << strerror(codigo_retorno) << std::endl;
            exit(EXIT_FAILURE);
        }
    }    
}

void destroyMutexes(std::vector<pthread_mutex_t>& mutexes){
    for (size_t i = 0; i < mutexes.size(); i++){
        int codigo_retorno = pthread_mutex_destroy(&mutexes[i]);
        if (codigo_retorno != 0){
            std::cerr << "Erro ao destruir mutex " << i << ": " << strerror(codigo_retorno) << std::endl;
        }
    }    
}

void spawnThreads(std::vector<pthread_t>& threads, const char* tipoEntregador, void* (*function)(void*)){
    for (size_t i = 0; i < threads.size(); i++){
        ThreadArgs* args = new ThreadArgs{tipoEntregador, (int) i + 1};

        int codigo_retorno = pthread_create(&threads.at(i), NULL, function, (void*) args);
        if (codigo_retorno != 0){
            std::cerr << "Erro ao criar thread " << i << ": " << strerror(codigo_retorno) << std::endl;
            delete args;
            for (size_t j = 0; j < i; j++){
                pthread_cancel(threads[j]);
            }
            exit(EXIT_FAILURE);
        }
    }
}

void joinThreads(std::vector<pthread_t>& threads){
    for (size_t i = 0; i < threads.size(); i++){
        int codigo_retorno = pthread_join(threads.at(i), NULL);
        if (codigo_retorno != 0){
            std::cerr << "Erro ao dar join na thread " << i << ": " << strerror(codigo_retorno) << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}