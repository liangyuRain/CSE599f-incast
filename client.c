#include <stdlib.h>
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
#include <sys/stat.h>

#include "timespec_util.h"

#define DEFAULT_PORT "4000"
#define ADDITIONAL_RECV_BYTES 128
#define Bps_TO_Kbps (8.0 / 1000.0)
#define RECV_TIMEOUT 5 // min

// read backend servers' IP addresses and ports from file
char*** read_server_ipAddrPorts(char* filename, size_t serverCount) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "[main] read hostnames failed: %s (%d)\n", strerror(errno), errno);
        exit(1);
    }

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
        char* ssh_host = strtok(line, " :\t\n\0");
        if (ssh_host == NULL) continue;
        char* ip_addr = strtok(NULL, " :\t\n\0");
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

struct rpc_argv {
    pthread_t thread_id;
    size_t thread_num;
    size_t exp_num;
    char *ip_addr, *port;
    size_t delay, fileSize;
    size_t launchDelay;
    struct rpc_result {
        struct timespec send_timestamp, comp_timestamp;
        struct timespec time_diff;
        size_t num_bytes_recv;
        double goodput;
    } result;
};

int tcp_connect(size_t exp_num, size_t thread_num, char *ip_addr, char *port) {
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status;
    if ((status = getaddrinfo(ip_addr, port, &hints, &servinfo))) {
        fprintf(stderr, "[thread %ld] [%s:%s] [Expt %ld] getaddrinfo error: %s\n", thread_num, ip_addr, port, exp_num, gai_strerror(status));
        exit(1);
    }

    int sktfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sktfd == -1) {
        fprintf(stderr, "[thread %ld] [%s:%s] [Expt %ld] socket error: %s (%d)\n", thread_num, ip_addr, port, exp_num, strerror(errno), errno);
        exit(1);
    }

    if (connect(sktfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        close(sktfd);
        fprintf(stderr, "[thread %ld] [%s:%s] [Expt %ld] connect error: %s (%d)\n", thread_num, ip_addr, port, exp_num, strerror(errno), errno);
        exit(1);
    }

    // set recv timeout to be 5 min
    struct timeval tv = (struct timeval) {
        .tv_sec = 60 * RECV_TIMEOUT,
        .tv_usec = 0
    };
    setsockopt(sktfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    char addr[INET6_ADDRSTRLEN];
    inet_ntop(servinfo->ai_family, get_in_addr((struct sockaddr*) servinfo->ai_addr), addr, sizeof(addr));
    printf("[thread %ld] [%s:%s] [Expt %ld] connecting to %s\n", thread_num, ip_addr, port, exp_num, addr);
    freeaddrinfo(servinfo);

    return sktfd;
}

size_t serverCount;
bool start_flag;
bool *ready_flags;
bool *finish_flags;

// simulate rpc
void* virtual_rpc(void *argv) {
    size_t thread_num = ((struct rpc_argv*) argv)->thread_num;
    size_t exp_num = ((struct rpc_argv*) argv)->exp_num;
    char* ip_addr = ((struct rpc_argv*) argv)->ip_addr;
    char* port = ((struct rpc_argv*) argv)->port;
    size_t delay = ((struct rpc_argv*) argv)->delay;
    size_t fileSize = ((struct rpc_argv*) argv)->fileSize;
    size_t launchDelay = ((struct rpc_argv*) argv)->launchDelay;

    int sktfd = tcp_connect(exp_num, thread_num, ip_addr, port);

    struct rpc_result* res = &((struct rpc_argv*) argv)->result;
    char *recv_buf = (char *) malloc((ADDITIONAL_RECV_BYTES + fileSize) * sizeof(char));

    printf("[thread %ld] [%s:%s] [Expt %ld] tcp connected; thread ready to go\n", thread_num, ip_addr, port, exp_num);
    ready_flags[thread_num] = true;
    while(!start_flag);

    struct timespec req = (struct timespec) {
        .tv_sec = launchDelay * US_TO_NS / SEC_TO_NS,
        .tv_nsec = launchDelay * US_TO_NS % SEC_TO_NS
    };
    while(req.tv_sec > 0 || req.tv_nsec > 0) {
        struct timespec rem = (struct timespec) {
            .tv_sec = 0,
            .tv_nsec = 0
        };
        nanosleep(&req, &rem);
        req = rem;
    }

    struct timespec begin = timespec_now();

    // send msg to backend server
    // char msg[] = "/homes/gws/liangyu/CSE550-HW/HW1/partb/test.txt\n";
    char msg[64];
    sprintf(msg, "%ld %ld\n", delay, fileSize);
    size_t num_bytes_sent = 0;
    size_t length = strlen(msg) + 1;
    msg[length - 1] = EOF;
    while(num_bytes_sent < length) {
        ssize_t numbytes;
        if ((numbytes = send(sktfd, msg + num_bytes_sent, length - num_bytes_sent, 0)) == -1) {
            fprintf(stderr, "[thread %ld] [%s:%s] [Expt %ld] send error: %s (%d)\n", thread_num, ip_addr, port, exp_num, strerror(errno), errno);
            exit(1);
        }
        num_bytes_sent += numbytes;
    }

    // receive reply from backend server
    ssize_t num_bytes_recv = 0;
    ssize_t last_num_bytes = -1;
    for (;;) {
        ssize_t numbytes;
        if ((numbytes = recv(sktfd, recv_buf, ADDITIONAL_RECV_BYTES + fileSize, 0)) <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("[thread %ld] [%s:%s] [Expt %ld] recv timeout (%d min) triggered. %ld bytes received. Continue recv.\n", thread_num, ip_addr, port, exp_num, RECV_TIMEOUT, num_bytes_recv);
                if (last_num_bytes > 0) {
                    char str[256];
                    size_t begin = 0;
                    begin += sprintf(&str[begin], "[thread %ld] [%s:%s] [Expt %ld] last recv has %ld bytes: \"", thread_num, ip_addr, port, exp_num, last_num_bytes);
                    for (size_t i = 0; i < (10 <= last_num_bytes ? 10 : last_num_bytes); ++i) {
                        begin += sprintf(&str[begin], "%02x ", recv_buf[i]);
                    }
                    --begin; // remove the last space
                    begin += sprintf(&str[begin], "\" (display up to 10 tail bytes)\n");
                    printf("%s", str);
                }
                continue;
            } else {
                fprintf(stderr, "[thread %ld] [%s:%s] [Expt %ld] recv error: %s (%d), numbytes = %ld\n", thread_num, ip_addr, port, exp_num, strerror(errno), errno, numbytes);
                exit(1);
            }
        } else {
            num_bytes_recv += numbytes;
            last_num_bytes = numbytes;
            if (recv_buf[numbytes - 1] == '\n') break;
        }
    }
    struct timespec end = timespec_now();
    struct timespec diff = timespec_diff(begin, end);
    double goodput = num_bytes_recv * 1.0 / (diff.tv_sec * SEC_TO_NS + diff.tv_nsec - delay * US_TO_NS) * SEC_TO_NS * Bps_TO_Kbps; // Kbps
    finish_flags[thread_num] = true;

    free(recv_buf);

    // measure completion time
    char begin_buf[64];
    char end_buf[64];
    char diff_buf[64];
    timespec_print(begin, begin_buf);
    timespec_print(end, end_buf);
    timespec_print_diff(diff, diff_buf);

    printf("[thread %ld] [%s:%s] [Expt %ld] individual rpc completed:\n"
        "\tsend timestamp: %s\n"
        "\tcompletion timestamp: %s\n"
        "\ttime elapsed: %s\n"
        "\tnum_bytes_recv: %ld\n"
        "\tgoodput: %ldKbps\n",
        thread_num, ip_addr, port, exp_num, begin_buf, end_buf, diff_buf, num_bytes_recv, (size_t) round(goodput));
    res->send_timestamp = begin;
    res->comp_timestamp = end;
    res->time_diff = diff;
    res->num_bytes_recv = num_bytes_recv;
    res->goodput = goodput;

    size_t finish_count = 0;
    for (size_t i = 0; i < serverCount; ++i) {
        if (finish_flags[i]) ++finish_count;
    }
    printf("[thread %ld] [%s:%s] [Expt %ld] %ld / %ld finished\n", thread_num, ip_addr, port, exp_num, finish_count, serverCount);

    close(sktfd);

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 7) {
        fprintf(stderr, "usage: client [hostname file] [server count] [server delay (us)] " 
                        "[server file size (byte)] [rpc launch interval (us)] [num of experiments]\n");
        exit(1);
    }

    char* filename = argv[1];
    serverCount = atol(argv[2]);
    size_t serverDelay = atol(argv[3]);
    size_t serverFileSize = atol(argv[4]);
    size_t launchInterval = atol(argv[5]);
    size_t numOfExperiments = atol(argv[6]);

    printf("[main] parameters:\n"
        "\tfilename: %s\n"
        "\tserverCount: %ld\n"
        "\tserverDelay(us): %ld\n"
        "\tserverFileSize(byte): %ld\n"
        "\tlaunchInterval(us): %ld\n"
        "\tnumOfExperiments: %ld\n",
        filename, serverCount, serverDelay, serverFileSize, launchInterval, numOfExperiments);
    
    // count number of valid server hostnames
    char*** ipAddrPorts = read_server_ipAddrPorts(filename, serverCount);
    for (size_t i = 0; i < serverCount; ++i) {
        if (ipAddrPorts[i] == NULL) {
            serverCount = i;
            break;
        }
    }

    ready_flags = (bool*) malloc(serverCount * sizeof(bool));
    finish_flags = (bool*) malloc(serverCount * sizeof(bool));

    struct rpc_result *expt_res_log = (struct rpc_result*) malloc(numOfExperiments * sizeof(struct rpc_result));
    struct rpc_argv *rpc_argv_log = (struct rpc_argv*) malloc(serverCount * numOfExperiments * sizeof(struct rpc_argv));
    struct timespec *total_results = (struct timespec*) malloc(numOfExperiments * sizeof(struct timespec));
    struct timespec *indiv_results = (struct timespec*) malloc(serverCount * numOfExperiments * sizeof(struct timespec));
    double total_goodput_over_expts = 0;
    for (size_t ex = 0; ex < numOfExperiments; ++ex) {
        // build argvs for threads. Each thread run a separate rpc.
        struct rpc_argv* thread_argvs = (struct rpc_argv*) malloc(serverCount * sizeof(struct rpc_argv));
        for (size_t i = 0; i < serverCount; ++i) {
            thread_argvs[i] = (struct rpc_argv) {
                .thread_num = i,
                .exp_num = ex,
                .ip_addr = ipAddrPorts[i][0],
                .port = ipAddrPorts[i][1],
                .delay = serverDelay,
                .fileSize = serverFileSize,
                .launchDelay = launchInterval * i
            };
        }

        start_flag = false;
        for (size_t i = 0; i < serverCount; ++i) {
            ready_flags[i] = false;
            finish_flags[i] = false;
        }

        // create threads
        for (size_t i = 0; i < serverCount; ++i) {
            int status;
            if ((status = pthread_create(&thread_argvs[i].thread_id, NULL, virtual_rpc, thread_argvs + i)) != 0) {
                fprintf(stderr, "[main] [Expt %ld] thread %ld creation failed: %d\n", ex, i, status);
                exit(1);
            }
        }

        printf("[main] [Expt %ld] %ld num of threads created\n", ex, serverCount);

        for(;;) {
            bool start = true;
            for (size_t i = 0; i < serverCount; ++i) {
                start = start && ready_flags[i];
            }
            if (start) break;
        }
        start_flag = true;

        printf("[main] [Expt %ld] experiment green light\n", ex);

        // join all threads
        for (size_t i = 0; i < serverCount; ++i) {
            int status;
            if ((status = pthread_join(thread_argvs[i].thread_id, NULL)) != 0) {
                fprintf(stderr, "[main] [Expt %ld] thread %ld join failed: %d\n", ex, i, status);
                exit(1);
            }
        }

        // calculate the earliest rpc send and the latest rpc completion
        // the total time of rpc is from the earliest rpc send to the latest rpc completion
        struct timespec earliest_send = timespec_now();
        struct timespec latest_comp = (struct timespec) {.tv_sec = 0, .tv_nsec = 0};
        size_t total_bytes_recv = 0;
        for (size_t i = 0; i < serverCount; ++i) {
            struct timespec send = thread_argvs[i].result.send_timestamp;
            struct timespec comp = thread_argvs[i].result.comp_timestamp;
            indiv_results[ex * serverCount + i] = thread_argvs[i].result.time_diff;
            rpc_argv_log[ex * serverCount + i] = thread_argvs[i];

            earliest_send = timespec_less(send, earliest_send) ? send : earliest_send;
            latest_comp = timespec_less(latest_comp, comp) ? comp : latest_comp;
            total_bytes_recv += thread_argvs[i].result.num_bytes_recv;
        }

        struct timespec total_diff = timespec_diff(earliest_send, latest_comp);
        total_results[ex] = total_diff;
        double total_goodput = total_bytes_recv * 1.0 / (total_diff.tv_sec * SEC_TO_NS + total_diff.tv_nsec - serverDelay * US_TO_NS) * SEC_TO_NS * Bps_TO_Kbps; // Kbps
        total_goodput_over_expts += total_goodput;

        char begin_buf[64];
        char end_buf[64];
        char diff_buf[64];
        timespec_print(earliest_send, begin_buf);
        timespec_print(latest_comp, end_buf);
        timespec_print_diff(total_diff, diff_buf);
        printf("[main] [Expt %ld] rpc experiment completed:\n"
            "\tearliest send timestamp: %s\n"
            "\tlatest completion timestamp: %s\n"
            "\ttotal time elapsed: %s\n"
            "\ttotal num of bytes received: %ld\n"
            "\texperiment goodput: %ldKbps\n",
            ex, begin_buf, end_buf, diff_buf, total_bytes_recv, (size_t) round(total_goodput));

        expt_res_log[ex] = (struct rpc_result) {
            .send_timestamp = earliest_send,
            .comp_timestamp = latest_comp,
            .time_diff = total_diff,
            .num_bytes_recv = total_bytes_recv,
            .goodput = total_goodput
        };
        
        free(thread_argvs);
    }

    struct timespec total_avg = timespec_avg(total_results, numOfExperiments);
    struct timespec indiv_avg = timespec_avg(indiv_results, serverCount * numOfExperiments);
    size_t avg_goodput = (size_t) round(total_goodput_over_expts / numOfExperiments);
    char total_avg_buf[64];
    char indiv_avg_buf[64];
    timespec_print_diff(total_avg, total_avg_buf);
    timespec_print_diff(indiv_avg, indiv_avg_buf);
    printf("[main] %ld num of experiments completed:\n" 
           "\tAverage RPC Time Cost: %s\n"
           "\tAverage Individual RPC Time Cost: %s\n"
           "\tAverage Goodput: %ldKbps\n",
        numOfExperiments, total_avg_buf, indiv_avg_buf, avg_goodput);

    // write results to file named by current time
    struct stat st = {0};
    if (stat("expts", &st) == -1) {
        mkdir("expts", 0700);
    }

    time_t now = time(NULL);
    now += TIME_UTC_TO_PST;
    struct tm *now_tm = gmtime(&now);
    char logfile[64];
    sprintf(logfile, "expts/expt_%d_%d_%d_%d_%d.tsv",
        now_tm->tm_mon + 1, now_tm->tm_mday, now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec);
    FILE *fp = fopen(logfile, "w");
    if (fp == NULL) {
        fprintf(stderr, "[main] write experiment results failed: %s (%d)\n", strerror(errno), errno);
        exit(1);
    }

    fprintf(fp, "filename\t%s\n"
        "serverCount\t%ld\n"
        "serverDelay(us)\t%ld\n"
        "serverFileSize(byte)\t%ld\n"
        "launchInterval(us)\t%ld\n"
        "numOfExperiments\t%ld\n",
        filename, serverCount, serverDelay, serverFileSize, launchInterval, numOfExperiments);
    
    fprintf(fp, "\ntotal_rpc_avg\t%s\nindiv_rpc_avg\t%s\navg_rpc_goodput\t%ldKbps\n",
        total_avg_buf, indiv_avg_buf, avg_goodput);

    fprintf(fp, "\nexp_num\tearliest_send\tlatest_comp\ttotal_diff\ttotal_bytes_recv\ttotal_goodput\n");
    for (size_t ex = 0; ex < numOfExperiments; ++ex) {
        struct rpc_result *log = expt_res_log + ex;

        char begin_buf[64];
        char end_buf[64];
        char diff_buf[64];
        timespec_print(log->send_timestamp, begin_buf);
        timespec_print(log->comp_timestamp, end_buf);
        timespec_print_diff(log->time_diff, diff_buf);

        fprintf(fp, "%ld\t%s\t%s\t%s\t%ld\t%ldKbps\n",
            ex, begin_buf, end_buf, diff_buf, log->num_bytes_recv, (size_t) round(log->goodput));
    }

    fprintf(fp, "\nexp_num\tthread_num\tip_addr\tport\tdelay\tfileSize\tsend_timestamp\tcomp_timestamp\ttime_diff\tnum_bytes_recv\tgoodput\n");
    for (size_t i = 0; i < numOfExperiments * serverCount; ++i) {
        struct rpc_argv *log = rpc_argv_log + i;

        char begin_buf[64];
        char end_buf[64];
        char diff_buf[64];
        timespec_print(log->result.send_timestamp, begin_buf);
        timespec_print(log->result.comp_timestamp, end_buf);
        timespec_print_diff(log->result.time_diff, diff_buf);

        fprintf(fp, "%ld\t%ld\t%s\t%s\t%ld\t%ld\t%s\t%s\t%s\t%ld\t%ldKbps\n",
            log->exp_num, log->thread_num, log->ip_addr, log->port, log->delay, 
            log->fileSize, begin_buf, end_buf, diff_buf, log->result.num_bytes_recv, (size_t) round(log->result.goodput));
    }

    printf("[main] log written to %s\n", logfile);

    fclose(fp);

    return 0;
}