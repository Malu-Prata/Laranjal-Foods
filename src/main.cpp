#include <iostream>
#include <pthread.h>    // variáveis e funções daqui possuem prefixo 'pthread_'
#include <unistd.h>     // getpid(), gettid(), sleep(),

void* printOi(void* arg){
    std::cout << "oi! o meu TID é " << gettid() << std::endl;
    sleep(3);
    return NULL;
}

int main(void){

    pthread_t thread1;
    pthread_create(&thread1, NULL, printOi, NULL);

        pthread_t thread2;
    pthread_create(&thread2, NULL, printOi, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

        std::cout << "cheguei aqui" << std::endl;

    return 0;
}