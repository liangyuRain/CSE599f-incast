#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include "timespec_util.h"

#define BACKLOG 64
#define SEND_TIMEOUT 10 // sec

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

// establish listening to given IP address and port
int get_listen_fd(char *ip_address, char *port) {
    for (size_t retry = 0; retry <= 5; ++retry) {
        if (retry > 0) {
            sleep(1);
            printf("[main] %lu retry to listen to port %s at %s\n", retry, port, ip_address);
        }

        struct addrinfo hints;
        struct addrinfo *servinfo;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        int status;
        if ((status = getaddrinfo(ip_address, port, &hints, &servinfo))) {
            fprintf(stderr, "[main] getaddrinfo error: %s\n", gai_strerror(status));
            continue;
        }

        int listen_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
        if (listen_fd == -1) {
            fprintf(stderr, "[main] socket error: %s (%d)\n", strerror(errno), errno);
            continue;
        }

        if (bind(listen_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
            fprintf(stderr, "[main] bind error: %s (%d)\n", strerror(errno), errno);
            continue;
        }

        if (listen(listen_fd, BACKLOG) == -1) {
            fprintf(stderr, "[main] listen error: %s (%d)\n", strerror(errno), errno);
            continue;
        }

        freeaddrinfo(servinfo);

        printf("[main] listening to port %s at %s\n", port, ip_address);
        return listen_fd;
    }
    return -1;
}

struct connection {
    pthread_t thread_id;
    size_t thread_num;
    int sktfd;
};

void* handle_connection(void *argv) {
    size_t thread_num = ((struct connection*) argv)->thread_num;
    int sktfd = ((struct connection*) argv)->sktfd;

    char* send_buf = NULL;

    for(;;) {
        char recv_buf[64];
        ssize_t num_bytes_recv = 0;
        for(;;) {
            ssize_t numbytes = recv(sktfd, recv_buf + num_bytes_recv, 64 - num_bytes_recv, 0);
            if (numbytes == -1) {
                fprintf(stderr, "[thread %ld] recv error: %s (%d)\n", thread_num, strerror(errno), errno);
                goto close_fd;
            } else if (numbytes == 0) {
                fprintf(stderr, "[thread %ld] connection closed\n", thread_num);
                goto close_fd;
            }
            num_bytes_recv += numbytes;
            recv_buf[num_bytes_recv] = '\0';
            char *newline_ch = strchr(recv_buf, '\n');
            if (newline_ch) {
                *newline_ch = '\0';
                break;
            }
        }

        printf("[thread %ld] msg received: \"%s\"\n", thread_num, recv_buf);

        char* delay_str = strtok(recv_buf, " \t\n\0");
        char* fileSize_str = strtok(NULL, " \t\n\0");
        if (delay_str == NULL || fileSize_str == NULL) {
            fprintf(stderr, "[thread %ld] illegal format \"%s\", expected \"[delay] [fileSize]\"\n", thread_num, recv_buf);
            goto close_fd;
        }
        // atol error unhandled
        size_t delay = atol(delay_str);
        size_t fileSize = atol(fileSize_str);
        printf("[thread %ld] request received: delay=%ld, fileSize=%ld\n", thread_num, delay, fileSize);

        sleepforus(delay);

        send_buf = (char*) malloc(fileSize * sizeof(char));
        for (size_t i = 0; i < fileSize - 2; ++i) {
            char c = (char) (i % 251u);
            if (c == '\n') c = (char) 252u;
            send_buf[i] = c;
        }
        send_buf[fileSize - 1] = '\n';
        ssize_t num_bytes_sent = 0;
        ssize_t last_num_bytes = -1;
        while(num_bytes_sent < fileSize) {
            ssize_t numbytes;
            if ((numbytes = send(sktfd, send_buf + num_bytes_sent, fileSize - num_bytes_sent, 0)) < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    printf("[thread %ld] send timeout (%d sec) triggered: %s (%d); %ld bytes sent; Continue send.\n", thread_num, SEND_TIMEOUT, strerror(errno), errno, num_bytes_sent);
                    if (last_num_bytes > 0) {
                        char str[256];
                        size_t begin = 0;
                        begin += sprintf(&str[begin], "[thread %ld] last send has %ld bytes: \"", thread_num, last_num_bytes);
                        for (size_t i = (10 <= last_num_bytes ? 10 : last_num_bytes); i > 0; --i) {
                            begin += sprintf(&str[begin], "%02hhx ", send_buf[num_bytes_sent - i]);
                        }
                        --begin; // remove the last space
                        begin += sprintf(&str[begin], "\" (display up to 10 tail bytes)\n");
                        printf("%s", str);
                    }
                    continue;
                } else if (errno == ETIMEDOUT) {
                    printf("[thread %ld] ETIMEDOUT triggered: %s (%d); %ld bytes sent; Closing connection.\n", thread_num, strerror(errno), errno, num_bytes_sent);
                    goto close_fd;
                } else {
                    fprintf(stderr, "[thread %ld] send error: %s (%d), numbytes = %ld\n", thread_num, strerror(errno), errno, numbytes);
                    exit(1);
                }
            } else {
                num_bytes_sent += numbytes;
                last_num_bytes = numbytes;
                printf("[thread %ld] numbytes = %ld, total %ld bytes sent.\n", thread_num, numbytes, num_bytes_sent);
            }
        }
        printf("[thread %ld] send finished. %ld bytes sent.\n", thread_num, num_bytes_sent);
        free(send_buf);
        send_buf = NULL;
    }

close_fd:
    close(sktfd);
    if (send_buf != NULL) free(send_buf);
    free(argv);

    printf("[thread %ld] connection closed.\n", thread_num);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: ./server [IP Address] [Port]\n");
        return 1;
    }

    // setup listening fd
    int listen_fd = get_listen_fd(argv[1], argv[2]);
    if (listen_fd < 0) {
        fprintf(stderr, "[main] get listen fd failed\n");
        exit(1);
    }

    size_t thread_num = 0;
    
    for(;;) {
        struct sockaddr_storage foreign_addr;
        socklen_t foreign_addr_size;
        char ip_address[INET6_ADDRSTRLEN];

        printf("[main] waiting for incomming connection\n");

        foreign_addr_size = sizeof(foreign_addr);
        int sktfd = accept(listen_fd, (struct sockaddr *) &foreign_addr, &foreign_addr_size);

        if (sktfd == -1) {
            fprintf(stderr, "[main] accept error: %s (%d)\n", strerror(errno), errno);
            continue;
        }

        inet_ntop(foreign_addr.ss_family,
                  get_in_addr((struct sockaddr *) &foreign_addr),
                  ip_address,
                  sizeof(ip_address));
        printf("[main] got connection from %s\n", ip_address);

        // set send timeout to be 10 sec
        struct timeval tv = (struct timeval) {
            .tv_sec = SEND_TIMEOUT,
            .tv_usec = 0
        };
        setsockopt(sktfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

        struct connection* conn = (struct connection*) malloc(sizeof(struct connection));
        conn->thread_num = thread_num++;
        conn->sktfd = sktfd;

        int status;
        if ((status = pthread_create(&conn->thread_id, NULL, handle_connection, conn)) != 0) {
            fprintf(stderr, "[main] thread %ld creation failed: %d\n", conn->thread_num, status);
            exit(1);
        }
    }
}
