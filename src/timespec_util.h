#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#define SEC_TO_NS 1000000000
#define US_TO_NS 1000
#define TIME_UTC_TO_PST (-7 * 3600)

struct timespec timespec_now() {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    ts.tv_sec += TIME_UTC_TO_PST;
    return ts;
}

struct timespec timespec_diff(struct timespec a, struct timespec b) {
    struct timespec diff;
    diff.tv_sec = b.tv_sec - a.tv_sec;
    diff.tv_nsec = b.tv_nsec - a.tv_nsec;
    if (diff.tv_nsec < 0) {
        diff.tv_sec -= 1;
        diff.tv_nsec += 1000000000l;
    }
    return diff;
}

void timespec_print(struct timespec a, char* buf) {
    assert(a.tv_sec >= 0);
    assert(a.tv_nsec >= 0);
    long hr = a.tv_sec / 3600 % 24;
    long min = a.tv_sec / 60 % 60;
    long sec = a.tv_sec % 60;
    long ms = a.tv_nsec / 1000000;
    long us = a.tv_nsec / 1000 % 1000;
    long ns = a.tv_nsec % 1000;
    sprintf(buf, "%02ld:%02ld:%02ld %ldms %ldus %ldns", hr, min, sec, ms, us, ns);
}

void timespec_print_diff(struct timespec a, char* buf) {
    assert(a.tv_sec >= 0);
    assert(a.tv_nsec >= 0);
    long sec = a.tv_sec;
    long ms = a.tv_nsec / 1000000;
    long us = a.tv_nsec / 1000 % 1000;
    long ns = a.tv_nsec % 1000;
    sprintf(buf, "%lds %ldms %ldus %ldns", sec, ms, us, ns);
}

bool timespec_less(struct timespec a, struct timespec b) {
    if (a.tv_sec < b.tv_sec) return true;
    if (a.tv_sec == b.tv_sec && a.tv_nsec < b.tv_nsec) return true;
    return false;
}

struct timespec timespec_avg(struct timespec *results, size_t count) {
    double fnsec = 0.0;
    for (size_t i = 0; i < count; ++i) {
        assert(results[i].tv_sec >= 0);
        assert(results[i].tv_nsec >= 0);
        fnsec += results[i].tv_sec * SEC_TO_NS;
        fnsec += results[i].tv_nsec;
    }
    fnsec /= count;
    return (struct timespec) {
        .tv_sec = floor(fnsec / SEC_TO_NS),
        .tv_nsec = round(fmod(fnsec, SEC_TO_NS))
    };
}

void sleepforus(size_t time) {
    struct timespec req = (struct timespec) {
        .tv_sec = time * US_TO_NS / SEC_TO_NS,
        .tv_nsec = time * US_TO_NS % SEC_TO_NS
    };
    while(req.tv_sec > 0 || req.tv_nsec > 0) {
        struct timespec rem = (struct timespec) {
            .tv_sec = 0,
            .tv_nsec = 0
        };
        nanosleep(&req, &rem);
        req = rem;
    }
}