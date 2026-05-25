// Bypasses hardware initialization since for Spike there is no hardware to configure 
// and we're tracing instructions instead of measuring wall time.

#include "support.h"

void initialise_board(void) {}
void start_trigger(void) {}
void stop_trigger(void) {}

int main()
{
    initialise_benchmark();
    int result = benchmark();
    return verify_benchmark(result) ? 0 : 1;
}
