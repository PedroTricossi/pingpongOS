#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

task_t main_context;
task_t *atual;
int id;

void ppos_init (){
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);
    id = 0;

    main_context.id = id;
    id++;

    getcontext (&main_context.context);

    atual = &(main_context);
}

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
    id++;

    makecontext (&task->context, (void*)(*start_func), 1, arg) ;


    return 0;
}

int task_switch (task_t *task){
    task_t *aux = atual;

    atual = task;
    
    swapcontext (&(aux->context), &(task->context));

    return 0;
}

int task_id (){
    return atual->id;
}

void task_exit (int exit_code){
    task_t *aux = atual;

    atual = &main_context;
    swapcontext (&(aux->context), &main_context.context);
}