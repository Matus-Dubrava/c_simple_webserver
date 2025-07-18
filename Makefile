CC = clang
CFLAGS = -Wall -Wextra -Werror -g -gdwarf-4
SRC = src/wb_config.c src/wb_http_request.c src/wb_http_response.c src/wb_queue.c
SRC_MAIN = src/server.c 
BUILD_DIR = build
TEST_DIR = tests
OUT = $(BUILD_DIR)/server
INCLUDE = -Iinclude

OUT_TEST_CONFIG = $(BUILD_DIR)/test_wb_config
OUT_TEST_HTTP_REQ = $(BUILD_DIR)/test_wb_http_req
OUT_TEST_QUEUE = $(BUILD_DIR)/test_wb_queue

server:
	mkdir -p $(BUILD_DIR)
	$(CC) $(SRC) $(SRC_MAIN) $(CFLAGS) $(INCLUDE) -o $(OUT)

server-run: server
	./$(OUT)

server-dbg: server
	gf2 ./$(OUT)

backend-run:
	fastapi dev dummy_backend.py

test-config:
	mkdir -p $(BUILD_DIR)
	$(CC) $(SRC) $(TEST_DIR)/test_wb_config.c $(CFLAGS) $(INCLUDE) -o $(OUT_TEST_CONFIG)
	./$(OUT_TEST_CONFIG)
	valgrind --leak-check=full ./$(OUT_TEST_CONFIG)

test-http-request:
	mkdir -p $(BUILD_DIR)
	$(CC) $(SRC) $(TEST_DIR)/test_wb_http_req.c $(CFLAGS) $(INCLUDE) -o $(OUT_TEST_HTTP_REQ)
	./$(OUT_TEST_HTTP_REQ)
	valgrind --leak-check=full ./$(OUT_TEST_HTTP_REQ)


test-queue:
	mkdir -p $(BUILD_DIR)
	$(CC) $(SRC) $(TEST_DIR)/test_wb_queue.c $(CFLAGS) $(INCLUDE) -o $(OUT_TEST_QUEUE)
	./$(OUT_TEST_QUEUE)
	valgrind --leak-check=full ./$(OUT_TEST_QUEUE)

test-all: test-queue test-http-request test-config


.PHONY: server server-run test-config test-http-request server-dbg test-queue
