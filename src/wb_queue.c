#include "wb_queue.h"
#include "stdio.h"
#include "stdbool.h"
#include "stdlib.h"

wb_node* create_node(void* data) {
    wb_node* node = malloc(sizeof(wb_node));
    if (!node) {
        perror("failed to allocate memory for wb_node");
        return NULL;
    }

    node->data = data;
    node->prev = NULL;
    node->next = NULL;
    return node;
}

void wb_queue_init(wb_queue* q) {
    q->head = NULL;
    q->tail = NULL;
};

bool wb_queue_enqueue(wb_queue* q, void* data) {
    wb_node* node = create_node(data);
    if (!node) {
        return false;
    }

    if (!q->head) {
        q->head = q->tail = node;
    } else {
        q->tail->prev = node;
        node->next = q->tail;
        q->tail = node;
    }

    return true;
}

void* wb_queue_dequeue(wb_queue* q) {
    if (!q->head) {
        return NULL;
    }

    wb_node* node = q->head;
    void* data = node->data;
    q->head = q->head->prev;

    if (q->head) {
        q->head->next = NULL;
    } else {
        q->tail = NULL;
    }

    free(node);
    return data;
}