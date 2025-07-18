#ifndef WB_HTTP_RESPONSE_H
#define WB_HTTP_RESPONSE_H

typedef struct wb_http_resp_t {
    char* raw_response;
} wb_http_resp_t;

wb_http_resp_t* wb_http_resp_parse(char* raw_response);

void wb_http_resp_destroy(wb_http_resp_t* resp);

void wb_http_resp_display(wb_http_resp_t* resp);

char* wb_http_resp_to_str(wb_http_resp_t* resp);

#endif