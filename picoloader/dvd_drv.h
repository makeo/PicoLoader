#ifndef DVD_DRV_H
#define DVD_DRV_H

#include <stdint.h>


void dvd_drv_init();

void dvd_drv_send(const void* data, uint32_t len);
void dvd_drv_set_error();

void dvd_drv_enable_passthrough();

#endif // DVD_DRV_H
