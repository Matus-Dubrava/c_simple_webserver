#include "wb_config.h"
#include "assert.h"
#include "string.h"
#include "stdbool.h"

void test_pass_rules() {
    wbc_t* conf = wbc_parse_file("tests/data/configs/config_pass_rules_1");
    assert(conf);

    assert(conf->pass_rules_len == 2);

    size_t len = 2;
    char* ips[] = {"127.0.0.1", "127.0.0.1"};
    char* paths[] = {"/api", "/another"};
    int ports[] = {8000, 9000};
    char* target_paths[] = {"/", "/api"};

    for (size_t i = 0; i < len; ++i) {
        assert(strncmp(conf->pass_rules[i]->target_ip, ips[i],
                       strlen(ips[i])) == 0);
        assert(strncmp(conf->pass_rules[i]->path, paths[i], strlen(paths[i])) ==
               0);
        assert(conf->pass_rules[i]->target_port == ports[i]);
        assert(strncmp(conf->pass_rules[i]->target_path, target_paths[i],
                       strlen(target_paths[i])) == 0);
    }

    assert(wbc_validate(conf));

    wbc_destroy(conf);
}

void test_duplicate_pass_rules() {
    wbc_t* conf =
        wbc_parse_file("tests/data/configs/config_duplicate_pass_rules");
    assert(conf);

    assert(conf->pass_rules_len == 2);

    size_t len = 2;
    char* ips[] = {"127.0.0.1", "127.0.0.1"};
    char* paths[] = {"/api", "/api"};
    int ports[] = {8000, 9000};
    char* target_paths[] = {"/", "/"};

    for (size_t i = 0; i < len; ++i) {
        assert(strncmp(conf->pass_rules[i]->target_ip, ips[i],
                       strlen(ips[i])) == 0);
        assert(strncmp(conf->pass_rules[i]->path, paths[i], strlen(paths[i])) ==
               0);
        assert(conf->pass_rules[i]->target_port == ports[i]);
        assert(strncmp(conf->pass_rules[i]->target_path, target_paths[i],
                       strlen(target_paths[i])) == 0);
    }

    assert(wbc_validate(conf) == false);

    wbc_destroy(conf);
}

void test_invalid_pass_rules() {
    wbc_t* conf =
        wbc_parse_file("tests/data/configs/config_invalid_pass_rules_1");
    assert(!conf);

    conf = wbc_parse_file("tests/data/configs/config_invalid_pass_rules_2");
    assert(!conf);
}

int main() {
    test_pass_rules();
    test_duplicate_pass_rules();
    test_invalid_pass_rules();
}