#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <limits.h>
#include <string.h>

#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "timespec_util.h"

#define DEFAULT_PORT "4000"
#define RECV_BUFFER_SIZE 512

char*** read_server_ipAddrPorts(char* filename, size_t serverCount) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) exit(1);

    char*** ipAddrPorts = (char***) malloc(serverCount * sizeof(char***));
    if (!ipAddrPorts) exit(1);

    for (int i = 0; i < serverCount; ++i) {
        ipAddrPorts[i] = NULL;
        char* line = NULL;
        size_t n = 0;
        ssize_t read = getline(&line, &n, fp);
        if (read <= 0) {
            free(line);
            continue;
        }
        char* ip_addr = strtok(line, " :\t\n\0");
        if (ip_addr == NULL) continue;
        char* port = strtok(NULL, " :\t\n\0");
        if (port == NULL) port = DEFAULT_PORT;

        ipAddrPorts[i] = (char**) malloc(2 * sizeof(char*)); // ipAddrPorts[i] = {ip_addr, port}
        ipAddrPorts[i][0] = strdup(ip_addr);
        ipAddrPorts[i][1] = strdup(port);

        free(line);
    }

    fclose(fp);

    return ipAddrPorts;
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

struct rpc_result {
    struct timespec send_timestamp, comp_timestamp;
    size_t num_bytes_recv;
};

struct rpc_argv {
    pthread_t thread_id;
    size_t thread_num;
    char *ip_addr, *port;
    size_t delay, fileSize;
    struct rpc_result result;
};

void* virtual_rpc(void *argv) {
    size_t thread_num = ((struct rpc_argv*) argv)->thread_num;
    char* ip_addr = ((struct rpc_argv*) argv)->ip_addr;
    char* port = ((struct rpc_argv*) argv)->port;
    size_t delay = ((struct rpc_argv*) argv)->delay;
    size_t fileSize = ((struct rpc_argv*) argv)->fileSize;

    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status;
    if ((status = getaddrinfo(ip_addr, port, &hints, &servinfo))) {
        fprintf(stderr, "[thread %ld] [%s:%s] getaddrinfo error: %s\n", thread_num, ip_addr, port, gai_strerror(status));
        exit(1);
    }

    int sktfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sktfd == -1) {
        fprintf(stderr, "[thread %ld] [%s:%s] socket error: %s (%d)\n", thread_num, ip_addr, port, strerror(errno), errno);
        exit(1);
    }

    if (connect(sktfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        close(sktfd);
        fprintf(stderr, "[thread %ld] [%s:%s] connect error: %s (%d)\n", thread_num, ip_addr, port, strerror(errno), errno);
        exit(1);
    }

    char addr[INET6_ADDRSTRLEN];
    inet_ntop(servinfo->ai_family, get_in_addr((struct sockaddr*) servinfo->ai_addr), addr, sizeof(addr));
    printf("[thread %ld] [%s:%s] connecting to %s\n", thread_num, ip_addr, port, addr);
    freeaddrinfo(servinfo);

    struct rpc_result* res = &((struct rpc_argv*) argv)->result;

    // char msg[] = "/homes/gws/liangyu/CSE550-HW/HW1/partb/test.txt\n";
    char msg[64];
    sprintf(msg, "%ld %ld", delay, fileSize);
    size_t num_bytes_sent = 0;
    size_t length = strlen(msg) + 1;
    msg[length - 1] = EOF;
    while(num_bytes_sent < length) {
        ssize_t numbytes;
        if ((numbytes = send(sktfd, msg + num_bytes_sent, length - num_bytes_sent, 0)) == -1) {
            fprintf(stderr, "[thread %ld] [%s:%s] send error: %s (%d)\n", thread_num, ip_addr, port, strerror(errno), errno);
            exit(1);
        }
        num_bytes_sent += numbytes;
    }
    struct timespec begin = timespec_now();

    char buf[RECV_BUFFER_SIZE];
    ssize_t num_bytes_recv = 0;
    for (;;) {
        ssize_t numbytes;
        if ((numbytes = recv(sktfd, buf, RECV_BUFFER_SIZE, 0)) == -1) {
            fprintf(stderr, "[thread %ld] [%s:%s] recv error: %s (%d)\n", thread_num, ip_addr, port, strerror(errno), errno);
            exit(1);
        }
        num_bytes_recv += numbytes;
        if (numbytes == 0) break;
    }
    struct timespec end = timespec_now();

    char begin_buf[64];
    char end_buf[64];
    char diff_buf[64];
    timespec_print(begin, begin_buf);
    timespec_print(end, end_buf);
    timespec_print_diff(timespec_diff(begin, end), diff_buf);

    printf("[thread %ld] [%s:%s] rpc completed, send_timestamp=%s, comp_timestamp=%s, time_elapsed=%s, num_bytes_recv=%ld\n", 
        thread_num, ip_addr, port, begin_buf, end_buf, diff_buf, num_bytes_recv);
    res->send_timestamp = begin;
    res->comp_timestamp = end;
    res->num_bytes_recv = num_bytes_recv;

    close(sktfd);

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        fprintf(stderr, "usage: ./client [hostname file] [server count] [server delay] [server file size] [rpc launch interval]\n");
        exit(1);
    }

    char* filename = argv[1];
    size_t serverCount = atol(argv[2]);
    size_t serverDelay = atol(argv[3]);
    size_t serverFileSize = atol(argv[4]);
    size_t launchInterval = atol(argv[5]);
    
    char*** ipAddrPorts = read_server_ipAddrPorts(filename, serverCount);
    for (size_t i = 0; i < serverCount; ++i) {
        if (ipAddrPorts[i] == NULL) {
            serverCount = i;
            break;
        }
    }

    struct rpc_argv* thread_argvs = (struct rpc_argv*) malloc(serverCount * sizeof(struct rpc_argv));
    for (size_t i = 0; i < serverCount; ++i) {
        thread_argvs[i] = (struct rpc_argv) {
            .thread_num = i,
            .ip_addr = ipAddrPorts[i][0],
            .port = ipAddrPorts[i][1],
            .delay = serverDelay,
            .fileSize = serverFileSize
        };
    }

    for (size_t i = 0; i < serverCount; ++i) {
        unsigned int sleepTime = launchInterval;
        while(sleepTime) {
            sleepTime = sleep(sleepTime);
        }

        int status;
        if ((status = pthread_create(&thread_argvs[i].thread_id, NULL, virtual_rpc, thread_argvs + i)) != 0) {
            fprintf(stderr, "[main] thread %ld creation failed: %d\n", i, status);
            exit(1);
        }
    }

    printf("[main] %ld number of threads created\n", serverCount);

    for (size_t i = 0; i < serverCount; ++i) {
        int status;
        if ((status = pthread_join(thread_argvs[i].thread_id, NULL)) != 0) {
            fprintf(stderr, "[main] thread %ld join failed: %d\n", i, status);
            exit(1);
        }
    }

    struct timespec earliest_send = timespec_now();
    struct timespec latest_comp = (struct timespec) {.tv_sec = 0, .tv_nsec = 0};
    size_t total_bytes_recv = 0;
    for (size_t i = 0; i < serverCount; ++i) {
        struct timespec send = thread_argvs[i].result.send_timestamp;
        struct timespec comp = thread_argvs[i].result.comp_timestamp;
        earliest_send = timespec_less(send, earliest_send) ? send : earliest_send;
        latest_comp = timespec_less(latest_comp, comp) ? comp : latest_comp;
        total_bytes_recv += thread_argvs[i].result.num_bytes_recv;
    }

    char begin_buf[64];
    char end_buf[64];
    char diff_buf[64];
    timespec_print(earliest_send, begin_buf);
    timespec_print(latest_comp, end_buf);
    timespec_print_diff(timespec_diff(earliest_send, latest_comp), diff_buf);
    printf("[main] rpc completed, earliest_send=%s, latest_comp=%s, time_elapsed=%s, total_bytes_recv=%ld\n",
        begin_buf, end_buf, diff_buf, total_bytes_recv);

    return 0;
}