#include "stdlib.h"
#include "stdio.h"
#include "assert.h"
#include "string.h"
#include "wb_http_request.h"
#include "wb_definitions.h"
#include "wb_queue.h"

void test_queue() {
    wb_queue q;
    wb_queue_init(&q);

    assert(q.head == NULL);
    assert(q.tail == NULL);

    int values[] = {1, 2, 3, 4, 5};
    size_t len = sizeof(values) / sizeof(values[0]);

    for (size_t i = 0; i < len; ++i) {
        wb_queue_enqueue(&q, (void*)&values[i]);
    }

    for (size_t i = 0; i < len; ++i) {
        int* value = wb_queue_dequeue(&q);
        assert(*value == values[i]);
    }

    assert(q.head == NULL);
    assert(q.tail == NULL);
}

void test_queue_http_req() {
    wb_queue q;
    wb_queue_init(&q);

    char* http_req_str =
        "GET /api HTTP/1.2\r\n\r\n<body><h1>test<h1></body>\r\n";

    wb_http_req_t* requests[] = {wb_http_req_parse(http_req_str),
                                 wb_http_req_parse(http_req_str)};

    size_t len = sizeof(requests) / sizeof(requests[0]);

    for (size_t i = 0; i < len; ++i) {
        wb_queue_enqueue(&q, requests[i]);
    }

    while (q.head) {
        wb_http_req_t* req = wb_queue_dequeue(&q);
        assert(strcmp(req->method, HTTP_METHOD_GET) == 0);
        assert(strcmp(req->path, "/api") == 0);
        wb_http_req_destroy(req);
    }

    assert(q.head == NULL);
    assert(q.tail == NULL);
}

int main() {
    test_queue();
    test_queue_http_req();
}