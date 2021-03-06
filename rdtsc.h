/*
 * rdtsc.h
 *
 *  Created on: 09.08.2016
 *      Author: kruppa
 */

#ifndef RDTSC_H_
#define RDTSC_H_

/* Enable at most one of these two defines */
// #define USE_INTEL_PCM
// #define USE_PERF

#ifndef _xstr
#define _xstr(s) _str(s)
#endif
#ifndef _str
#define _str(x) #x
#endif

#include <stdint.h>

#ifdef USE_INTEL_PCM
    #include <sched.h>
	#include "cpucounters.h"
	static PCM * m;
	static CoreCounterState before_sstate, after_sstate;
#elif defined USE_PERF
	#include "libperf.h"  /* standard libperf include */
	static struct libperf_data* pd;
	static uint64_t start_time, end_time;
#elif USE_JEVENTS
    #ifdef __cplusplus
	    extern "C" {
    #endif
    #include "rdpmc.h"
    #ifdef __cplusplus
	    }
    #endif

#define IMMEDIATE_RDPMC 1
	struct rdpmc_ctx ctx;
#ifdef IMMEDIATE_RDPMC
	static union {
	    struct {unsigned lo, hi;} pair;
	    unsigned long l;
	} start_time;
	static unsigned long end_time;
#else
	static uint64_t start_time, end_time;
#endif
#else
#define START_COUNTER rdtscpl
#define END_COUNTER rdtscpl
	static uint64_t start_time, end_time;
#endif

__attribute__((__unused__, __artificial__, __always_inline__))
static inline void serialize()
{
		unsigned long dummy = 0;
		__asm__ volatile("CPUID\n\t"
	                 : "+a" (dummy)
	                 :: "%rbx", "%rcx", "%rdx"
	                );
}

__attribute__((__unused__, __artificial__, __always_inline__))
static inline void rdtsc(uint32_t *low, uint32_t *high)
{
  __asm__ volatile("RDTSC\n\t"
               : "=d" (*high), "=a" (*low)
              );
}

__attribute__((__unused__, __artificial__, __always_inline__))
static inline void rdtscp(uint32_t *low, uint32_t *high)
{
  __asm__ volatile("RDTSCP\n\t"
               : "=d" (*high), "=a" (*low)
               :: "%rcx"
              );
}

__attribute__((__unused__, __artificial__, __always_inline__))
static inline uint64_t rdtscl()
{
  uint32_t high, low;
  rdtsc(&low, &high);
  return ((uint64_t) high << 32) + (uint64_t) low;
}

__attribute__((__unused__, __artificial__, __always_inline__))
static inline uint64_t rdtscpl()
{
  uint32_t high, low;
  rdtscp(&low, &high);
  return ((uint64_t) high << 32) + (uint64_t) low;
}

__attribute__((__unused__, __artificial__, __always_inline__))
static inline uint64_t rdtscidl()
{
  uint32_t high, low;
  rdtsc(&low, &high);
  serialize();
  return ((uint64_t) high << 32) + (uint64_t) low;
}

__attribute__((__unused__, __artificial__, __always_inline__))
static inline uint64_t idrdtscl()
{
  uint32_t high, low;
  serialize();
  rdtsc(&low, &high);
  return ((uint64_t) high << 32) + (uint64_t) low;
}

__attribute__((__unused__, __artificial__, __always_inline__))
static inline uint64_t rdtscpidl()
{
  uint32_t high, low;
  rdtscp(&low, &high);
  serialize();
  return ((uint64_t) high << 32) + (uint64_t) low;
}

__attribute__((__unused__, __artificial__, __always_inline__))
static inline uint64_t idrdtscpl()
{
  uint32_t high, low;
  serialize();
  rdtscp(&low, &high);
  return ((uint64_t) high << 32) + (uint64_t) low;
}

__attribute__((__unused__, __artificial__, __always_inline__))
static inline unsigned long
rdpmc(const unsigned int selector)
{
    unsigned hi, lo;
    __asm__ volatile("rdpmc" : "=a" (lo), "=d" (hi) : "c" (selector));
    return ((unsigned long)lo) + (((unsigned long)hi) << 32);
}

// rdpmc_cycles uses a "fixed-function" performance counter to return
// the count of actual CPU core cycles executed by the current core.
// Core cycles are not accumulated while the processor is in the "HALT"
// state, which is used when the operating system has no task(s) to run
// on a processor core.
// Note that this counter continues to increment during system calls
// and task switches. As such, it may be unreliable for timing long
// functions where the CPU may serve an interrupt request or where
// the kernel may preempt execution and switch to another process.
// It is best used for timing short intervals which usually run
// uninterrupted, and where occurrences of interruption are easily
// detected by an abnormally large cycle count.

// The RDPMC instruction must be enabled for execution in user-space.
// This requires a total of three bits to be set in CR4 and MSRs of
// the CPU:
// Bit 1<<8 in CR4 must be set to 1. On Linux, this can be effected by
// executing as root:
//   echo 1 >> /sys/devices/cpu/rdpmc
// Bit 1<<33 must be set to 1 in the MSR_CORE_PERF_GLOBAL_CTRL
// (MSR address 0x38f). This enables the cycle counter so that it
// actually increment with each clock cycle; while this bit is 0,
// the counter value stays fixed.
// Bit 1<<5 must be set to 1 in the MSR_CORE_PERF_FIXED_CTR_CTRL
// (MSR address 0x38d) which causes the counter to be incremented
// on non-halted clock cycles that occur while the CPL is >0
// (user-mode). If bit 1<<4 is set to 1, then the counter will
// increment while CPL is 0 (kernel mode), e.g., during interrupts,
// etc.

__attribute__((__unused__, __artificial__, __always_inline__))
static inline unsigned long
rdpmc_cycles()
{
   const unsigned c = (1U<<30) + 1; /* Second Fixed-function counter:
                                      clock cycles in non-HALT */
   return rdpmc(c);
}

__attribute__((__unused__, __artificial__, __always_inline__))
static inline void
rdpmc_cycles2(unsigned *lo, unsigned *hi)
{
   const unsigned c = (1U<<30) + 1; /* Second Fixed-function counter:
                                      clock cycles in non-HALT */

   __asm__ volatile("rdpmc" : "=a" (*lo), "=d" (*hi) : "c" (c));
}

static void init_timing()
{
#ifdef USE_INTEL_PCM
  printf("# Using Intel PCM library\n");
  m = PCM::getInstance();
  if (m->program() != PCM::Success) {
    printf("Could not initialise PCM\n");
    exit(EXIT_FAILURE);
  }

  const bool have_smt = m->getSMT();
  if (have_smt) {
	  printf("# CPU uses SMT\n");
  } else {
	  printf("# CPU does not use SMT\n");
  }
#elif defined(USE_PERF)
  printf("# Using PERF library\n");
  pd = libperf_initialize(-1,-1); /* init lib */
  libperf_enablecounter(pd, LIBPERF_COUNT_HW_CPU_CYCLES);
                                        /* enable HW counter */
#elif defined(USE_JEVENTS)
  printf("# Using jevents library\n");
#ifdef TIMING_SERIALIZE
  printf("# Using %s to serialize before timing start/before timing end\n",
          _xstr(TIMING_SERIALIZE));
#endif
  if (rdpmc_open(PERF_COUNT_HW_CPU_CYCLES, &ctx) < 0)
	exit(EXIT_FAILURE);
#else
  printf("# Using " _xstr(START_COUNTER) " to start and " _xstr(END_COUNTER) " to end measurement\n");
#endif
}

static void clear_timing()
{
#ifdef USE_INTEL_PCM
  m->cleanup();
#elif defined(USE_PERF)
  libperf_close(pd);
  pd = NULL;
#elif defined(USE_JEVENTS)
  rdpmc_close (&ctx);
#endif
}

static inline void start_timing()
{
#ifdef USE_INTEL_PCM
    const int cpu = sched_getcpu();
    before_sstate = getCoreCounterState(cpu);
#elif defined(USE_PERF)
    start_time = libperf_readcounter(pd, LIBPERF_COUNT_HW_CPU_CYCLES);
                                          /* obtain counter value */
#elif defined(USE_JEVENTS)
#ifdef TIMING_SERIALIZE
    serialize();
#endif
#ifdef IMMEDIATE_RDPMC
    rdpmc_cycles2(&start_time.pair.lo, &start_time.pair.hi);
#else
    start_time = rdpmc_read(&ctx);
#endif
#else
    start_time = START_COUNTER();
#endif
}

static inline void end_timing()
{
#ifdef USE_INTEL_PCM
    const int cpu = sched_getcpu();
    after_sstate = getCoreCounterState(cpu);
#elif defined(USE_PERF)
    end_time = libperf_readcounter(pd, LIBPERF_COUNT_HW_CPU_CYCLES);
                                          /* obtain counter value */
#elif defined(USE_JEVENTS)
#ifdef TIMING_SERIALIZE
    rdtscpl();
#endif
#ifdef IMMEDIATE_RDPMC
    end_time = rdpmc_cycles();
#else
    end_time = rdpmc_read(&ctx);
#endif
#else
    end_time = END_COUNTER();
#endif
}

static inline uint64_t get_diff_timing()
{
#ifdef USE_INTEL_PCM
    return getCycles(before_sstate,after_sstate);
#elif defined(USE_PERF)
    return end_time - start_time;
#elif defined(USE_JEVENTS)
#ifdef IMMEDIATE_RDPMC
    return end_time - start_time.l;
#else
    return end_time - start_time;
#endif
#else
    return end_time - start_time;
#endif

}

#ifdef USE_JEVENTS
static unsigned long pmc0, pmc1, pmc2, pmc3;
#endif

void
readAllPmc()
{
#ifdef USE_JEVENTS
  pmc0 = rdpmc(0);
  pmc1 = rdpmc(1);
  pmc2 = rdpmc(2);
  pmc3 = rdpmc(3);
#endif
}

void
diffAllPmc()
{
#ifdef USE_JEVENTS
  pmc0 = rdpmc(0) - pmc0;
  pmc1 = rdpmc(1) - pmc1;
  pmc2 = rdpmc(0) - pmc2;
  pmc3 = rdpmc(1) - pmc3;
#endif
}

void
printAllPmc()
{
#ifdef USE_JEVENTS
  printf(" (pmc0: %lu, pmc1: %lu, pmc2: %lu, pmc3: %lu)", pmc0, pmc1, pmc2, pmc3);
#endif
}

#endif /* RDTSC_H_ */
