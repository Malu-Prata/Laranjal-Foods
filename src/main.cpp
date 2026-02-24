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

#define SLEEP_CAMINHANDO_PRA_PEGAR_SEGUNDO_RECURSO 2    // 2 segundos
#define SLEEP_DESCULPA_POR_TENTAR_PEGAR_PRIMEIRO_RECURSO_RESERVADO_PARA_OUTRO_ENTREGADOR 300000 // 300ms
#define SLEEP_DESCULPA_POR_TENTAR_PEGAR_SEGUNDO_RECURSO_RESERVADO_PARA_OUTRO_ENTREGADOR  100000 // 100ms
#define SLEEP_ESPERA_PRA_PROXIMA_ENTREGA 500000 // 500ms

// ========= * PREVENÇÃO DE DEADLOCK COM RESERVA DE RECURSOS * =========

#define TENTATIVAS_CONFIRMAR_DEADLOCK 3
#define MAX_TENTATIVAS_TOTAIS_PEGAR_SEGUNDO_RECURSO 15
#define FATOR_PRIORIDADE 3

#define TEMPO_BASE_ENTRE_TENTATIVAS    50000   // 50ms
#define TEMPO_VARIACAO_TENTATIVAS      100000  // 100ms

#define TEMPO_BASE_BACKOFF_DESISTENCIA  300000  // 300ms
#define TEMPO_FATOR_PRIORIDADE          100000  // 100ms por nível de prioridade
#define TEMPO_VARIACAO_BACKOFF          500000  // 500ms

#define TEMPO_PAUSA_ENTRE_ENTREGAS     500000  // 500ms
#define TEMPO_VARIACAO_PAUSA_ENTREGAS  500000  // 500ms

pthread_mutex_t controleMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t recursosMutex = PTHREAD_MUTEX_INITIALIZER;

std::vector<float> lucrosGlobais;
std::vector<int>   prioridades;

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
        const char* primeiroRecurso   = (isVeterano) ? "a moto" : "o pedido";
        const char* segundoRecurso    = (isVeterano) ? "o pedido" : "a moto";
        pthread_mutex_t* primeiraAcao = (isVeterano) ? &motos[numRestaurante]   : &pedidos[numRestaurante];
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

        // PASSO 1: PEGAR PRIMEIRO RECURSO -> se não tiver a reserva e outro entregador a possuir, irá falhar e procurar outro restaurante
        if (tenhoReserva) {
            pthread_mutex_lock(primeiraAcao);                       // bloqueante, espera até conseguir
            
            pthread_mutex_lock(&recursosMutex);                     // limpando a reserva (já que agora tem posse de fato do recurso)
                if (isVeterano) { reservaMoto[restIndex] = -1; } 
                         else { reservaPedido[restIndex] = -1; }
            pthread_mutex_unlock(&recursosMutex);
        } else {
            pthread_mutex_lock(primeiraAcao);                       // se chegar aqui, o entregador não tem reserva, vai tentar pegar o 1º recurso na sorte
            
            bool recursoReservadoPraOutro = false;                  // verificar se o 1º recurso não está reservado para outro entregador
            pthread_mutex_lock(&recursosMutex);
                if (isVeterano && reservaMoto[restIndex] != -1) {
                    recursoReservadoPraOutro = true;
                } else if (!isVeterano && reservaPedido[restIndex] != -1) {
                    recursoReservadoPraOutro = true;
                }
            pthread_mutex_unlock(&recursosMutex);
            if (recursoReservadoPraOutro) {                         // se estiver, desiste imediatamente
                pthread_mutex_unlock(primeiraAcao);
                usleep(SLEEP_DESCULPA_POR_TENTAR_PEGAR_PRIMEIRO_RECURSO_RESERVADO_PARA_OUTRO_ENTREGADOR);
                continue;                                           // e procura outro restaurante
            }
        }

        pthread_mutex_lock(&recursosMutex);                         // definindo que entregador é dono do quê
            if (isVeterano) { donoDaMoto[restIndex] = idLocal; }
                     else { donoDoPedido[restIndex] = idLocal; }
        pthread_mutex_unlock(&recursosMutex);

        log("pegou recurso", coutMutex, spaces, cor, tipoEntregador, threadID, primeiroRecurso, numRestaurante, RESET_COR, 0.0);

        sleep(SLEEP_CAMINHANDO_PRA_PEGAR_SEGUNDO_RECURSO);

        // PASSO 2: TENTAR PEGAR SEGUNDO RECURSO
        bool conseguiuSegundo = false;
        int tentativas = 0;
        
        while (!conseguiuSegundo && tentativas < MAX_TENTATIVAS_TOTAIS_PEGAR_SEGUNDO_RECURSO) {
            if (pthread_mutex_trylock(segundaAcao) == 0) {
                // se chegou aqui, APARENTEMENTE ninguém tá usando o segundo recurso agora
                // VERIFICAR SE SEGUNDO RECURSO TÁ RESERVADO PARA OUTRO
                bool reservadoParaOutro = false;
                pthread_mutex_lock(&recursosMutex);
                    if (isVeterano && reservaPedido[restIndex] != -1 && reservaPedido[restIndex] != idLocal) {
                        reservadoParaOutro = true;
                    } else if (!isVeterano && reservaMoto[restIndex] != -1 && reservaMoto[restIndex] != idLocal) {
                        reservadoParaOutro = true;
                    }
                pthread_mutex_unlock(&recursosMutex);
                
                if (reservadoParaOutro) {   // recurso reservado para outro, não posso pegar
                    pthread_mutex_unlock(segundaAcao);
                    tentativas++;
                    usleep(SLEEP_DESCULPA_POR_TENTAR_PEGAR_SEGUNDO_RECURSO_RESERVADO_PARA_OUTRO_ENTREGADOR);
                    continue;
                }
                
                // CONSEGUIU PEGAR SEGUNDO RECURSO -> Marcar como dono e limpar reserva (caso tivesse)
                pthread_mutex_lock(&recursosMutex);
                    if (isVeterano) { 
                        donoDoPedido[restIndex] = idLocal;
                        if (reservaPedido[restIndex] == idLocal) { reservaPedido[restIndex] = -1; }
                    } else { 
                        donoDaMoto[restIndex] = idLocal;
                        if (reservaMoto[restIndex] == idLocal) { reservaMoto[restIndex] = -1; }
                    }
                pthread_mutex_unlock(&recursosMutex);
                
                conseguiuSegundo = true;
                
            } else {
                // NÃO CONSEGUIU PEGAR SEGUNDO RECURSO -> POSSÍVEL DEADLOCK
                tentativas++;
                
                if (tentativas >= TENTATIVAS_CONFIRMAR_DEADLOCK) { // (3)
                    
                    // achar adversário
                    pthread_mutex_lock(&recursosMutex);
                        int idAdversario = isVeterano ? donoDoPedido[restIndex] : donoDaMoto[restIndex];
                    pthread_mutex_unlock(&recursosMutex);
                    
                    // se houver adversário, aplicar a regra de resolução
                    if (idAdversario != -1 && idAdversario < (int) lucrosGlobais.size()) {
                        
                        pthread_mutex_lock(&controleMutex);
                        
                        float meuLucro = lucrosGlobais[idLocal];
                        float lucroAdversario = lucrosGlobais[idAdversario];
                        int minhaPrioridade = prioridades[idLocal];
                        int prioridadeAdversario = prioridades[idAdversario];
                        
                        bool euDesisto = false;
                        
                        // REGRA 1: quem tem MAIS lucro desiste
                        if (meuLucro > lucroAdversario) {
                            euDesisto = true;
                        } 
                        else if (lucroAdversario > meuLucro) {
                            euDesisto = false;
                        }
                        // REGRA 2: prioridade (aging)
                        else if (minhaPrioridade > prioridadeAdversario) {
                            euDesisto = false;
                        }
                        else if (prioridadeAdversario > minhaPrioridade) {
                            euDesisto = true;
                        }
                        // REGRA 3: veterano tem preferência
                        else {
                            euDesisto = !isVeterano;
                        }
                        
                        if (euDesisto) {
                            // DESISTIR E RESERVAR RECURSO PARA O ADVERSÁRIO
                            prioridades[idLocal] = (prioridades[idLocal] + 1) * FATOR_PRIORIDADE;
                            prioridades[idLocal] = std::min(prioridades[idLocal], 100);
                            
                            pthread_mutex_unlock(&controleMutex);
                            
                            // *** RESERVAR MEU RECURSO PARA O ADVERSÁRIO ***
                            pthread_mutex_lock(&recursosMutex);
                                if (isVeterano) { 
                                    donoDaMoto[restIndex] = -1;
                                    reservaMoto[restIndex] = idAdversario;
                                } else { 
                                    donoDoPedido[restIndex] = -1;
                                    reservaPedido[restIndex] = idAdversario;
                                }
                            pthread_mutex_unlock(&recursosMutex);
                            
                            pthread_mutex_unlock(primeiraAcao);
                            
                            log("desistiu", coutMutex, spaces, cor, tipoEntregador, threadID, primeiroRecurso, numRestaurante, RESET_COR, 0.0);
                            
                            // backoff aleatório progressivo
                            int backoffTime = TEMPO_BASE_BACKOFF_DESISTENCIA + 
                                              (prioridades[idLocal] * TEMPO_FATOR_PRIORIDADE) + 
                                              (rand_r(&seedLocal) % TEMPO_VARIACAO_BACKOFF);
                            usleep(backoffTime);
                            
                            break;  // sai do while e volta pro início do loop principal
                        } else {
                            pthread_mutex_unlock(&controleMutex);
                        }
                        
                    }
                }
                
                usleep(TEMPO_BASE_ENTRE_TENTATIVAS + (rand_r(&seedLocal) % TEMPO_VARIACAO_TENTATIVAS));
            }
        }
        
        // se não conseguiu pegar o 2º recurso após todas tentativas, desiste do 1º recurso e recomeça
        if (!conseguiuSegundo) {
            pthread_mutex_lock(&controleMutex);
                prioridades[idLocal] = (prioridades[idLocal] + 1) * FATOR_PRIORIDADE;
                prioridades[idLocal] = std::min(prioridades[idLocal], 100);
            pthread_mutex_unlock(&controleMutex);
            
            pthread_mutex_lock(&recursosMutex);
                if (isVeterano) { donoDaMoto[restIndex] = -1; }
                else          { donoDoPedido[restIndex] = -1; }
            pthread_mutex_unlock(&recursosMutex);
            
            pthread_mutex_unlock(primeiraAcao); // larga primeiro recurso
            
            usleep(TEMPO_PAUSA_ENTRE_ENTREGAS + (rand_r(&seedLocal) % TEMPO_VARIACAO_PAUSA_ENTREGAS));
            continue;  // volta pro início do loop principal
        }
        
        // ========= AMBOS RECURSOS FORAM ADQUIRIDOS =========
        
        log("pegou recurso", coutMutex, spaces, cor, tipoEntregador, threadID, segundoRecurso, numRestaurante, RESET_COR, 0.0);

        float ganho = (rand_r(&seedLocal) % 4001 + 1000) / 100.0f;  // entre 10,00 e 50,00 reais

        pthread_mutex_lock(&controleMutex);
            lucrosGlobais[idLocal] += ganho;
            float lucroAtual = lucrosGlobais[idLocal];
            prioridades[idLocal] = 0;                   // reseta a prioridade após sucesso
        pthread_mutex_unlock(&controleMutex);

        log("fez a entrega", coutMutex, spaces, cor, tipoEntregador, threadID, primeiroRecurso, numRestaurante, RESET_COR, 0.0);

        // limpar propriedade dos recursos
        pthread_mutex_lock(&recursosMutex);
            donoDoPedido[restIndex] = -1;
            donoDaMoto[restIndex]   = -1;
        pthread_mutex_unlock(&recursosMutex);

        // liberar mutex dos recursos
        pthread_mutex_unlock(segundaAcao);
        pthread_mutex_unlock(primeiraAcao);

        log("possui R$", coutMutex, spaces, cor, tipoEntregador, threadID, primeiroRecurso, numRestaurante, RESET_COR, lucroAtual);

        usleep(SLEEP_ESPERA_PRA_PROXIMA_ENTREGA);  // pausa antes de próxima entrega (500ms)
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