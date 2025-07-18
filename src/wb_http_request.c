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

    // end of first line; this will need to be updated once we start
    // parsing headers
    char* header_end = strstr(raw_request, "\r\n\r\n");
    if (header_end) {
        size_t header_len = header_end - (strstr(raw_request, "\r\n") + 2);
        request->raw_headers =
            strndup(strstr(raw_request, "\r\n") + 2, header_len);
        request->raw_remaining = strdup(header_end + 4);
    } else {
        request->raw_headers = strdup("");
        request->raw_remaining = strdup("");
    }

    return request;
}

void wb_http_req_display(wb_http_req_t* request) {
    printf("---------------------\n");
    printf("HTTP REQUEST: method=%s, path=%s, version=%s\n", request->method,
           request->path, request->version);
    printf("%s\r\n\r\n%s", request->raw_headers, request->raw_remaining);
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

    sprintf(req_str, "%s %s %s\r\n%s\r\n\r\n%s", request->method, request->path,
            request->version, request->raw_headers, request->raw_remaining);
    return req_str;
}

void wb_http_req_destroy(wb_http_req_t* request) {
    if (request) {
        free(request->raw_request);
        free(request->raw_headers);
        free(request->raw_remaining);
        free(request);
    }
}