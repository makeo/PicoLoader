#include "dvd.h"

#include <stdio.h>
#include <stdbool.h>
#include <hardware/flash.h>
#include "dvd_drv.h"

static bool disable_on_rst = false;
static uint32_t last_error = 0;

static const uint8_t* disk_data;


// include header data for dol files
asm("\
    .section .header, \"a\"\n\
    .incbin \"data/gbi.hdr\"\n\
    .incbin \"data/iso.hdr\"\n\
");
extern uint8_t data_header[];

// start of the actual payload
extern uint8_t data_payload[];


bool dvd_is_valid_dol(const uint8_t* dol);

void dvd_init()
{
    dvd_drv_init();

    // iso
    if (*(uint32_t*)&data_payload[0x1c] == 0x3d9f33c2) {
        disk_data = data_payload;
        return;
    }

    // dol
    if (dvd_is_valid_dol(data_payload)) {
        disk_data = data_header;
        return;
    }

    dvd_drv_enable_passthrough();
}

bool dvd_is_valid_dol(const uint8_t* dol) {
    // check addresses to see if it is a valid dol
    #define CHECK_ADDR(x) ((x) & 0b11 || (x) < 0x80000000 || (x) > 0x817fffff)
    uint32_t entrypoint = __builtin_bswap32(*(uint32_t*)&dol[0xe0]);
    if (CHECK_ADDR(entrypoint))
        return false;
    for (uint32_t i = 0; i < 18; i++) {
        uint32_t address =  __builtin_bswap32(*(uint32_t*)&dol[0x48 + 4*i]);
        if (address != 0 && CHECK_ADDR(address))
            return false;
    }

    // compute the size of the dol 
    uint32_t size = 0;
    for (uint32_t i = 0; i < 18; i++)
        size = MAX(size, __builtin_bswap32(*(uint32_t*)&dol[4*i]) + __builtin_bswap32(*(uint32_t*)&dol[0x90 + 4*i]));

    // reject if too big
    if(size < 0xFF || ((uint32_t)dol + size) > XIP_NOALLOC_BASE)
        return false;

    return true;
}


void dvd_request(uint8_t *req)
{
    if (req == NULL) {
        dvd_drv_set_error();
        last_error = 0x040800;
        return;
    }

    switch(req[0]) {
        case 0x12: // inquiry
            static uint8_t drive_info[32] = { 0x00, 0x00, 0x00, 0x00, 0x20, 0x02, 0x04, 0x02, 0x61, 0x00, 0x00, 0x00 };
            dvd_drv_send(drive_info, sizeof(drive_info));
            break;

        case 0xA8: // read
            {
                disable_on_rst = true;
                uint32_t addr = __builtin_bswap32(*(uint32_t*)&req[4]) << 2;
                uint32_t len = __builtin_bswap32(*(uint32_t*)&req[8]) & ~0x1F;
                dvd_drv_send(&disk_data[addr], len);
            }
            break;

        case 0xAB: // seek
            static uint8_t seek_resp[4] = { 0 };
            dvd_drv_send(seek_resp, sizeof(seek_resp));
            break;

        case 0xE0: // request error
            static uint8_t error_resp[4] = { 0 };
            *(uint32_t*)error_resp = __builtin_bswap32(last_error);
            error_resp[0] = 0; // status
            dvd_drv_send(error_resp, sizeof(error_resp));

            last_error = 0;
            break;

        case 0xE3: // stop motor
            static uint8_t motor_resp[4] = { 0 };
            dvd_drv_send(motor_resp, sizeof(motor_resp));
            break;

        case 0xE4: // audio buffer config
            static uint8_t audio_resp[4] = { 0 };
            dvd_drv_send(audio_resp, sizeof(audio_resp));
            break;

        case 0xF4: // custom device info
            static uint8_t device_info[32] = { 0x0D, 0x15, 0xE4, 0x5E, 0x00, 0x00, 0x00, 0x00 };
            dvd_drv_send(device_info, sizeof(device_info));
            break;

        default:
            dvd_drv_set_error();
            last_error = 0x052000;
            break;
    }
}

void dvd_reset()
{
    if (disable_on_rst) {
        dvd_drv_enable_passthrough();
    }
}

void dvd_task()
{
    
}
