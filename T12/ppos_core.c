// Pedro Tricossi GRR20203895

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"
#include <string.h>

#define QUANTUM 20

task_t main_context, dispatcher_task;
task_t *current_task, *running_task, *suspended_task, *sleep_task;
int id, ready_task_counter, suspended_task_counter, sleep_task_counter = 0;

unsigned int globalClock;

struct sigaction action ;
struct itimerval timer;

unsigned int systime (){
    return globalClock;
}


// Escalonamento por prioridades, implementando aging.
// acha a menor prioridade, logo após diminui em uma uniade a prioridade de 
// todas as outras tasks.
task_t* priorityScheduler(){
    task_t *aux = running_task;
    task_t *next = running_task;
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
    task_t *temp = sleep_task;

    if(temp != NULL) {
        do {
            task_t *aux = temp->next;

            if(temp->sleepTime <= systime()) {
                queue_remove((queue_t**)&sleep_task, (queue_t*)temp);
                queue_append((queue_t **)&running_task, (queue_t*)temp);
            }

            temp = aux;
        } while(sleep_task != NULL && temp != sleep_task);
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
    --current_task->tick;

    if(current_task->tick < 1 && current_task->preemptable == 0)
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

    queue_append((queue_t**) &running_task,(queue_t*) &main_context);

    return 0;
}

// Inicia Sistema operacional
void ppos_init (){
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);

    globalClock = 0;
    clockInit();

    create_main_task();
    current_task = &main_context;

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
        queue_append((queue_t**) &running_task,(queue_t*) task);
    }

    return 0;
}

// Troca a task em execução
int task_switch (task_t *task){
    task_t *aux = current_task;

    current_task = task;
    task->activation++;
    
    swapcontext (&(aux->context), &(task->context));

    return 0;
}

int task_id (){
    return current_task->id;
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
                queue_append((queue_t**) &running_task, (queue_t*)aux);
            }

            aux = aux->next;
            i++;
        }
    }

}


// Sai a task em execução
void task_exit (int exit_code){
    current_task->preemptable = 1;
    task_t *aux = current_task;
    unsigned int now = systime();
    
    current_task->executionTime = now - current_task->executionTime;

    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations \n", current_task->id, current_task->executionTime, current_task->cpuTime, current_task->activation);

    if(exit_code == 42){
        current_task = &main_context;
        
        swapcontext (&(aux->context), &main_context.context);
    }else{

    ready_task_counter--;
    aux->exitCode = exit_code;
    current_task->status = 0;
    queue_remove((queue_t**) &running_task, (queue_t*)aux);
    update_suspended_queue(aux);
    
    task_yield();
    }
}

void task_yield (){
    task_switch(&dispatcher_task);
};

// define a prioridade estática de uma tarefa (ou a tarefa current_task)
void task_setprio (task_t *task, int prio){
    if(task == NULL){
        current_task->pe = prio;
        current_task->pd = prio;
    }
    else{
        task->pe = prio;
        task->pd = prio;
    }
    
}

// retorna a prioridade estática de uma tarefa (ou a tarefa current_task)
int task_getprio (task_t *task){
    if(task == NULL){
        return current_task->pe;
    }

    return task->pe;
}

void task_suspend (task_t **queue){
    queue_remove((queue_t**) &running_task, (queue_t*)current_task);
    queue_append((queue_t**) queue, (queue_t*)current_task);

}

int task_join (task_t *task){
    current_task->waitFor = task;
    
    if(task->status != 0){
        current_task->status = 2;
        suspended_task_counter++;
        
        task_suspend(&suspended_task);
        
        task_yield();
    }

    return task->exitCode;
}

void task_sleep (int t){
    current_task->preemptable = 1;
    current_task->sleepTime = systime() + t;
    sleep_task_counter++;

    task_suspend(&sleep_task);
    current_task->preemptable = 0;
    task_yield();
}

void return_to_running(semaphore_t *s){
    task_t *aux = s->queue;
    queue_remove((queue_t**)&(s->queue), (queue_t*)aux);

    queue_append((queue_t **)&running_task, (queue_t*)aux);
}

// Atomic operations used for mutual exclusion
void enter(int *lock) {
  while (__sync_fetch_and_or(lock, 1));   // busy waiting
}

void leave(int *lock){
    (*lock) = 0;
}

// Semaphores
int sem_create(semaphore_t *s, int value) {
    current_task->preemptable = 1;

    if (s->light_on) {
        current_task->preemptable = 0;
        return -1;
    }

    s->count = value;
    s->queue = NULL;
    s->light_on = 1;
    s->lock = 0;

    current_task->preemptable = 0;
    return 0;
}

int sem_down(semaphore_t *s) {
    current_task->preemptable = 1;

    if (s->light_on == 0) {
        current_task->preemptable = 0;
        return -1;
    }
    
    enter(&(s->lock));
    s->count--;
    if (s->count < 0) {
        task_suspend(&s->queue);
        leave(&(s->lock));

        task_yield();
    }
    else {
        leave(&(s->lock));
    }
    
    current_task->preemptable = 0;
    return 0;
}

int sem_up(semaphore_t *s) {
    current_task->preemptable = 1;

    if (s->light_on == 0) {
        current_task->preemptable = 0;
        return -1;
    }

    enter(&(s->lock));
    s->count++;
    if(s->count <= 0) {
        return_to_running(s);
    }
    leave(&(s->lock));

    current_task->preemptable = 0;
    return 0;
}


int sem_destroy(semaphore_t *s) {
    task_t *temp;
    current_task->preemptable = 1;

    if (s->light_on == 0) {
        current_task->preemptable = 0;
        return -1;
    }

    enter(&(s->lock));
    temp = s->queue;

    while(temp != NULL) {

        return_to_running(s);

        temp = s->queue;
    }
    
    leave(&(s->lock));

    current_task->preemptable = 0;
    return 0;
}

// cria uma fila para até max mensagens de size bytes cada
int mqueue_create (mqueue_t *queue, int max, int size){
    queue->queue = (void *) malloc(max * size);

    if(queue->queue == NULL)
        return -1;
    
    sem_create(&queue->sem, max);
    sem_create(&queue->tem_item, 0);
    sem_create(&queue->buffer, 1);

    queue->index = 0;
    queue->first_msg = 0;
    queue->max_msg = max;
    queue->max_size = size;
    queue->msg_in_queue = 0;

    return 0;
}

// envia uma mensagem para a fila
int mqueue_send (mqueue_t *queue, void *msg){
    if(queue->queue == NULL)
        return -1;
    
    sem_down(&queue->sem);
    sem_down(&queue->buffer);
    current_task->preemptable = 1;

    queue->queue[queue->index] = msg;
    queue->index = (queue->index + 1) % queue->max_msg;
    queue->msg_in_queue++;

    sem_up(&queue->tem_item);
    sem_up(&queue->buffer);
    current_task->preemptable = 0;
    
    return 0;
}

// recebe uma mensagem da fila
int mqueue_recv (mqueue_t *queue, void *msg){
    if(queue->queue == NULL)
        return -1;
    
    sem_down(&queue->tem_item);
    sem_down(&queue->buffer);
    current_task->preemptable = 1;

    if(queue != NULL && queue->queue != NULL && queue->queue[queue->first_msg] != NULL){
        memcpy(msg, queue->queue[queue->first_msg], queue->max_size);

        queue->first_msg = (queue->first_msg + 1) % queue->max_msg;
        queue->msg_in_queue--;
    }else {
        return -1;
    }

    
    sem_up(&queue->sem);
    sem_up(&queue->buffer);
    current_task->preemptable = 0;

    return 0;
}

// destroi a fila, liberando as tarefas bloqueadas
int mqueue_destroy (mqueue_t *queue){
    sem_destroy(&queue->sem);
    sem_destroy(&queue->tem_item);
    sem_destroy(&queue->buffer);

    queue->queue = NULL;

    return 0;
}

// informa o número de mensagens atualmente na fila
int mqueue_msgs (mqueue_t *queue){    
    return queue->msg_in_queue; 
}