#include "misc/queue.h"
#include <stdio.h>


int main(){
    int a=0, b=1, c=2, d=3;
    struct Node* head = NULL;
    enqueue(&head, &a);
    enqueue(&head, &a);
    enqueue(&head, &c);
    enqueue(&head, &d);
    enqueue(&head, &c);
    enqueue(&head, &b);

    int answer[] = {0, 0, 2, 3, 2, 1};
    size_t i = 0;
    while(head != NULL){
        int pop = *(int*)dequeue(&head);
        if(pop != answer[i]){
            return -1 * i;
        }
        ++i;
    }

    return 0;
}
