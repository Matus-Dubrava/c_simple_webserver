CC = clang
CFLAGS = -Wall -Wextra -Werror -g -gdwarf-4
SRC = src/server.c
BUILD_DIR = build
OUT = $(BUILD_DIR)/server
INCLUDE = -Iinclude

server:
	mkdir -p $(BUILD_DIR)
	$(CC) $(SRC) $(CFLAGS) $(INCLUDE) -o $(OUT)

server-run: server
	./$(OUT)

backend-run:
	fastapi dev dummy_backend.py

.PHONY: server server-run