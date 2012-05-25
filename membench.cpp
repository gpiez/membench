/*
 * memcpybench64.cpp
 *
 *  Created on: 15.06.2009
 *      Author: gpiez
 */

#include <x86intrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <stdint.h>
#include <sys/time.h>
#include <stddef.h>
#include <vector>
#include <algorithm>

#include <sched.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <sstream>

#define CACHE_SIZE (0x600000 + 0x8000)
#define CACHE_LINE_SIZE 64

char* m1;
char* m2;
uint64_t rnd = 0x123456789abcdef;
volatile uint64_t end=0;
double cpufreq, tscfreq;
uint64_t intbs;
uint64_t cnt;
std::vector<uint64_t> times;

static inline uint64_t rdtsc() {
//	return __rdtsc();
        uint64_t tsc;
        asm volatile("cpuid\n rdtsc\n shl $32,%%rdx\n add %%rdx,%%rax\n":"=a"(tsc)::"%rdx","%rbx","%rcx");
        return tsc;
}


void loop(size_t size) {
    asm volatile("\n\
        .align  16                      \n\
    0:  add     $1, %%eax               \n\
        add     $1, %%eax               \n\
        add     $1, %%eax               \n\
        add     $1, %%eax               \n\
        add     $1, %%eax               \n\
        add     $1, %%eax               \n\
        add     $1, %%eax               \n\
        add     $1, %%eax               \n\
        add     $1, %%eax               \n\
        add     $1, %%eax               \n\
        add     $1, %%eax               \n\
        add     $1, %%eax               \n\
        add     $1, %%eax               \n\
        add     $1, %%eax               \n\
        add     $1, %%eax               \n\
        add     $1, %%eax               \n\
        dec     %0                      \n\
        jnz     0b                      \n"
        :
        : "r" (size)
        : "%eax");
}

void empty(void* src, size_t size) {
    asm volatile("\n" ::"r"(src), "r"(size): "%xmm0","%xmm1","%xmm2","%xmm3");
}

void memRead(void* src, size_t size) {
    asm volatile("\n\
        .align  16                      \n\
    0:  movdqa  (%1),   %%xmm0          \n\
        movdqa  16(%1), %%xmm1          \n\
        movdqa  32(%1), %%xmm2          \n\
        movdqa  48(%1), %%xmm3          \n\
        add     $64,    %1              \n\
        dec     %0                      \n\
        jnz     0b                      \n"
        :"+r" (size), "+r" (src)
        :
        :"%xmm0","%xmm1","%xmm2","%xmm3");
}

void memReadNT(void* src, size_t size) {
    asm volatile("\n\
        .align  16                      \n\
    0:  movntdqa  (%1),   %%xmm0          \n\
        movntdqa  16(%1), %%xmm1          \n\
        movntdqa  32(%1), %%xmm2          \n\
        movntdqa  48(%1), %%xmm3          \n\
        add     $64,    %1              \n\
        dec     %0                      \n\
        jnz     0b                      \n"
        :"+r" (size), "+r" (src)
        :
        :"%xmm0","%xmm1","%xmm2","%xmm3");
}

void memCopy(void* src, size_t size) {
    asm volatile("\n\
        .align  16                      \n\
    0:  movdqa  (%1),   %%xmm0          \n\
        movdqa  16(%1), %%xmm1          \n\
        movdqa  32(%1), %%xmm2          \n\
        movdqa  48(%1), %%xmm3          \n\
        movdqa  %%xmm0, (%1)            \n\
        movdqa  %%xmm1, 16(%1)          \n\
        movdqa  %%xmm2, 32(%1)          \n\
        movdqa  %%xmm3, 48(%1)          \n\
        add     $64,    %1              \n\
        dec     %0                      \n\
        jnz     0b                      \n"
        :"+r" (size), "+r" (src)
        :
        :"%xmm0","%xmm1","%xmm2","%xmm3");
}

void memCopy2(void* src, size_t size) {
    asm volatile("\n\
        .align  16                      \n\
    0:  movdqa  (%1),   %%xmm0          \n\
        movdqa  16(%1), %%xmm1          \n\
        movdqa  32(%1), %%xmm2          \n\
        movdqa  48(%1), %%xmm3          \n\
        movdqa  %%xmm1, 16(%1)          \n\
        movdqa  %%xmm3, 48(%1)          \n\
        movdqa  %%xmm2, (%1)          \n\
        movdqa  %%xmm0, 32(%1)            \n\
        add     $64,    %1              \n\
        dec     %0                      \n\
        jnz     0b                      \n"
        :"+r" (size), "+r" (src)
        :
        :"%xmm0","%xmm1","%xmm2","%xmm3");
}

void memCopyNT(void* src, size_t size) {
    asm volatile("\n\
        .align  16                      \n\
    0:  movntdqa  (%1),   %%xmm0          \n\
        movntdqa  16(%1), %%xmm1          \n\
        movntdqa  32(%1), %%xmm2          \n\
        movntdqa  48(%1), %%xmm3          \n\
        movntdq  %%xmm0, (%1)            \n\
        movntdq  %%xmm1, 16(%1)          \n\
        movntdq  %%xmm2, 32(%1)          \n\
        movntdq  %%xmm3, 48(%1)          \n\
        add     $64,    %1              \n\
        dec     %0                      \n\
        jnz     0b                      \n"
        :"+r" (size), "+r" (src)
        :
        :"%xmm0","%xmm1","%xmm2","%xmm3");
}
void memWrite(void* src, size_t size) {
    asm volatile("\n\
        .align  16                      \n\
    0:  movdqa  %%xmm0, (%1)            \n\
        movdqa  %%xmm1, 16(%1)          \n\
        movdqa  %%xmm2, 32(%1)          \n\
        movdqa  %%xmm3, 48(%1)          \n\
        add     $64,    %1              \n\
        dec     %0                      \n\
        jnz     0b                      \n"
        :"+r" (size), "+r" (src)
        :
        :"%xmm0","%xmm1","%xmm2","%xmm3");
}

void memWriteNT(void* src, size_t size) {
    asm volatile("\n\
        .align  16                      \n\
    0:  movntdq %%xmm0, (%1)            \n\
        movntdq %%xmm1, 16(%1)          \n\
        movntdq %%xmm2, 32(%1)          \n\
        movntdq %%xmm3, 48(%1)          \n\
        add     $64,    %1              \n\
        dec     %0                      \n\
        jnz     0b                      \n"
        :"+r" (size), "+r" (src)
        :
        :"%xmm0","%xmm1","%xmm2","%xmm3");
}

void memReadRandom(void* src, size_t size) {
    asm volatile("\n\
        mov     %1, %%rax               \n\
        .align  16                      \n\
    0:  mov     (%%rax), %%rax   \n\
        mov     (%%rax), %%rax   \n\
        mov     (%%rax), %%rax   \n\
        mov     (%%rax), %%rax   \n\
        mov     (%%rax), %%rax   \n\
        mov     (%%rax), %%rax   \n\
        mov     (%%rax), %%rax   \n\
        mov     (%%rax), %%rax   \n\
        dec     %0                      \n\
        jnz     0b                      \n"
        :"+r" (size), "+r" (src)
        :
        :"%rax");
}

void memcpyOS() {
    memcpy(m1, m2, intbs);
}

char *cachebegin;

void clrcache() {
        for (unsigned int i = 0; i < CACHE_SIZE; i += CACHE_LINE_SIZE) {
                cachebegin[i] = cachebegin[i + CACHE_LINE_SIZE / 2];
        }
}

void done() {
    end = rdtsc();
    return;
}

uint64_t benchempty(void (*func)(void*, size_t), void* mem, size_t size) {
    for (uint64_t i = 0; i < cnt; i++) {
        uint64_t t0 = rdtsc();
        func(mem, size);
        t0 = rdtsc() - t0;
        times[i] = t0;
    }
    std::sort(times.begin(), times.begin() + cnt);
    return times[cnt/2];
}

void bench(void (*func)(void*, size_t), void* mem, size_t size, uint64_t be ) {
    for (uint64_t i = 0; i < cnt; i++) {
        uint64_t t0 = rdtsc();
        func(mem, size);
        t0 = rdtsc() - t0;
        times[i] = t0;
    }
    std::sort(times.begin(), times.begin() + cnt);
    if (func == memReadRandom) {
    printf("%5.1f (%5.2f)", ((times[cnt/2]-be)/8.0/size*(cpufreq/tscfreq)), ((times[cnt/2]-be)/8.0/size/tscfreq*1e9));
        
    } else
    printf("%5.1f    ", (intbs * tscfreq) / ((times[cnt/2]-be) * 1e9));
}

int main(int argc, char *argv[]) {
    cpu_set_t cpuset;
    pid_t pid;

    int cpunum = 0;
    if (argc > 1) {
        std::stringstream args(argv[1]);
        args >> cpunum;
    }
    pid = getpid();
    CPU_ZERO(&cpuset);
    CPU_SET(cpunum, &cpuset);
//     sched_setaffinity(pid, sizeof(cpuset), &cpuset);

    signal(SIGALRM, (__sighandler_t)done);
    uint64_t start = rdtsc();
    alarm(1);
    while(!end);
    tscfreq = end-start;

    timeval tv1, tv2;
    loop(tscfreq/16);
    gettimeofday(&tv1, NULL);
    loop(tscfreq/16);
    gettimeofday(&tv2, NULL);
    cpufreq = (tscfreq*1e6)/(tv2.tv_sec*1e6+tv2.tv_usec - (tv1.tv_sec*1e6+tv1.tv_usec));
    
    printf("CPU frequency %6.3f GHz\n", cpufreq/1e9);
    printf("TSC frequency %6.3f GHz\n", tscfreq/1e9);
    times.reserve(65536);
    double maxblk = 0x40000000;
    uint64_t secondLevelCache = 0x800000;

    ptrdiff_t* mem = (ptrdiff_t*)valloc(maxblk);
    cnt = 65536;
	benchempty(empty, mem, 0);
	benchempty(empty, mem, 0);
	benchempty(empty, mem, 0);
    uint64_t be = benchempty(empty, mem, 0);
    printf("%lu\n", be);
    printf("size        read     copy     write GiB/s  latency clk(ns)\n");

    if (!mem) {
        printf("Not enough memory\n");
        exit(2);
    }

    for (int i = 0; i < maxblk/sizeof(uint64_t); i++)
            mem[i] = i;

    cachebegin = (char*) valloc(CACHE_SIZE);
    for (double bs = CACHE_LINE_SIZE * 32 * 2; bs <= maxblk; bs *= 2.0) {
        intbs = floor((bs + 0.5) / 128) * 128;
        cnt = (2 * maxblk) / intbs;
        if (cnt > 32768)
            cnt = 32768;

        uint64_t pbs = intbs;
        int expo = 0;
        while (pbs >= 1024) {
            pbs /= 1024;
            expo++;
        }
        uint64_t size64 = intbs/64;
        printf("%4lu %ciB:   ", pbs, " kMGT"[expo] );
        if (intbs > secondLevelCache) {
            bench(memReadNT, mem, size64, be);
            bench(memCopyNT, mem, size64, be);
            bench(memWriteNT, mem, size64, be);
        } else {
            bench(memRead, mem, size64, be);
    	    bench(memCopy, mem, size64, be);
            bench(memWrite, mem, size64, be);
        }
        for (ptrdiff_t i = 0; i < intbs/sizeof(uint64_t); i++)
            mem[i] = (ptrdiff_t)&mem[i];
        unsigned nCacheLines = intbs/CACHE_LINE_SIZE;
//         printf("%u ", nCacheLines);
        for (ptrdiff_t i = intbs/sizeof(ptrdiff_t)-1; i>0; --i) {
            // sattolo, not fisher yates
            ptrdiff_t a = random() % i;
            ptrdiff_t temp = mem[i];
            mem[i] = mem[a];
            mem[a] = temp;
        }
        printf("  ");    
        bench(memReadRandom, mem, size64, be);
        printf("\n");
        cnt = cnt/2;
        }

        return EXIT_SUCCESS;
}
