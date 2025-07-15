#include "wb_config.h"
#include "stdbool.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

wbc_pr_t* wbc_pr_create(const char* path,
                        const char* target_ip,
                        const char* target_port,
                        const char* target_path) {
    wbc_pr_t* pass_rule = malloc(sizeof(wbc_pr_t));
    if (!pass_rule) {
        perror("failed to allocate memory for pass rule");
        return NULL;
    }

    pass_rule->target_port = atoi(target_port);
    if (pass_rule->target_port == 0) {
        fprintf(stderr, "failed to convert port %s to int\n", target_port);
        free(pass_rule);
        return NULL;
    }

    pass_rule->path = strdup(path);
    pass_rule->target_ip = strdup(target_ip);
    pass_rule->target_path = strdup(target_path);
    return pass_rule;
}

void wbc_pr_destroy(wbc_pr_t* pass_rule) {
    free(pass_rule->path);
    free(pass_rule->target_path);
    free(pass_rule->target_ip);
    free(pass_rule);
}

void wbc_pr_display(wbc_pr_t* pass_rule) {
    printf(
        "pass rule: path=%s, target_path: %s, target_ip=%s, target_port=%d\n",
        pass_rule->path, pass_rule->target_path, pass_rule->target_ip,
        pass_rule->target_port);
}

void wbc_destroy(wbc_t* conf) {
    for (size_t i = 0; i < conf->pass_rules_len; ++i) {
        wbc_pr_destroy(conf->pass_rules[i]);
    }

    free(conf->pass_rules);
    free(conf);
}

wbc_t* wbc_parse_file(char* filepath) {
    wbc_t* conf = malloc(sizeof(wbc_t));
    if (!conf) {
        perror("failed to allocate memory for wb config");
        return NULL;
    }

    wbc_pr_t** pass_rules =
        malloc(WB_CONF_PASS_RULE_MAX_SIZE * sizeof(wbc_pr_t*));
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

    bool all_valid = true;
    char line[1024];
    while (fgets(line, 1024, f) != NULL) {
        char* line_cpy = strdup(line);
        char* rule = strtok(line, " ");

        if (strncmp(rule, "pass", 4) == 0) {
            char* path = strtok(NULL, " ");
            char* target_ip = strtok(NULL, ":");
            char* target_port = strtok(NULL, " ");
            char* target_path = strtok(NULL, "\n");

            if (!path || !target_ip || !target_port || !target_path) {
                fprintf(stderr, "invalid pass rule found: %s\n", line_cpy);
                all_valid = false;
                free(line_cpy);
                break;
            }

            printf(
                "rule: %s, path: %s, target_path: %s, target_ip: %s, "
                "target_target_port: %s\n",
                rule, path, target_path, target_ip, target_port);
            wbc_pr_t* pass_rule =
                wbc_pr_create(path, target_ip, target_port, target_path);
            if (!pass_rule) {
                all_valid = false;
                free(line_cpy);
                break;
            } else {
                conf->pass_rules[conf->pass_rules_len++] = pass_rule;
            }
        }

        free(line_cpy);
    }

    if (!all_valid) {
        wbc_destroy(conf);
        return NULL;
    }

    return conf;
}

void wbc_display(wbc_t* conf) {
    printf("config: len=%zu, capacity=%zu\n", conf->pass_rules_len,
           conf->pass_rules_capacity);
    for (size_t i = 0; i < conf->pass_rules_len; ++i) {
        wbc_pr_display(conf->pass_rules[i]);
    }
}

bool wbc_validate(wbc_t* conf) {
    // validate pass rules; make sure there is only one pass rule for given path
    bool is_valid = true;

    for (size_t i = 0; i < conf->pass_rules_len && is_valid; ++i) {
        for (size_t j = 0; j < conf->pass_rules_len && is_valid; ++j) {
            if (i != j && strcmp(conf->pass_rules[i]->path,
                                 conf->pass_rules[j]->path) == 0) {
                fprintf(stderr, "found duplicate pass rule path: %s\n",
                        conf->pass_rules[i]->path);
                is_valid = false;
            }
        }
    }

    return is_valid;
}