// Pedro Tricossi GRR20203895

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

task_t main_context, dispatcher_task;
task_t *atual, *prox, *userTasks;
int id, userTaskCounter = 0;

// Escalonador baseado na ordem de entrada na fila.
// (remove o primeiro e elemento e depois coloca novamente no final da fila)
void schedulerFIFS(){ 
    if (prox != NULL){
        queue_remove((queue_t**) &userTasks, (queue_t*)prox);
        queue_append((queue_t**) &userTasks, (queue_t*)prox);
    }
    
    prox = userTasks;
};

// Escalonamento por prioridades, implementando aging.
// acha a menor prioridade, logo após diminui em uma uniade a prioridade de 
// todas as outras tasks.
void priorityScheduler(){
    task_t *aux = userTasks;
    prox = aux;
    int i = 0;

    while (i < userTaskCounter)
    {
        if(aux->pd < prox->pd)
            prox = aux;
        
        aux = aux->next;
        i++;
    }

    i = 0;
    prox->pd = prox->pe;

    while (i < userTaskCounter)
    {
        if(aux != prox)
            aux->pd--;
        
        aux = aux->next;
        i++;
        
    }    
    
}

// Dispacher para controle de task
void dispatcher(){
    while (userTaskCounter > 0)
    {
        priorityScheduler();

        if(prox != NULL){
            task_switch(prox);

            if(prox->status == 0)
                free(atual->context.uc_stack.ss_sp);
        }
    }
    
    task_exit(1);
}

// Inicia Sistema operacional
void ppos_init (){
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);
    id = 0;

    main_context.id = id;
    id++;

    getcontext (&main_context.context);
    atual = &(main_context);

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
    task_t *aux = atual;

    atual = task;
    
    swapcontext (&(aux->context), &(task->context));

    return 0;
}

int task_id (){
    return atual->id;
}

// Sai a task em execução
void task_exit (int exit_code){
    task_t *aux = atual;
    

    if(exit_code == 0){
        userTaskCounter--;
        queue_remove((queue_t**) &userTasks, (queue_t*)aux);
        atual->status = 0;
        
        swapcontext (&(aux->context), &dispatcher_task.context);

    }

    if(exit_code == 1){
        atual = &main_context;
        
        swapcontext (&(aux->context), &main_context.context);
    }
}

void task_yield (){
    task_switch(&dispatcher_task);
};

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio){
    if(task == NULL){
        atual->pe = prio;
        atual->pd = prio;
    }
    else{
        task->pe = prio;
        task->pd = prio;
    }
    
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task){
    if(task == NULL){
        return atual->pe;
    }

    return task->pe;
}