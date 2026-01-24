#include <iostream>
#include <pthread.h>    // variáveis e funções daqui possuem prefixo 'pthread_'
#include <unistd.h>     // getpid(), gettid(), sleep()
#include <vector>
#include <cstring>
#include "headers/init_and_destroy.h"

#define NUM_REST 6
std::vector<pthread_mutex_t> pedidos(NUM_REST);
std::vector<pthread_mutex_t> motos(NUM_REST);

#define NUM_ENTREG 4

void* fazerEntrega(void* tipoEntrega){
    float lucroDiario = 0;
    while(true){
        if(strcmp((char*) tipoEntrega, "Veterano") == 0){
            int numRestaurante = (rand() % NUM_REST);
            pthread_mutex_lock(&motos[numRestaurante]);
            std::cout << "[Veterano nº  " << gettid() << "]: peguei a moto do restaurante " << numRestaurante << std::endl;
            sleep(2); // caminhando pro balcão pegar o pedido
            pthread_mutex_lock(&pedidos[numRestaurante]);
            std::cout << "[Veterano nº  " << gettid() << "]: peguei o pedido do restaurante " << numRestaurante << std::endl;
                    lucroDiario += (std::rand() % 4001 + 1000) / 100.0f; // conseguiu fazer a entrega e ganhou algo entre 10,00 e 50,00
                    std::cout << "[Veterano nº  " << gettid() << "]: realizei a entrega do restaurante " << numRestaurante << std::endl;
            pthread_mutex_unlock(&motos[numRestaurante]);
            pthread_mutex_unlock(&pedidos[numRestaurante]);
        }
        else if(strcmp((char*) tipoEntrega, "Novato") == 0){
            int numRestaurante = (rand() % NUM_REST);
            pthread_mutex_lock(&pedidos[numRestaurante]);
            std::cout << "[Novato nº  " << gettid() << "]: peguei o pedido do restaurante " << numRestaurante << std::endl;
            sleep(2); // caminhando pro estacionamento pegar a moto
            pthread_mutex_lock(&motos[numRestaurante]);
            std::cout << "[Novato nº  " << gettid() << "]: peguei a moto do restaurante " << numRestaurante << std::endl;
                    lucroDiario += (std::rand() % 4001 + 1000) / 100.0f; // conseguiu fazer a entrega e ganhou algo entre 10,00 e 50,00
                    std::cout << "[Novato nº  " << gettid() << "]: realizei a entrega do restaurante " << numRestaurante << std::endl;
            pthread_mutex_unlock(&motos[numRestaurante]);
            pthread_mutex_unlock(&pedidos[numRestaurante]);
        }
    }
    pthread_exit(NULL);
}

int main(void){

    srand(time(0));

    std::vector<pthread_t> entregNov(NUM_ENTREG);
    std::vector<pthread_t> entregVet(NUM_ENTREG);

    initMutexes(pedidos);
    initMutexes(motos);

    spawnThreads(entregNov, "Novato", &fazerEntrega);
    spawnThreads(entregVet, "Veterano", &fazerEntrega);

    // --- --- --- * --- --- --- * --- --- ---

    joinThreads(entregNov);
    joinThreads(entregVet);

    destroyMutexes(pedidos);
    destroyMutexes(motos);
    
    std::cout << "cheguei no final" << std::endl;

    return 0;
}