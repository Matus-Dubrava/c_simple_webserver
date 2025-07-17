CC = clang
CFLAGS = -Wall -Wextra -Werror -g -gdwarf-4
SRC = src/wb_config.c src/wb_http_request.c
SRC_MAIN = src/server.c 
BUILD_DIR = build
TEST_DIR = tests
OUT = $(BUILD_DIR)/server
INCLUDE = -Iinclude

OUT_TEST_CONFIG = $(BUILD_DIR)/test_wb_config
OUT_TEST_HTTP_REQ = $(BUILD_DIR)/test_wb_http_req

server:
	mkdir -p $(BUILD_DIR)
	$(CC) $(SRC) $(SRC_MAIN) $(CFLAGS) $(INCLUDE) -o $(OUT)

server-run: server
	./$(OUT)

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




.PHONY: server server-run test-config test-http-request
