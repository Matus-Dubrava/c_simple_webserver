CC = clang
CFLAGS = -Wall -Wextra -Werror -g -gdwarf-4
SRC = src/server.c src/wb_config.c
SRC_TEST = src/wb_config.c
BUILD_DIR = build
TEST_DIR = tests
OUT = $(BUILD_DIR)/server
INCLUDE = -Iinclude

OUT_TEST_CONFIG = $(BUILD_DIR)/test_wb_config

server:
	mkdir -p $(BUILD_DIR)
	$(CC) $(SRC) $(CFLAGS) $(INCLUDE) -o $(OUT)

server-run: server
	./$(OUT)

backend-run:
	fastapi dev dummy_backend.py

test-config:
	mkdir -p $(BUILD_DIR)
	$(CC) $(SRC_TEST) $(TEST_DIR)/test_wb_config.c $(CFLAGS) $(INCLUDE) -o $(OUT_TEST_CONFIG)
	./$(OUT_TEST_CONFIG)
	valgrind --leak-check=full ./$(OUT_TEST_CONFIG)

.PHONY: server server-run test-config
