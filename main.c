// Lucas Kenzo Kawamoto 10396359 
// ANDREA MINDLIN TESSLER 10381702
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define MAX_PEDIDOS 5  // Capacidade máxima da fila de pedidos
#define NUM_CHEFS 3    // Número de chefs disponíveis
#define NUM_CLIENTES 10 // Número total de clientes

int fila_pedidos[MAX_PEDIDOS];  // Fila de pedidos
int num_pedidos = 0;            // Número de pedidos na fila
int pedidos_atendidos = 0;       // Número de pedidos atendidos
pthread_mutex_t mutex_fila;     // Mutex para proteger a fila de pedidos
pthread_cond_t cond_novo_pedido; // Condição para novo pedido
pthread_cond_t cond_pedido_pronto; // Condição para pedidos disponíveis
sem_t sem_chefs;                 // Semáforo para controlar o número de chefs disponíveis
int clientes_atendidos = 0;       // Contador de clientes atendidos
int finalizar = 0;               // Sinalizador para finalizar as threads dos chefs

// Função que simula o cliente fazendo um pedido
void* cliente(void* id) {
    int cliente_id = *((int*)id);

    pthread_mutex_lock(&mutex_fila);
    
    // Espera até que haja espaço na fila para um novo pedido
    while (num_pedidos == MAX_PEDIDOS) {
        pthread_cond_wait(&cond_pedido_pronto, &mutex_fila);
    }

    // Cliente faz um pedido
    fila_pedidos[num_pedidos] = cliente_id;
    num_pedidos++;
    printf("Cliente %d fez o pedido. (Pedidos na fila: %d)\n", cliente_id, num_pedidos);
    
    // Notifica os chefs que há um novo pedido
    pthread_cond_signal(&cond_novo_pedido);
    pthread_mutex_unlock(&mutex_fila);

    pthread_exit(NULL);
}

// Função que simula o chef preparando um pedido
void* chef(void* arg) {
    while (1) {
        sem_wait(&sem_chefs); // Espera até que haja um chef disponível

        pthread_mutex_lock(&mutex_fila);
        
        // Se todos os clientes já foram atendidos e não há mais pedidos, finalizar chef
        if (finalizar && num_pedidos == 0) {
            pthread_mutex_unlock(&mutex_fila);
            sem_post(&sem_chefs); // Libera o chef para sair
            break;
        }

        // Espera até que haja um pedido na fila
        while (num_pedidos == 0 && !finalizar) {
            pthread_cond_wait(&cond_novo_pedido, &mutex_fila);
        }

        if (num_pedidos > 0) {
            // Pega o próximo pedido
            int pedido = fila_pedidos[0];
            // Remove o pedido da fila
            for (int i = 1; i < num_pedidos; i++) {
                fila_pedidos[i-1] = fila_pedidos[i];
            }
            num_pedidos--;
            printf("Chef está preparando o pedido do Cliente %d. (Pedidos restantes: %d)\n", pedido, num_pedidos);
            pedidos_atendidos++;

            // Notifica que há espaço para novos pedidos
            pthread_cond_signal(&cond_pedido_pronto);
            pthread_mutex_unlock(&mutex_fila);

            // Simula o tempo de preparo
            sleep(2);
            printf("Pedido do Cliente %d está pronto!\n", pedido);

            sem_post(&sem_chefs); // Libera o chef para outro pedido
        } else {
            pthread_mutex_unlock(&mutex_fila);
        }
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t threads_clientes[NUM_CLIENTES];
    pthread_t thread_chef[NUM_CHEFS];
    int cliente_ids[NUM_CLIENTES];

    // Inicializa o mutex, condições e semáforo
    pthread_mutex_init(&mutex_fila, NULL);
    pthread_cond_init(&cond_novo_pedido, NULL);
    pthread_cond_init(&cond_pedido_pronto, NULL);
    sem_init(&sem_chefs, 0, NUM_CHEFS);

    // Cria as threads dos chefs
    for (int i = 0; i < NUM_CHEFS; i++) {
        pthread_create(&thread_chef[i], NULL, chef, NULL);
    }

    // Cria as threads dos clientes
    for (int i = 0; i < NUM_CLIENTES; i++) {
        cliente_ids[i] = i + 1;
        pthread_create(&threads_clientes[i], NULL, cliente, &cliente_ids[i]);
        sleep(1); // Simula o tempo entre clientes chegando
    }

    // Espera todas as threads dos clientes terminarem
    for (int i = 0; i < NUM_CLIENTES; i++) {
        pthread_join(threads_clientes[i], NULL);
    }

    // Sinaliza que todos os clientes já foram atendidos
    pthread_mutex_lock(&mutex_fila);
    finalizar = 1;
    pthread_cond_broadcast(&cond_novo_pedido); // Notifica todas as threads dos chefs
    pthread_mutex_unlock(&mutex_fila);

    // Espera todas as threads dos chefs terminarem
    for (int i = 0; i < NUM_CHEFS; i++) {
        pthread_join(thread_chef[i], NULL);
    }

    // Destrói mutex, condições e semáforo
    pthread_mutex_destroy(&mutex_fila);
    pthread_cond_destroy(&cond_novo_pedido);
    pthread_cond_destroy(&cond_pedido_pronto);
    sem_destroy(&sem_chefs);

    printf("Todos os clientes foram atendidos.\n");
    return 0;
}

