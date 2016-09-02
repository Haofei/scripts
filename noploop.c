/*
 * $File: noploop.c
 * $Description:
 *   Calculate CPU frequency based on execution time of nop instructions.
 *   Calculate the Superscalar CPU factor of instructions completed per cycle.
 *
 *   Three time measures are used to calculate CPU frequency:
 *   - usertime():  Measures user process time withouth external interference. Cost of usertime() call ~450 cycles.
 *   - realtime():  Measures wallclock time including system & other processes interferece. Cost of realtime() call ~250 cycles.
 *   - tsccycles(): Measures CPU cycles using the rdtsc instruction. Cost of tsccyles() call ~20 cycles.
 *
 * Idea from Brendan Gregg's: The noploop CPU Benchmark
 * http://www.brendangregg.com/blog/2014-04-26/the-noploop-cpu-benchmark.html
 */

#define _GNU_SOURCE

#include <time.h>
#include <stdio.h>
#include <stdint.h>

#define NOP0  __asm__ __volatile__ ("nop" : /* out */ : /* in */ : "eax" );
#define NOP1  NOP0 NOP0
#define NOP2  NOP1 NOP1
#define NOP3  NOP2 NOP2
#define NOP4  NOP3 NOP3
#define NOP5  NOP4 NOP4
#define NOP6  NOP5 NOP5
#define NOP7  NOP6 NOP6
#define NOP8  NOP7 NOP7
#define NOP9  NOP8 NOP8
#define NOP10 NOP9 NOP9
#define NOP11 NOP10 NOP10
#define NOP12 NOP11 NOP11

#define NOPS  4096  // NOP12 generates 4096 = 2^12 NOPs
#define NLOOP 10000000

static void
noploop(void)
{
    register int i;
    for (i = 0; i < NLOOP; i++) {
        NOP12;
    }
}

static inline uint64_t /* wallclock time (ms) */
realtime(void)
{
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

static inline uint64_t /* user process time (ms) */
usertime(void)
{
    return clock() / (CLOCKS_PER_SEC / 1000);
}

static inline uint64_t /* tsc cpu cycles (cycles) */
tsccycles(void)
{
    uint64_t cycles;
    __asm__ __volatile__ ("rdtsc" : "=A" (cycles));
    return cycles;
}

static void
mainloop(uint64_t (*gettime)(void))
{
    uint64_t start, end, instrate, period;
    uint64_t tscstart, tscend, tscticks, tscfreq;

    start = gettime();
    tscstart = tsccycles();
    noploop();
    tscend = tsccycles();
    end = gettime();

    tscticks = tscend - tscstart;
    period = end - start;
    tscfreq = tscticks / period;
    instrate = ((uint64_t)NOPS * NLOOP) / (period);
    printf ("noploop: instr: %lld KHz (time %lld ms) freq %lld KHz (%lld ticks) Superscalar: %lld instr/cycle\n",
            instrate, period, tscfreq, tscticks, (instrate+ (tscfreq / 2)) / tscfreq);
}

# if 0
static void
maincost(uint64_t (*gettime)(void))
{
    int32_t i;
    uint64_t a, b;
    int32_t iterations = 1000000;

    a = tsccycles();
    for (i=0; i < iterations; i++)
    {
        gettime();
    }
    b = tsccycles();

    printf("gettime at %p cost: %lld cycles\n", gettime, (b - a) / iterations);
}
#endif

int main(int argc, char *argv[])
{
    int32_t i;
    uint64_t (*gettime)(void) = (argc > 1) ? realtime : usertime;

    for (i=0; i<3; i++)
        mainloop(gettime);
    return 0;
}

/* Compiler command-line:
   cc -std=c99 -m32 -Wall -lrt -O0 noploop.c -o noploop && ./noploop & # */
