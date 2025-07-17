#include "assert.h"
#include "string.h"
#include "wb_http_request.h"

void test_http_request_parsing() {
    char* req_str =
        "GET /api HTTP/1.1\r\nHost: "
        "localhost:8080\r\n\r\n<body><h1>something</h1></body>\r\n";
    wb_http_req_t* req = wb_http_req_parse(req_str);

    assert(strcmp(req->method, "GET") == 0);
    assert(strcmp(req->path, "/api") == 0);
    assert(strcmp(req->version, "HTTP/1.1") == 0);
    assert(
        strcmp(req->raw_remaining,
               "Host: "
               "localhost:8080\r\n\r\n<body><h1>something</h1></body>\r\n") ==
        0);

    wb_http_req_destroy(req);
}

int main() {
    test_http_request_parsing();
}