#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/clocks.h>
#include "dvd.h"


int main()
{
    set_sys_clock_khz(200000, true);
    stdio_init_all();

    dvd_init();

    while (true) {
        sleep_ms(10000);
    }
}
