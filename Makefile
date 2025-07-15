server:
	clang server.c -Wall -Werror -Wextra -g -gdwarf-4 -o server

server-run: server
	./server

backend-run:
	fastapi dev dummy_backend.py

.PHONY: server server-run