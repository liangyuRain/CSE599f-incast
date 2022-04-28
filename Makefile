TARGET := client

client: client.c
	gcc -Wall -o ${TARGET} client.c -pthread
clean:
	[ -f ${TARGET} ] && rm ${TARGET} || echo "${TARGET} not found"