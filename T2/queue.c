// Pedro Tricossi GRR20203895

#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

// Retorna tamanho da fila
int queue_size (queue_t *queue){
    int element = 1;
    queue_t *aux = queue;
    queue_t *next_item = NULL;
    
    if (queue == NULL)
        return 0;
    
    if(queue->next == NULL)
        return 1;
    
    next_item = queue->next;

    while (next_item != aux)
    {
        element++;
        next_item = next_item->next;
    }
    
    return element;
}

// Imprime fila
void queue_print (char *name, queue_t *queue, void print_elem (void*) ){
    queue_t *aux = queue;
    //queue_t *next = NULL;
    int i;

    printf("%s", name);
    printf("[");

    for(i=0; i<queue_size(queue); i++){
        print_elem(aux);
        printf(" ");
        aux = aux->next;
    }

    printf("]\n");


}

// Adiciona novo elemente a fila
int queue_append (queue_t **queue, queue_t *elem){
    queue_t *last = NULL;
    // Verificação de existencia fila e elementos
    if(queue == NULL)
        return 1;
    
    if(elem == NULL)
        return 1;
    
    if(elem->next != NULL && elem->prev != NULL)
        return 1;
    
    // primeiro elementro da fila
    if(queue[0] == NULL){
        queue[0] = elem;
        queue[0]->next = elem;
        queue[0]->prev = elem;

        return 0;
    }

    last = queue[0]->prev;

    elem->prev = last;
    elem->next = queue[0];

    last->next = elem;
    queue[0]->prev = elem;
    

    return 0;
}

// Remove elementos da fila
int queue_remove (queue_t **queue, queue_t *elem){
    queue_t *aux = NULL;
    queue_t *rem = NULL;

    // Verificação de existencia fila e elementos
    if(queue == NULL)
        return 1;
    
    if(queue[0] == NULL)
        return 1;
    
    if(elem == NULL)
        return 1;
    
    aux = queue[0]->next;

    if(elem == queue[0]){
        rem = elem;
    }else{
        while(aux != queue[0]){
            if(aux == elem)
                rem = elem;
            aux = aux->next;
        }
    }

    if(rem == NULL)
        return 1;
    
    // remove da fila com apenas um elemento
    if(rem->next == queue[0] && rem->prev == queue[0] && rem == queue[0]){
        rem->next = NULL;
        rem->prev = NULL;
        queue[0] = NULL;

        return 0;
    }

    // remove primeiro elemento da fila
    if(rem == queue[0]){
        rem->prev->next = rem->next;
        rem->next->prev = rem->prev;
        queue[0] = queue[0]->next;

        rem->next = NULL;
        rem->prev = NULL;

        return 0;
    }

    // remove segundo elemento da fila
    if(rem->next == queue[0] && rem->prev == queue[0]){
        queue[0]->prev = rem->prev;
        queue[0]->next = rem->next;

        rem->next = NULL;
        rem->prev = NULL;

        return 0;
    }

    rem->prev->next = rem->next;
    rem->next->prev = rem->prev;

    rem->next = NULL;
    rem->prev = NULL;

    return 0;
}
