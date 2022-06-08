CLIENT_TARGET := client
SERVER_TARGET := server

client: src/client.c
	gcc -Wall -o ${CLIENT_TARGET} src/client.c -lm -pthread
server: src/server.c
	gcc -Wall -o ${SERVER_TARGET} src/server.c -lm -pthread
all: client server
clean:
	[ -f ${CLIENT_TARGET} ] && rm ${CLIENT_TARGET} || echo "${CLIENT_TARGET} not found"
	[ -f ${SERVER_TARGET} ] && rm ${SERVER_TARGET} || echo "${SERVER_TARGET} not found"