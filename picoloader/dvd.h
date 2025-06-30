#ifndef DVD_H
#define DVD_H

#include <stdint.h>


void dvd_init();

// callbacks from driver
void dvd_request(uint8_t* req);
void dvd_reset();

void dvd_task();

#endif // DVD_H
