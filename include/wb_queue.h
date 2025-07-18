#ifndef QUEUE_H
#define QUEUE_H

#include "stdbool.h"

typedef struct wb_node {
    void* data;
    struct wb_node* next;
    struct wb_node* prev;
} wb_node;

typedef struct wb_queue {
    wb_node* head;
    wb_node* tail;
} wb_queue;

void wb_node_create(void* data);

void wb_queue_init(wb_queue* q);

bool wb_queue_enqueue(wb_queue* q, void* data);

void* wb_queue_dequeue(wb_queue* q);

#endif