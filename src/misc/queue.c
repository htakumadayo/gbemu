#include "misc/queue.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>


void enqueue(struct Node** head, void* new_ptr){
    struct Node* new_node = malloc(sizeof(struct Node));
    if(!new_node){
        ERR("Queue node creation failed");
        return;
    }

    new_node->ptr = new_ptr;
    new_node->next = *head;

    *head = new_node;

    printf("Something added to queue\n");
}

void* dequeue(struct Node** head){
    struct Node *current, *prev = NULL;
    void* out_ptr = NULL;

    if(*head == NULL) return NULL;
    current = *head;
    while(current->next != NULL){
        prev = current;
        current = current->next;
    }

    out_ptr = current->ptr;
    free(current);
    if(prev){
        prev->next = NULL;
    }
    else{
        *head = NULL;
    }
    printf("Dequed\n");
    return out_ptr;
}
