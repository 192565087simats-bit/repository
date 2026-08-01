#include <stdio.h>

#include <stdlib.h>

#include <stdint.h>

#include <time.h>

/* ============================================================================
 * 1. SIMULATED HARDWARE REGISTERS & PERIPHERAL STRUCTURES
 * ============================================================================
 */
typedef struct {
    volatile uint32_t CTRL; /* Control Register */
    volatile uint32_t LOAD; /* Reload Register */
    volatile uint32_t VAL; /* Current Value Register */
    volatile uint32_t PRESCALER; /* Prescaler */
}
HardwareTimer_RegisterMap;

static HardwareTimer_RegisterMap Timer1 = {
    0,
    0,
    0,
    64
};

#define TIMER_CTRL_ENABLE(1 U << 0)
#define TIMER_CTRL_COUNTFLAG(1 U << 16)

/* ============================================================================
 * 2. TIMING DELAY FUNCTIONS
 * ============================================================================
 */

/* Precise delay in seconds using C clock */
void delay_seconds(double sec) {
    clock_t start = clock();
    while (((double)(clock() - start) / CLOCKS_PER_SEC) < sec);
}

/* Software Busy Loop */
void delay_software_unoptimized(uint32_t iterations) {
    for (volatile uint32_t i = 0; i < iterations; i++) {
        /* Prevent compiler from optimizing the loop away */
        __asm__("");
    }
}

/* Simulated Hardware Register Timer Delay */
void delay_hardware_register(double target_sec, uint32_t cpu_freq) {
    Timer1.LOAD = (uint32_t)((cpu_freq / Timer1.PRESCALER) * target_sec);
    Timer1.VAL = Timer1.LOAD;
    Timer1.CTRL = TIMER_CTRL_ENABLE;

    /* Simulate countdown in hardware register */
    delay_seconds(target_sec);

    Timer1.VAL = 0;
    Timer1.CTRL |= TIMER_CTRL_COUNTFLAG;
}

/* ============================================================================
 * 3. DASHBOARD DISPLAY & PROGRESS BAR (WITH FORCED FLUSH)
 * ============================================================================
 */

void draw_progress_bar(const char * label, double progress, double elapsed) {
    int bar_width = 25;
    printf("\r %-16s [", label);
    int pos = (int)(bar_width * progress);
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) printf("#");
        else if (i == pos) printf(">");
        else printf(".");
    }
    printf("] %3.0f%% | %6.4f s", progress * 100.0, elapsed);

    /* CRITICAL FIX: Forces terminal to display output instantly without buffering */
    fflush(stdout);
}

void print_dashboard_header(uint32_t cpu_freq) {
    printf("================================================================================\n");
    printf("                EMBEDDED HARDWARE REGISTER & TIMING DASHBOARD                  \n");
    printf("================================================================================\n");
    printf(" Target System Clock : %10u Hz (%u MHz)\n", cpu_freq, cpu_freq / 1000000);
    printf(" Timer Prescaler     : 1:%u\n", Timer1.PRESCALER);
    printf("================================================================================\n\n");
    fflush(stdout);
}

/* ============================================================================
 * 4. MAIN PROGRAM EXECUTION
 * ============================================================================
 */

int main(void) {
    uint32_t system_clock_hz = 16000000 U; /* 16 MHz System Clock */
    double target_time_sec = 1.0; /* 1 Second Target Delay */

    print_dashboard_header(system_clock_hz);

    printf(">>> RUNNING TIMING BENCHMARKS (Target: 1.0000s) <<<\n\n");
    fflush(stdout);

    /* -------------------------------------------------------------------------
     * TEST 1: Software Delay Loop Benchmark
     * -------------------------------------------------------------------------
     */
    clock_t sw_start = clock();
    uint32_t total_iterations = 80000000 UL;

    for (int step = 1; step <= 10; step++) {
        delay_software_unoptimized(total_iterations / 10);
        double elapsed = (double)(clock() - sw_start) / CLOCKS_PER_SEC;
        draw_progress_bar("Software Loop", (double) step / 10.0, elapsed);
    }
    double sw_total_time = (double)(clock() - sw_start) / CLOCKS_PER_SEC;
    printf("\n");

    /* -------------------------------------------------------------------------
     * TEST 2: Hardware Register Delay Benchmark
     * -------------------------------------------------------------------------
     */
    clock_t hw_start = clock();

    for (int step = 1; step <= 10; step++) {
        delay_hardware_register(target_time_sec / 10.0, system_clock_hz);
        double elapsed = (double)(clock() - hw_start) / CLOCKS_PER_SEC;
        draw_progress_bar("Hardware Timer", (double) step / 10.0, elapsed);
    }
    double hw_total_time = (double)(clock() - hw_start) / CLOCKS_PER_SEC;
    printf("\n\n");

    /* -------------------------------------------------------------------------
     * SUMMARY & EVALUATION TABLE
     * -------------------------------------------------------------------------
     */
    double sw_error = ((sw_total_time - target_time_sec) / target_time_sec) * 100.0;
    double hw_error = ((hw_total_time - target_time_sec) / target_time_sec) * 100.0;

    printf("================================================================================\n");
    printf("                         PERFORMANCE METRICS EVALUATION                         \n");
    printf("================================================================================\n");
    printf(" Method           | Target Time | Measured Time | Timing Drift Error | CPU Load \n");
    printf("------------------+-------------+---------------+--------------------+----------\n");
    printf(" Software Loop    |   1.0000 s  |   %6.4f s   |     %+6.2f %%      |   100%%   \n",
        sw_total_time, sw_error);
    printf(" Hardware Timer   |   1.0000 s  |   %6.4f s   |     %+6.2f %%      |  Minimal \n",
        hw_total_time, hw_error);
    printf("================================================================================\n");

    printf("\nRegister State After Execution:\n");
    printf(" [LOAD: 0x%08X] | [VAL: 0x%08X] | [CTRL: 0x%08X (FLAG_SET)]\n\n",
        Timer1.LOAD, Timer1.VAL, Timer1.CTRL);

    return 0;
}