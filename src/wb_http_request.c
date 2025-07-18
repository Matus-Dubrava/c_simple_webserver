#include "wb_http_request.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

wb_http_req_t* wb_http_req_parse(char* raw_request) {
    wb_http_req_t* request = malloc(sizeof(wb_http_req_t));
    if (!request) {
        perror("failed to allocate memory for http header");
        return NULL;
    }

    request->raw_request = strdup(raw_request);
    request->method = strtok(request->raw_request, " ");
    request->path = strtok(NULL, " ");
    request->version = strtok(NULL, "\r\n");

    char* end_of_line = strstr(raw_request, "\r\n");
    if (end_of_line && *(end_of_line + 2) != '\0') {
        request->raw_remaining = end_of_line + 2;
    } else {
        request->raw_remaining = "";
    }

    return request;
}

void wb_http_req_display(wb_http_req_t* request) {
    printf("---------------------\n");
    printf("HTTP REQUEST: method=%s, path=%s, version=%s\n", request->method,
           request->path, request->version);
    printf("%s", request->raw_remaining);
}

char* wb_http_req_to_str(wb_http_req_t* request) {
    // @TODO: make sure that the request size make sense; this is hardcoded for
    // testing purposes
    size_t max_req_size = 1024 * 16 * sizeof(char);

    char* req_str = malloc(max_req_size);
    if (!req_str) {
        perror("failed to allocate memory for request string");
        return NULL;
    }

    sprintf(req_str, "%s %s %s\r\n%s", request->method, request->path,
            request->version, request->raw_remaining);
    return req_str;
}

void wb_http_req_destroy(wb_http_req_t* request) {
    if (request) {
        free(request->raw_request);
        free(request);
    }
}