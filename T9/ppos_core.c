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
task_t *currentTask, *userTasks, *suspended_task, *sleep_task;
int id, ready_task_counter, suspended_task_counter, sleep_task_counter = 0;

unsigned int globalClock;

struct sigaction action ;
struct itimerval timer;

unsigned int systime (){
    return globalClock;
}

// Escalonador baseado na ordem de entrada na fila.
// (remove o primeiro e elemento e depois coloca novamente no final da fila)
// void schedulerFIFS(){ 
//     if (nextTask != NULL){
//         queue_remove((queue_t**) &userTasks, (queue_t*)nextTask);
//         queue_append((queue_t**) &userTasks, (queue_t*)nextTask);
//     }
    
//     nextTask = userTasks;
// };

// Escalonamento por prioridades, implementando aging.
// acha a menor prioridade, logo após diminui em uma uniade a prioridade de 
// todas as outras tasks.
task_t* priorityScheduler(){
    task_t *aux = userTasks;
    task_t *next = userTasks;
    int i = 0;

    if(aux != NULL){
        while (i < ready_task_counter)
        {
            if(aux->pd <= next->pd)
                next = aux;

            if(aux->pd > -20)
                aux->pd--;

            aux = aux->next;
            i++;
        }

        next->pd = next->pe;
    }

    return next;
}

void update_sleep_queue(){
    task_t *aux = sleep_task;
    int i = 0;

    if(aux != NULL){
        while (i <= sleep_task_counter)
        {
            if(aux->sleepTime <= systime()){
                queue_remove((queue_t**) &sleep_task, (queue_t*)aux);
                queue_append((queue_t**) &userTasks, (queue_t*)aux);
            }

            aux = aux->next;
            i++;
        }
    }
}

// Dispacher para controle de task
void dispatcher(){
    unsigned int t1 = 0;
    unsigned int t2 = 0;
    task_t* nextTask = NULL;

    while (ready_task_counter != 0)
    {
        update_sleep_queue();
        nextTask = priorityScheduler();

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
    
    task_exit(42);
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

int create_main_task(){
    char *stack ;

    getcontext (&main_context.context);

    stack = malloc (STACKSIZE) ;
    if (stack)
    {
        main_context.context.uc_stack.ss_sp = stack ;
        main_context.context.uc_stack.ss_size = STACKSIZE ;
        main_context.context.uc_stack.ss_flags = 0 ;
        main_context.context.uc_link = 0 ;
    }else
    {
        perror ("Erro na criação da pilha: ") ;
        return 1;
    }

    main_context.id = id;
    main_context.status = 1;
    main_context.preemptable = 0;
    main_context.tick = QUANTUM;
    main_context.executionTime = systime();
    main_context.cpuTime = 0;
    main_context.activation = 0;
    main_context.pe = 0;
    main_context.pd = 0;

    ready_task_counter++;
    id++;

    queue_append((queue_t**) &userTasks,(queue_t*) &main_context);

    return 0;
}

// Inicia Sistema operacional
void ppos_init (){
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);

    globalClock = 0;
    clockInit();

    create_main_task();
    currentTask = &main_context;

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
    task->pe = 0;
    task->pd = 0;

    if(task == &dispatcher_task)
        task->preemptable = 1;

    id++;

    makecontext (&task->context, (void*)(*start_func), 1, arg);

    // tudo que for criado após o dispacher entra na fila
    if(id > 2){
        ready_task_counter++;
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

void task_resume (task_t *task, task_t **queue){

}

void update_suspended_queue(task_t *completed){
    task_t *aux = suspended_task;
    int i = 0;

    if(aux != NULL){
        while (i <= suspended_task_counter)
        {
            if(aux->waitFor == completed){
                queue_remove((queue_t**) &suspended_task, (queue_t*)aux);
                queue_append((queue_t**) &userTasks, (queue_t*)aux);
            }

            aux = aux->next;
            i++;
        }
    }

}


// Sai a task em execução
void task_exit (int exit_code){
    currentTask->preemptable = 1;
    task_t *aux = currentTask;
    unsigned int now = systime();
    
    currentTask->executionTime = now - currentTask->executionTime;

    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations \n", currentTask->id, currentTask->executionTime, currentTask->cpuTime, currentTask->activation);

    if(exit_code == 42){
        currentTask = &main_context;
        
        swapcontext (&(aux->context), &main_context.context);
    }else{

    ready_task_counter--;
    aux->exitCode = exit_code;
    currentTask->status = 0;
    queue_remove((queue_t**) &userTasks, (queue_t*)aux);
    update_suspended_queue(aux);
    
    task_yield();
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

void task_suspend (task_t **queue){
    task_t *aux = userTasks;
    int i = 0;

    while (i < ready_task_counter)
    {
        if(aux == currentTask){
            queue_remove((queue_t**) &userTasks, (queue_t*)aux);
            queue_append((queue_t**) queue, (queue_t*)aux);
        }

        if(aux->next == userTasks){
            queue_append((queue_t**) queue, (queue_t*) currentTask);
        }

        aux = aux->next;
        i++;
    }

}

int task_join (task_t *task){
    currentTask->waitFor = task;
    
    if(task->status != 0){
        currentTask->status = 2;
        suspended_task_counter++;
        
        task_suspend(&suspended_task);
        
        task_yield();
    }

    return task->exitCode;
}

void task_sleep (int t){
    currentTask->sleepTime = systime() + t;
    sleep_task_counter++;

    task_suspend(&sleep_task);
    task_yield();
}