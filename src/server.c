#include "stdio.h"
#include "signal.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "unistd.h"
#include "time.h"
#include "wb_config.h"
#include "wb_definitions.h"
#include "wb_http_request.h"

int init_sockaddr_in(struct sockaddr_in* addr, const char* ip, int port) {
    memset(addr, 0, sizeof(*addr));
    addr->sin_port = htons(port);
    addr->sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
        perror("invalid upstream IP address");
        return -1;
    }
    return 0;
}

int wb_send_payload(int socket, char* payload) {
    size_t total_len = strlen(payload);
    size_t total_sent = 0;

    while (total_sent < total_len) {
        ssize_t sent = send(socket, payload + total_sent, strlen(payload), 0);

        if (sent < 0) {
            perror("failed to send payload");
            break;
        }

        total_sent += sent;
    }

    if (total_sent == total_len) {
        printf("sent full payload (%zu bytes)\n", total_sent);
        return total_sent;
    } else {
        fprintf(stderr, "failed to send full payload (sent only %zu bytes)\n",
                total_sent);
        return -1;
    }
}

char* send_upstream(struct sockaddr_in* addr,
                    char* payload,
                    size_t max_retries,
                    size_t retry_wait_seconds) {
    struct timespec wait_time;
    wait_time.tv_nsec = 0;
    wait_time.tv_sec = retry_wait_seconds;

    size_t attempt = 0;
    for (;;) {
        bool is_request_successful = true;

        int u_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (u_sock == -1) {
            perror("failed to create upstream socket");
            return NULL;
        }

        if (connect(u_sock, (struct sockaddr*)addr, sizeof(*addr)) == -1) {
            perror("failed to connect upstream");
            return NULL;
        }

        int sent = wb_send_payload(u_sock, payload);
        if (sent >= 0) {
            char* res = malloc(MAX_RESP_SIZE);
            if (!res) {
                perror("failed to allocate memory for upstream response");
                return NULL;
            }
            int rec = recv(u_sock, res, MAX_RESP_SIZE, 0);
            if (rec >= 0) {
                return res;
            } else {
                is_request_successful = false;
                perror("failed to receive response from usptream service");
            }
        } else {
            is_request_successful = false;
            fprintf(stderr, "failed to send payload to upstream service");
        }

        if (!is_request_successful) {
            if (attempt < max_retries) {
                printf("retrying in %zu seconds (attemt %zu/%zu)\n",
                       retry_wait_seconds, attempt, max_retries);
                attempt++;
                nanosleep(&wait_time, NULL);
            } else {
                fprintf(stderr, "failed to resolve issues after %zu retries\n",
                        max_retries);
                return NULL;
            }
        }
    }
}

char* create_500_response() {
    char* resp = malloc(1024 * sizeof(char));
    if (!resp) {
        perror("failed to allocate memory for response");
        return NULL;
    }

    char* body = "<body><p><b>500 Interanl Server Error</b></p><body>";
    char content_len_header[1024];
    content_len_header[0] = '\0';
    sprintf(content_len_header, "Content-Length: %zu\r\n", strlen(body));

    resp[0] = '\0';
    strcat(resp, "HTTP/1.1 500 Interanl Server Error\r\n");
    strcat(resp, "Content-Type: text/html\r\n");
    strcat(resp, content_len_header);
    strcat(resp, "\r\n");
    strcat(resp, body);
    return resp;
}

int get_listening_socket(const char* ip, int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        perror("invalid IP address");
        return -1;
    }

    int l_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (l_sock == -1) {
        perror("failed to create socket");
        return -1;
    }

    int one = 1;
    if (setsockopt(l_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)) == -1) {
        perror("failed to set socket option REUSEADDR");
        return -1;
    }

    if (bind(l_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind failed");
        return -1;
    }

    if (listen(l_sock, MAX_CONN) == -1) {
        perror("listen failed");
        return -1;
    }

    return l_sock;
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    wbc_t* wb_conf = wbc_parse_file(WB_CONFIG_PATH);
    if (!wb_conf) {
        exit(EXIT_FAILURE);
    }
    wbc_validate(wb_conf);
    wbc_display(wb_conf);

    const char* ip = "127.0.0.1";
    const int port = 8080;

    struct sockaddr_in upstream_addr;
    init_sockaddr_in(&upstream_addr, UPSTREAM_IP, UPSTREAM_PORT);

    int l_sock = get_listening_socket(ip, port);
    if (l_sock == -1) {
        exit(EXIT_FAILURE);
    }

    for (;;) {
        int c_sock = accept(l_sock, NULL, NULL);
        if (c_sock == -1) {
            perror("accept failed");
        }

        char buf[MAX_REQ_SIZE];
        ssize_t received = recv(c_sock, buf, MAX_REQ_SIZE - 1, 0);
        if (received > 0) {
            buf[received] = '\0';
            printf("received:\n%s\n", buf);
            wb_http_req_t* req = wb_http_req_parse(buf);
            wb_http_req_display(req);

            req->path = "/";
            char* req_str = wb_http_req_to_str(req);

            char* u_resp =
                send_upstream(&upstream_addr, req_str, DEFAULT_MAX_RETRIES,
                              DEFAULT_RETRY_WAIT_SECONDS);
            if (!u_resp) {
                char* resp_500 = create_500_response();
                wb_send_payload(c_sock, resp_500);
                free(resp_500);
            } else {
                wb_send_payload(c_sock, u_resp);
            }
            free(u_resp);
            free(req);
            free(req_str);
            close(c_sock);
        } else if (received == 0) {
            printf("client disconnected");
        } else {
            perror("failed to receive request from client\n");
        }
    }

    wbc_destroy(wb_conf);
}