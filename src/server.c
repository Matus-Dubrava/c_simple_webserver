#include "stdio.h"
#include "semaphore.h"
#include "pthread.h"
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
#include "wb_queue.h"
#include "wb_definitions.h"
#include "wb_http_request.h"
#include "wb_http_response.h"

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

        // TODO: retry connection to upstream service
        if (connect(u_sock, (struct sockaddr*)addr, sizeof(*addr)) == -1) {
            close(u_sock);  // TODO: when retry is implemented, remove this line
            perror("failed to connect upstream");
            return NULL;
        }

        int sent = wb_send_payload(u_sock, payload);
        if (sent >= 0) {
            char* res = malloc(MAX_RESP_SIZE);
            if (!res) {
                perror("failed to allocate memory for upstream response");
                close(u_sock);
                return NULL;
            }
            int nrec = recv(u_sock, res, MAX_RESP_SIZE, 0);
            if (nrec >= 0) {
                res[nrec] = '\0';
                close(u_sock);
                return res;
            } else {
                is_request_successful = false;
                free(res);
                perror("failed to receive response from upstream service");
            }
        } else {
            is_request_successful = false;
            fprintf(stderr, "failed to send payload to upstream service");
        }

        if (!is_request_successful) {
            close(u_sock);
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

wb_http_resp_t* wb_proxy_pass_request(wb_http_req_t* req,
                                      const wbc_t* wb_config) {
    size_t pass_rule_idx = 0;
    bool found = false;

    // TODO: this should not be linear array search
    for (size_t i = 0; i < wb_config->pass_rules_len; ++i) {
        if (strncmp(wb_config->pass_rules[i]->path, req->path,
                    strlen(req->path)) == 0) {
            pass_rule_idx = i;
            found = true;
            break;
        }
    }

    if (!found) {
        printf("no pass rule for path %s\n", req->path);
        return NULL;
    } else {
        struct sockaddr_in addr;
        init_sockaddr_in(&addr, wb_config->pass_rules[pass_rule_idx]->target_ip,
                         wb_config->pass_rules[pass_rule_idx]->target_port);

        // rewrite path based on provided configuration
        req->path = wb_config->pass_rules[pass_rule_idx]->target_path;
        char* req_str = wb_http_req_to_str(req);
        printf("sending request to %s:%d\n",
               wb_config->pass_rules[pass_rule_idx]->target_ip,
               wb_config->pass_rules[pass_rule_idx]->target_port);
        wb_http_req_display(req);
        char* raw_resp = send_upstream(&addr, req_str, DEFAULT_MAX_RETRIES,
                                       DEFAULT_RETRY_WAIT_SECONDS);
        wb_http_resp_t* resp = wb_http_resp_parse(raw_resp);

        free(req_str);
        return resp;
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

typedef struct wb_worker_ctx_t {
    const wbc_t* wb_conf;
    wb_queue* queue;
    sem_t* sem;
    pthread_mutex_t* mut;
} wb_worker_ctx_t;

void wb_worker_ctx_init(wb_worker_ctx_t* ctx,
                        const wbc_t* wb_conf,
                        wb_queue* queue,
                        sem_t* sem,
                        pthread_mutex_t* mut) {
    ctx->mut = mut;
    ctx->queue = queue;
    ctx->sem = sem;
    ctx->wb_conf = wb_conf;
}

void* process_http_request(void* worker_ctx) {
    wb_worker_ctx_t* ctx = (wb_worker_ctx_t*)worker_ctx;

    for (;;) {
        pthread_mutex_lock(ctx->mut);
        if (!ctx->queue->head) {
            pthread_mutex_unlock(ctx->mut);
            printf("[worker %zx] waiting for requests...\n", pthread_self());
            sem_wait(ctx->sem);
            continue;
        }

        int* c_sock = wb_queue_dequeue(ctx->queue);
        pthread_mutex_unlock(ctx->mut);

        char buf[MAX_REQ_SIZE];
        ssize_t received = recv(*c_sock, buf, MAX_REQ_SIZE - 1, 0);
        if (received > 0) {
            buf[received] = '\0';
            printf("[worker %zu] received:\n%s\n", pthread_self(), buf);
            wb_http_req_t* req = wb_http_req_parse(buf);
            wb_http_req_display(req);

            wb_http_resp_t* upstream_resp =
                wb_proxy_pass_request(req, ctx->wb_conf);
            if (!upstream_resp) {
                // this is not always 500; maybe we are just missing proxy pass
                // rule in which case we should return something else
                char* resp_500 = create_500_response();
                wb_send_payload(*c_sock, resp_500);
                free(resp_500);
            } else {
                char* resp_str = wb_http_resp_to_str(upstream_resp);
                if (!resp_str) {
                    char* resp_500 = create_500_response();
                    wb_send_payload(*c_sock, resp_500);
                    free(resp_500);
                } else {
                    wb_send_payload(*c_sock, resp_str);
                    free(resp_str);
                }
            }

            wb_http_resp_destroy(upstream_resp);
            wb_http_req_destroy(req);
        } else if (received == 0) {
            printf("client disconnected");
        } else {
            perror("failed to receive request from client\n");
        }
        close(*c_sock);
        free(c_sock);
    }
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    wbc_t* wb_conf = wbc_parse_file(WB_CONFIG_PATH);
    if (!wb_conf) {
        exit(EXIT_FAILURE);
    }
    wbc_validate(wb_conf);
    wbc_display(wb_conf);

    size_t nworkers = 5;
    wb_queue queue;
    wb_queue_init(&queue);

    sem_t sem;
    pthread_mutex_t mut;
    pthread_mutex_init(&mut, NULL);
    sem_init(&sem, 0, 0);

    wb_worker_ctx_t worker_ctx;
    wb_worker_ctx_init(&worker_ctx, wb_conf, &queue, &sem, &mut);

    const char* ip = "127.0.0.1";
    const int port = 8080;

    struct sockaddr_in upstream_addr;
    init_sockaddr_in(&upstream_addr, UPSTREAM_IP, UPSTREAM_PORT);

    int l_sock = get_listening_socket(ip, port);
    if (l_sock == -1) {
        exit(EXIT_FAILURE);
    }

    pthread_t workers[nworkers];
    for (size_t i = 0; i < nworkers; ++i) {
        if (pthread_create(&workers[i], NULL, process_http_request,
                           &worker_ctx) == -1) {
            perror("failed to create worker thread");
            exit(EXIT_FAILURE);
        }
    }

    for (;;) {
        int c_sock = accept(l_sock, NULL, NULL);
        if (c_sock == -1) {
            perror("accept failed");
        }

        pthread_mutex_lock(&mut);
        int* c_sock_ptr = malloc(sizeof(int));
        if (!c_sock_ptr) {
            perror("failed to allocate memory for client socket");
            close(c_sock);
            pthread_mutex_unlock(&mut);
            continue;
        }
        *c_sock_ptr = c_sock;

        wb_queue_enqueue(&queue, c_sock_ptr);
        sem_post(&sem);
        pthread_mutex_unlock(&mut);
    }

    for (size_t i = 0; i < nworkers; ++i) {
        if (pthread_join(workers[i], NULL) == -1) {
            perror("failed to join worker thread");
        }
    }

    wbc_destroy(wb_conf);
    pthread_mutex_destroy(&mut);
    sem_destroy(&sem);
}