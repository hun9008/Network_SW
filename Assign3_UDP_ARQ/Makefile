CC = gcc
TARGETS = server_test client_test

all: $(TARGETS)

server_test: UDP_ARQ_server_test.c
	$(CC) -o server_test UDP_ARQ_server_test.c

client_test: UDP_ARQ_client_test.c
	$(CC) -o client_test UDP_ARQ_client_test.c

server: UDP_ARQ_server.c
	$(CC) -o server UDP_ARQ_server.c

client: UPD_ARQ_client.c
	$(CC) -o client UDP_ARQ_client.c


run_tests: all
	@chmod +x test_script.sh
	@./test_script.sh

clean:
	rm -f $(TARGETS)