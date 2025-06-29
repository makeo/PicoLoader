#include "dvd.h"

#include <stdio.h>
#include <stdbool.h>
#include "dvd_drv.h"
#include "test_payload.h"

static bool disable_on_rst = false;
static uint32_t last_error = 0;

void dvd_init()
{
    dvd_drv_init();
}

void dvd_request(uint8_t *req)
{
    printf("req: %x %x %x %x %x %x %x %x %x %x %x %x\n", req[0], req[1], req[2], req[3], req[4], req[5], req[6], req[7], req[8], req[9], req[10], req[11]);

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
                uint32_t addr = __builtin_bswap32(*(uint32_t*)&req[4]);
                uint32_t len = __builtin_bswap32(*(uint32_t*)&req[8]) & ~0x1F;
                dvd_drv_send(&gekkoboot_pal_payload[addr << 2], len);
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
            printf("err\n");
            dvd_drv_set_error();
            last_error = 0x052000;
            break;
    }
}

void dvd_reset()
{
    printf("reset cb\n");
    if (disable_on_rst) {
        printf("enable passthrough\n");
        dvd_drv_enable_passthrough();
    }
}
