#include <iostream>
#include <pthread.h>    // variáveis e funções daqui possuem prefixo 'pthread_'
#include <unistd.h>     // getpid(), gettid(), sleep()
#include <vector>
#include <cstring>
#include <ctime>
#include "headers/init_and_destroy.h"
#include "headers/ThreadArgs.h"
#include "headers/build_strings.h"
#include "headers/Cores.h"
#include "headers/log.h"

#define NUM_REST 5

std::vector<pthread_mutex_t> pedidos(NUM_REST);
std::vector<pthread_mutex_t> motos(NUM_REST);

std::vector<std::string> tabsRestaurante;

#define NUM_ENTREG 3

pthread_mutex_t coutMutex = PTHREAD_MUTEX_INITIALIZER;

#define SLEEP_TIME 2

// ========= * PREVENÇÃO DE DEADLOCK COM RESERVA DE RECURSOS * =========

#define MAX_TENTATIVAS_ANTES_AGING 5

pthread_mutex_t controleMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t recursosMutex = PTHREAD_MUTEX_INITIALIZER;

std::vector<float> lucrosGlobais;
std::vector<int>   prioridades;
std::vector<int>   tentativasRecurso;

std::vector<int>   donoDoPedido;
std::vector<int>   donoDaMoto;

// pra evitar "roubo" de recursos causados por race conditions
std::vector<int>   reservaPedido;  // -1 = livre, ID = reservado para esse entregador
std::vector<int>   reservaMoto;    // -1 = livre, ID = reservado para esse entregador

int registrarThread(){
    pthread_mutex_lock(&controleMutex);

    int id = lucrosGlobais.size();
    lucrosGlobais.push_back(0.0f);
    prioridades.push_back(0);
    tentativasRecurso.push_back(0);

    pthread_mutex_unlock(&controleMutex);
    return id;
}

// --- --- --- * --- --- --- * --- --- --- * --- --- --- * --- --- --- * --- --- --- * --- --- --- * --- --- --- * --- --- --- 

void* fazerEntrega(void* args){
    ThreadArgs* localArgs = (ThreadArgs*) args;
    int threadID = localArgs->threadID;
    const char* tipoEntregador = localArgs->tipoEntregador;

    int idLocal = registrarThread();

    unsigned int seedLocal = time(NULL) ^ gettid();
    const char* cor = (strcmp(tipoEntregador, "Veterano") == 0) ? ROXO : AMARELO;

    while(true){
        int numRestaurante = rand_r(&seedLocal) % NUM_REST;
        std::string spaces = tabsRestaurante[numRestaurante];

        bool isVeterano = strcmp(tipoEntregador, "Veterano") == 0;
        const char* primeiroRecurso = (isVeterano) ? "a moto" : "o pedido";
        const char* segundoRecurso  = (isVeterano) ? "o pedido" : "a moto";
        pthread_mutex_t* primeiraAcao = (isVeterano) ? &motos[numRestaurante] : &pedidos[numRestaurante];
        pthread_mutex_t* segundaAcao  = (isVeterano) ? &pedidos[numRestaurante] : &motos[numRestaurante];
        int restIndex = numRestaurante;
        numRestaurante++;   // só pra ficar bonitinho no output

        // PASSO 0: VERIFICAR SE RECURSO ESTÁ RESERVADO PARA MIM
        bool tenhoReserva = false;
        pthread_mutex_lock(&recursosMutex);
            if (isVeterano && reservaMoto[restIndex] == idLocal) {
                tenhoReserva = true;
            } else if (!isVeterano && reservaPedido[restIndex] == idLocal) {
                tenhoReserva = true;
            }
        pthread_mutex_unlock(&recursosMutex);

        // PASSO 1: PEGAR PRIMEIRO RECURSO
        // Se tenho reserva no primeiro recurso, espero até conseguir
        if (tenhoReserva) {
            pthread_mutex_lock(primeiraAcao);  // Bloqueante
            
            // Limpar minha reserva
            pthread_mutex_lock(&recursosMutex);
                if (isVeterano) { reservaMoto[restIndex] = -1; } 
                         else { reservaPedido[restIndex] = -1; }
            pthread_mutex_unlock(&recursosMutex);
        } else {
            // Sem reserva: tentar pegar normalmente
            pthread_mutex_lock(primeiraAcao);
            
            // Verificar se não está reservado para outro
            bool recursoReservado = false;
            pthread_mutex_lock(&recursosMutex);
                if (isVeterano && reservaMoto[restIndex] != -1) {
                    recursoReservado = true;
                } else if (!isVeterano && reservaPedido[restIndex] != -1) {
                    recursoReservado = true;
                }
            pthread_mutex_unlock(&recursosMutex);
            
            // Se está reservado para outro, desiste imediatamente
            if (recursoReservado) {
                pthread_mutex_unlock(primeiraAcao);
                usleep(200000 + (rand_r(&seedLocal) % 300000));
                continue;
            }
        }

        pthread_mutex_lock(&recursosMutex);
            if (isVeterano) { donoDaMoto[restIndex] = idLocal; }    // definindo que entregador é dono do quê
                     else { donoDoPedido[restIndex] = idLocal; }
        pthread_mutex_unlock(&recursosMutex);

        log("pegou recurso", coutMutex, spaces, cor, tipoEntregador, threadID, primeiroRecurso, numRestaurante, RESET_COR);

        sleep(SLEEP_TIME);

        // PASSO 2: TENTAR PEGAR SEGUNDO RECURSO
        bool conseguiuSegundo = false;
        int tentativas = 0;
        
        while (!conseguiuSegundo && tentativas < 15) {
            if (pthread_mutex_trylock(segundaAcao) == 0) {
                // VERIFICAR SE ESTÁ RESERVADO PARA OUTRO
                bool reservadoParaOutro = false;
                pthread_mutex_lock(&recursosMutex);
                    if (isVeterano && reservaPedido[restIndex] != -1 && reservaPedido[restIndex] != idLocal) {
                        reservadoParaOutro = true;
                    } else if (!isVeterano && reservaMoto[restIndex] != -1 && reservaMoto[restIndex] != idLocal) {
                        reservadoParaOutro = true;
                    }
                pthread_mutex_unlock(&recursosMutex);
                
                if (reservadoParaOutro) {
                    // Recurso reservado para outro, não posso pegar
                    pthread_mutex_unlock(segundaAcao);
                    tentativas++;
                    usleep(100000);
                    continue;
                }
                
                // CONSEGUIU! Marcar propriedade
                pthread_mutex_lock(&recursosMutex);
                    if (isVeterano) { 
                        donoDoPedido[restIndex] = idLocal;
                        reservaPedido[restIndex] = -1;  // Limpar qualquer reserva
                    } else { 
                        donoDaMoto[restIndex] = idLocal;
                        reservaMoto[restIndex] = -1;    // Limpar qualquer reserva
                    }
                pthread_mutex_unlock(&recursosMutex);
                
                conseguiuSegundo = true;
                
            } else {
                // NÃO CONSEGUIU - POSSÍVEL DEADLOCK
                tentativas++;
                
                // Só toma decisão após confirmar deadlock (3+ tentativas)
                if (tentativas >= 3) {
                    
                    // DETECTAR QUEM É O ADVERSÁRIO
                    pthread_mutex_lock(&recursosMutex);
                        int idAdversario = isVeterano ? donoDoPedido[restIndex] : donoDaMoto[restIndex];
                    pthread_mutex_unlock(&recursosMutex);
                    
                    // Se há adversário válido, aplicar regra de resolução
                    if (idAdversario != -1 && idAdversario < (int)lucrosGlobais.size()) {
                        
                        pthread_mutex_lock(&controleMutex);
                        
                        float meuLucro = lucrosGlobais[idLocal];
                        float lucroAdversario = lucrosGlobais[idAdversario];
                        int minhaPrioridade = prioridades[idLocal];
                        int prioridadeAdversario = prioridades[idAdversario];
                        
                        bool euDesisto = false;
                        
                        // REGRA 1: Quem tem MAIS lucro desiste
                        if (meuLucro > lucroAdversario + 5.0f) {
                            euDesisto = true;
                        } 
                        else if (lucroAdversario > meuLucro + 5.0f) {
                            euDesisto = false;
                        }
                        // REGRA 2: Prioridade (aging)
                        else if (minhaPrioridade > prioridadeAdversario) {
                            euDesisto = false;
                        }
                        else if (prioridadeAdversario > minhaPrioridade) {
                            euDesisto = true;
                        }
                        // REGRA 3: Veterano tem preferência
                        else {
                            euDesisto = !isVeterano;
                        }
                        
                        if (euDesisto) {
                            // DESISTIR E RESERVAR RECURSO PARA O ADVERSÁRIO
                            prioridades[idLocal]++;
                            tentativasRecurso[idLocal] = 0;
                            
                            pthread_mutex_unlock(&controleMutex);
                            
                            // *** RESERVAR MEU RECURSO PARA O ADVERSÁRIO ***
                            pthread_mutex_lock(&recursosMutex);
                                if (isVeterano) { 
                                    donoDaMoto[restIndex] = -1;
                                    reservaMoto[restIndex] = idAdversario;  // RESERVA!
                                } else { 
                                    donoDoPedido[restIndex] = -1;
                                    reservaPedido[restIndex] = idAdversario;  // RESERVA!
                                }
                            pthread_mutex_unlock(&recursosMutex);
                            
                            pthread_mutex_unlock(primeiraAcao);
                            
                            log("desistiu", coutMutex, spaces, cor, tipoEntregador, threadID, primeiroRecurso, numRestaurante, RESET_COR);
                            
                            // Backoff aleatório progressivo
                            int backoffTime = 300000 + (prioridades[idLocal] * 100000) + (rand_r(&seedLocal) % 500000);
                            usleep(backoffTime);
                            
                            break;
                        } else {
                            // EU FICO
                            tentativasRecurso[idLocal]++;
                            pthread_mutex_unlock(&controleMutex);
                        }
                        
                    }
                }
                
                usleep(50000 + (rand_r(&seedLocal) % 100000));
            }
        }
        
        // Se não conseguiu após todas tentativas
        if (!conseguiuSegundo) {
            pthread_mutex_lock(&controleMutex);
                prioridades[idLocal]++;
            pthread_mutex_unlock(&controleMutex);
            
            pthread_mutex_lock(&recursosMutex);
                if (isVeterano) { donoDaMoto[restIndex] = -1; }
                else { donoDoPedido[restIndex] = -1; }
            pthread_mutex_unlock(&recursosMutex);
            
            pthread_mutex_unlock(primeiraAcao);
            
            usleep(500000 + (rand_r(&seedLocal) % 500000));
            continue;
        }
        
        // ========= SUCESSO! AMBOS RECURSOS ADQUIRIDOS =========
        
        log("pegou recurso", coutMutex, spaces, cor, tipoEntregador, threadID, segundoRecurso, numRestaurante, RESET_COR);

        float ganho = (rand_r(&seedLocal) % 4001 + 1000) / 100.0f;

        pthread_mutex_lock(&controleMutex);
            lucrosGlobais[idLocal] += ganho;
            prioridades[idLocal] = 0;
            tentativasRecurso[idLocal] = 0;
        pthread_mutex_unlock(&controleMutex);

        log("fez a entrega", coutMutex, spaces, cor, tipoEntregador, threadID, primeiroRecurso, numRestaurante, RESET_COR);

        // Limpar propriedade dos recursos
        pthread_mutex_lock(&recursosMutex);
            donoDoPedido[restIndex] = -1;
            donoDaMoto[restIndex] = -1;
        pthread_mutex_unlock(&recursosMutex);

        pthread_mutex_unlock(segundaAcao);
        pthread_mutex_unlock(primeiraAcao);

        pthread_mutex_lock(&controleMutex);
            float lucroAtual = lucrosGlobais[idLocal];
        pthread_mutex_unlock(&controleMutex);

        log("possui R$", coutMutex, spaces, cor, tipoEntregador, threadID, primeiroRecurso, numRestaurante, RESET_COR, lucroAtual);

        usleep(500000);
    }

    pthread_exit(NULL);
}


int main(void){

    if (NUM_ENTREG < 2){
        std::cerr << "Devem existir mais de 2 entregadores de cada tipo!" << std::endl;
        return 1;
    } 
    if (!(NUM_ENTREG * 2 > NUM_REST)){
        std::cerr << "Pra ser divertido devem existir mais entregadores que restaurantes!" << std::endl;
        return 2;
    }

    std::cout << buildTitulo(NUM_REST);
    tabsRestaurante = buildTabs(NUM_REST);

    donoDoPedido.resize(NUM_REST, -1);
    donoDaMoto.resize(NUM_REST, -1);
    reservaPedido.resize(NUM_REST, -1);
    reservaMoto.resize(NUM_REST, -1);

    std::vector<pthread_t> entregNov(NUM_ENTREG);
    std::vector<pthread_t> entregVet(NUM_ENTREG);

    initMutexes(pedidos);
    initMutexes(motos);

    spawnThreads(entregNov, "Novato", &fazerEntrega);
    spawnThreads(entregVet, "Veterano", &fazerEntrega);

    joinThreads(entregNov);
    joinThreads(entregVet);

    destroyMutexes(pedidos);
    destroyMutexes(motos);

    std::cout << "cheguei no final" << std::endl;

    return 0;
}