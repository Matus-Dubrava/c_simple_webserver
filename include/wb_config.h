#ifndef WB_CONFIG_H
#define WB_CONFIG_H

#include "stdlib.h"
#include "stdbool.h"
#include "wb_definitions.h"

typedef struct wbc_pr_t {
    char* path;
    char* target_ip;
    int target_port;
    char* target_path;
} wbc_pr_t;

typedef struct wbc_t {
    wbc_pr_t** pass_rules;
    size_t pass_rules_len;
    size_t pass_rules_capacity;
} wbc_t;

wbc_pr_t* wbc_pr_create(const char* path,
                        const char* target_ip,
                        const char* target_port,
                        const char* target_path);

void wbc_pr_destroy(wbc_pr_t* pass_rule);

void wbc_pr_display(wbc_pr_t* pass_rule);

void wbc_destroy(wbc_t* conf);

wbc_t* wbc_parse_file(char* filepath);

void wbc_display(wbc_t* conf);

bool wbc_validate(wbc_t* conf);

#endif