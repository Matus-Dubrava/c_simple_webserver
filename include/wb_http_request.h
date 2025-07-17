#ifndef WB_HTTP_REQ_H
#define WB_HTTP_REQ_H

typedef struct wb_http_req_t {
    char* raw_request;
    char* method;
    char* path;
    char* version;
    char* raw_remaining;
} wb_http_req_t;

wb_http_req_t* wb_http_req_parse(char* raw_request);

void wb_http_req_display(wb_http_req_t* request);

void wb_http_req_destroy(wb_http_req_t* request);

#endif