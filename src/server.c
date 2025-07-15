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

typedef struct http_request_t {
    char* method;
    char* path;
    char* version;
} http_request_t;

http_request_t* wb_parse_http_request(char* raw_request) {
    http_request_t* request = malloc(sizeof(http_request_t));
    if (!request) {
        perror("failed to allocate memory for http header");
        return NULL;
    }

    char* raw_request_copy = strdup(raw_request);

    request->method = strtok(raw_request_copy, " ");
    request->path = strtok(NULL, " ");
    request->version = strtok(NULL, "\r\n");

    return request;
}

typedef struct wb_conf_pass_rule_t {
    char* path;
    char* ip;
    int port;
} wb_conf_pass_rule_t;

wb_conf_pass_rule_t* wb_create_pass_rule(char* path, char* ip, char* port) {
    wb_conf_pass_rule_t* pass_rule = malloc(sizeof(wb_conf_pass_rule_t));
    if (!pass_rule) {
        perror("failed to allocate memory for pass rule");
        return NULL;
    }

    pass_rule->port = atoi(port);
    if (pass_rule->port == 0) {
        fprintf(stderr, "failed to convert port %s to int\n", port);
        free(pass_rule);
        return NULL;
    }

    pass_rule->path = strdup(path);
    pass_rule->ip = strdup(ip);
    return pass_rule;
}

void wb_conf_pass_rule_destroy(wb_conf_pass_rule_t* pass_rule) {
    free(pass_rule->path);
    free(pass_rule->ip);
    free(pass_rule);
}

void wb_conf_pass_rule_display(wb_conf_pass_rule_t* pass_rule) {
    printf("pass rule: path=%s, ip=%s, port=%d\n", pass_rule->path,
           pass_rule->ip, pass_rule->port);
}

typedef struct wb_conf_t {
    wb_conf_pass_rule_t** pass_rules;
    size_t pass_rules_len;
    size_t pass_rules_capacity;
} wb_conf_t;

void wb_conf_destroy(wb_conf_t* conf) {
    for (size_t i = 0; i < conf->pass_rules_len; ++i) {
        wb_conf_pass_rule_destroy(conf->pass_rules[i]);
    }

    free(conf->pass_rules);
    free(conf);
}

wb_conf_t* wb_parse_config(char* filepath) {
    wb_conf_t* conf = malloc(sizeof(wb_conf_t));
    if (!conf) {
        perror("failed to allocate memory for wb config");
        return NULL;
    }

    wb_conf_pass_rule_t** pass_rules =
        malloc(WB_CONF_PASS_RULE_MAX_SIZE * sizeof(wb_conf_pass_rule_t*));
    if (!pass_rules) {
        perror("failed to allocate memory for pass rules\n");
        free(conf);
        return NULL;
    }

    conf->pass_rules_capacity = WB_CONF_PASS_RULE_MAX_SIZE;
    conf->pass_rules_len = 0;
    conf->pass_rules = pass_rules;

    FILE* f = fopen(filepath, "r");
    if (!f) {
        perror("failed to open config file");
        return NULL;
    }

    char line[1024];
    while (fgets(line, 1024, f) != NULL) {
        char* rule = strtok(line, " ");
        if (strncmp(rule, "pass", 4) == 0) {
            char* path = strtok(NULL, " ");
            char* ip = strtok(NULL, ":");
            char* port = strtok(NULL, "\n");

            printf("rule: %s, path: %s, ip: %s, port: %s\n", rule, path, ip,
                   port);
            wb_conf_pass_rule_t* pass_rule =
                wb_create_pass_rule(path, ip, port);
            if (!pass_rule) {
                wb_conf_destroy(conf);
                return NULL;
            } else {
                conf->pass_rules[conf->pass_rules_len++] = pass_rule;
            }
        }
    }

    return conf;
}

void wb_conf_display(wb_conf_t* conf) {
    printf("config: len=%zu, capacity=%zu\n", conf->pass_rules_len,
           conf->pass_rules_capacity);
    for (size_t i = 0; i < conf->pass_rules_len; ++i) {
        wb_conf_pass_rule_display(conf->pass_rules[i]);
    }
}

void wb_request_display(http_request_t* request) {
    printf("request: method=%s, path=%s, version=%s\n", request->method,
           request->path, request->version);
}

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

    wb_conf_t* wb_conf = wb_parse_config(WB_CONFIG_PATH);
    if (!wb_conf) {
        exit(EXIT_FAILURE);
    }
    wb_conf_display(wb_conf);

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
            http_request_t* request = wb_parse_http_request(buf);
            wb_request_display(request);

            char* u_resp =
                send_upstream(&upstream_addr, buf, DEFAULT_MAX_RETRIES,
                              DEFAULT_RETRY_WAIT_SECONDS);
            if (!u_resp) {
                char* resp_500 = create_500_response();
                wb_send_payload(c_sock, resp_500);
                free(resp_500);
            } else {
                wb_send_payload(c_sock, u_resp);
            }
            free(u_resp);
            free(request);
            close(c_sock);
        } else if (received == 0) {
            printf("client disconnected");
        } else {
            perror("failed to receive request from client\n");
        }
    }

    wb_conf_destroy(wb_conf);
}