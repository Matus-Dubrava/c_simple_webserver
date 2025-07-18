#include "wb_http_response.h"

#include "stdlib.h"
#include "string.h"
#include "stdio.h"

wb_http_resp_t* wb_http_resp_parse(char* raw_response) {
    wb_http_resp_t* resp = malloc(sizeof(wb_http_resp_t));
    if (!resp) {
        perror("failed to allocate memory for http response");
        return NULL;
    }

    resp->raw_response = strdup(raw_response);
    return resp;
}

void wb_http_resp_destroy(wb_http_resp_t* resp) {
    if (resp) {
        free(resp->raw_response);
        free(resp);
    }
}

void wb_http_resp_display(wb_http_resp_t* resp) {
    printf("---------------------\n");
    printf("HTTP RESPONSE:\n%s\n", resp->raw_response);
}

char* wb_http_resp_to_str(wb_http_resp_t* resp) {
    // @TODO: make sure that the response size make sense; this is hardcoded for
    // testing purposes
    size_t resp_size = 1024 * 16 * sizeof(char);
    char* resp_str = malloc(resp_size);
    if (!resp_str) {
        perror("failed to allocate memory for http respnse");
        return NULL;
    }
    resp_str[0] = '\0';
    strcat(resp_str, resp->raw_response);
    return resp_str;
}