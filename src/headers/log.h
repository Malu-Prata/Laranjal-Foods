#include <string>
#include <pthread.h>

void log(std::string tipoLog, 
         pthread_mutex_t& coutMutex, 
         std::string spaces = "", 
         std::string cor = "", 
         std::string tipoEntregador = "", 
         int threadID = -1, 
         std::string recurso = "", 
         int numRestaurante = -1, 
         std::string resetCor = "",
         float lucroAtual = 0.0f);