// Pedro Tricossi GRR20203895
// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev;		// ponteiros para usar em filas
  struct task_t *next;
  int id ;				// identificador da tarefa
  ucontext_t context ;			// contexto armazenado da tarefa
  short status ;			// pronta, rodando, suspensa, ...
  short preemptable ;			// pode ser preemptada?
  int pe;
  int pd;
  int tick;
  unsigned int cpuTime;
  unsigned int executionTime;
  int activation;
  struct task_t *waitFor;
  int exitCode;
  int sleepTime;
   // ... (outros campos serão adicionados mais tarde)
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  int lock;
  int count;
  int task_in_queue;
  int light_on;
  task_t *queue;
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  int max_msg;
  int max_size;
  int msg_in_queue;
  long **queue;
  int index;
  int first_msg;
  semaphore_t sem;
  semaphore_t tem_item;
  semaphore_t buffer;
} mqueue_t ;




#endif
