#ifndef MISC_QUEUE_H
#define MISC_QUEUE_H 1


struct Node{
    void* ptr;
    struct Node* next;
};

void enqueue(struct Node** head, void* new_ptr);
void* dequeue(struct Node** head);

#endif
