#include <string>
#include <pthread.h>
#include <iostream>

void log(std::string tipoLog, 
         pthread_mutex_t& coutMutex, 
         std::string spaces = "", 
         std::string cor = "", 
         std::string tipoEntregador = "", 
         int threadID = -1, 
         std::string recurso = "", 
         int numRestaurante = -1, 
         std::string resetCor = "",
         float lucroAtual = 0.0f)
{
    pthread_mutex_lock(&coutMutex);
    if (tipoLog.compare("pegou recurso") == 0){
            std::cout << spaces << cor << "[" << tipoEntregador << " nº " << threadID << "]:\n" 
                      << spaces << "pegou " << recurso << " do res. " << numRestaurante << "\n\n" << resetCor;
    }
    else if(tipoLog.compare("desistiu") == 0){
            std::cout << spaces << cor << "[" << tipoEntregador << " nº " << threadID << "]:\n" 
                      << spaces << "desistiu d" << recurso << " do res. " << numRestaurante << "\n\n" << resetCor;
    }
    else if(tipoLog.compare("fez a entrega") == 0){
            std::cout << spaces << cor << "[" << tipoEntregador << " nº " << threadID << "]:\n" 
                      << spaces << "fez a entrega do res. " << numRestaurante << "\n\n" << resetCor;
    }
    else if(tipoLog.compare("possui R$") == 0){
            std::cout << spaces << cor << "[" << tipoEntregador << " nº " << threadID << "]:\n" 
                      << spaces << "possui R$ " << lucroAtual << "\n\n" << resetCor;
    }
    else{
        std::cerr << "Você passou um tipo de log errado!" << std::endl;
        exit(99);
    }
    pthread_mutex_unlock(&coutMutex);
}