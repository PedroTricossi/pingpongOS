// Pedro Tricossi GRR20203895

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

#define QUANTUM 20

task_t main_context, dispatcher_task;
task_t *currentTask, *nextTask, *userTasks;
int id, userTaskCounter = 0;

unsigned int globalClock;

struct sigaction action ;
struct itimerval timer;

unsigned int systime (){
    return globalClock;
}

// Escalonador baseado na ordem de entrada na fila.
// (remove o primeiro e elemento e depois coloca novamente no final da fila)
void schedulerFIFS(){ 
    if (nextTask != NULL){
        queue_remove((queue_t**) &userTasks, (queue_t*)nextTask);
        queue_append((queue_t**) &userTasks, (queue_t*)nextTask);
    }
    
    nextTask = userTasks;
};

// Escalonamento por prioridades, implementando aging.
// acha a menor prioridade, logo após diminui em uma uniade a prioridade de 
// todas as outras tasks.
void priorityScheduler(){
    task_t *aux = userTasks;
    nextTask = aux;
    int i = 0;

    while (i < userTaskCounter)
    {
        if(aux->pd < nextTask->pd)
            nextTask = aux;
        
        aux = aux->next;
        i++;
    }

    i = 0;
    nextTask->pd = nextTask->pe;

    while (i < userTaskCounter)
    {
        if(aux != nextTask)
            aux->pd--;
        
        aux = aux->next;
        i++;
        
    }    
    
}

// Dispacher para controle de task
void dispatcher(){
    unsigned int t1 = 0;
    unsigned int t2 = 0;


    while (userTaskCounter > 0)
    {
        priorityScheduler();

        if(nextTask != NULL){
            nextTask->tick = QUANTUM;

            t1 = systime();
            task_switch(nextTask);
            t2 = systime();
 
            nextTask->cpuTime += t2 - t1;    

            if(nextTask->status == 0){
                free(nextTask->context.uc_stack.ss_sp);
            }
        }
 
    }

    
    task_exit(1);
}

void tickCounter(){
    ++globalClock;
    --currentTask->tick;

    if(currentTask->tick < 1 && currentTask->preemptable == 0)
        task_yield();
}

void clockInit(){

  action.sa_handler = tickCounter ;
  sigemptyset (&action.sa_mask) ;
  action.sa_flags = 0 ;
  if (sigaction (SIGALRM, &action, 0) < 0)
  {
    perror ("Erro em sigaction: ") ;
    exit (1) ;
  }

  timer.it_value.tv_usec = 1000;
  timer.it_value.tv_sec  = 0;
  timer.it_interval.tv_usec = 1000;
  timer.it_interval.tv_sec  = 0;

  // arma o temporizador ITIMER_REAL (vide man setitimer)
  if (setitimer (ITIMER_REAL, &timer, 0) < 0)
  {
    perror ("Erro em setitimer: ") ;
    exit (1) ;
  }

}

// Inicia Sistema operacional
void ppos_init (){
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);

    globalClock = 0;
    clockInit();

    id = 0;
    main_context.id = id;
    id++;

    getcontext (&main_context.context);
    currentTask = &(main_context);

    task_create(&dispatcher_task, dispatcher,"");
}


// Cria uma nova task
int task_create (task_t *task, void (*start_func)(void *), void *arg){
    char *stack ;

    getcontext (&task->context) ;

    stack = malloc (STACKSIZE) ;
    if (stack)
    {
        task->context.uc_stack.ss_sp = stack ;
        task->context.uc_stack.ss_size = STACKSIZE ;
        task->context.uc_stack.ss_flags = 0 ;
        task->context.uc_link = 0 ;
    }
    else
    {
        perror ("Erro na criação da pilha: ") ;
        return 1;
    }

    task->id = id;
    task->status = 1;
    task->preemptable = 0;
    task->executionTime = systime();
    task->cpuTime = 0;
    task->activation = 0;

    if(task == &dispatcher_task)
        task->preemptable = 1;

    id++;

    makecontext (&task->context, (void*)(*start_func), 1, arg);

    // tudo que for criado após o dispacher entra na fila
    if(id > 2){
        userTaskCounter++;
        queue_append((queue_t**) &userTasks,(queue_t*) task);
    }

    return 0;
}

// Troca a task em execução
int task_switch (task_t *task){
    task_t *aux = currentTask;

    currentTask = task;
    task->activation++;
    
    swapcontext (&(aux->context), &(task->context));

    return 0;
}

int task_id (){
    return currentTask->id;
}

// Sai a task em execução
void task_exit (int exit_code){
    currentTask->preemptable = 1;
    task_t *aux = currentTask;
    unsigned int now = systime();
    
    currentTask->executionTime = now - currentTask->executionTime;

    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations \n", currentTask->id, currentTask->executionTime, currentTask->cpuTime, currentTask->activation);



    if(exit_code == 0){
        userTaskCounter--;
        queue_remove((queue_t**) &userTasks, (queue_t*)aux);
        currentTask->status = 0;
        
        task_yield();
    }

    if(exit_code == 1){
        currentTask = &main_context;
        
        swapcontext (&(aux->context), &main_context.context);
    }
}

void task_yield (){
    task_switch(&dispatcher_task);
};

// define a prioridade estática de uma tarefa (ou a tarefa currentTask)
void task_setprio (task_t *task, int prio){
    if(task == NULL){
        currentTask->pe = prio;
        currentTask->pd = prio;
    }
    else{
        task->pe = prio;
        task->pd = prio;
    }
    
}

// retorna a prioridade estática de uma tarefa (ou a tarefa currentTask)
int task_getprio (task_t *task){
    if(task == NULL){
        return currentTask->pe;
    }

    return task->pe;
}