#include "dvd.h"

#include <stdio.h>
#include <stdbool.h>
#include <hardware/flash.h>
#include <pico/bootrom.h>
#include <pico/time.h>
#include <hardware/gpio.h>
#include "dvd_drv.h"

// 0: off 1: on 2: blink
static uint8_t led_state = 0;

static bool disable_on_rst = false;
static uint32_t last_error = 0;

static const uint8_t* disk_data;

static uint32_t flash_capacity = 0;
#define XIP_MAX (XIP_BASE + flash_capacity)

// include header data for dol files
asm("\
    .section .header, \"a\"\n\
    .incbin \"data/gbi.hdr\"\n\
    .incbin \"data/iso.hdr\"\n\
");
extern uint8_t data_header[];

// start of the actual payload
extern uint8_t data_payload[];

uint32_t get_flash_capacity();
bool dvd_is_valid_dol(const uint8_t* dol);

void dvd_init()
{
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    led_state = 1;

    flash_capacity = get_flash_capacity();

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

    if (!dvd_drv_enable_passthrough()) {
        dvd_drv_set_cover(true);
    }
    led_state = 2;
}

uint32_t get_flash_capacity() {
    uint8_t txbuf[4] = {0x9f};
    uint8_t rxbuf[4] = {0};
    flash_do_cmd(txbuf, rxbuf, 4);
    return 1 << rxbuf[3];
}

bool dvd_is_valid_dol(const uint8_t* dol) {
    // check if the dol is bootable by ipl

    bool entrypoint_valid = false;
    uint32_t entrypoint = __builtin_bswap32(*(uint32_t*)&dol[0xe0]);

    uint32_t dol_size = 0;

    for (uint32_t i = 0; i < 18; i++) {
        uint32_t offset = __builtin_bswap32(*(uint32_t*)&dol[0x00 + 4*i]);
        if (offset != 0 && (offset & 0b11 || offset < 0x100))
            return false;

        uint32_t address = __builtin_bswap32(*(uint32_t*)&dol[0x48 + 4*i]);
        if (address != 0 && (address & 0b11 || address < 0x80000000 || address > 0x81200000))
            return false;

        uint32_t size = __builtin_bswap32(*(uint32_t*)&dol[0x90 + 4*i]);

        dol_size = MAX(dol_size, offset + size);

        // check if entrypoint is within a text segment
        if (i < 7 && entrypoint >= address && entrypoint < address + size)
            entrypoint_valid = true;
    }

    if (!entrypoint_valid)
        return false;

    uint32_t bss_address = __builtin_bswap32(*(uint32_t*)&dol[0xd8]);
    if (bss_address != 0 && bss_address < 0x80000000)
        return false;

    // reject if too small or too big
    if(dol_size < 0x100 || ((uint32_t)dol + dol_size) > XIP_MAX)
        return false;

    return true;
}

void dvd_request_custom(uint8_t *req);

void dvd_request(uint8_t *req)
{
    if (req == NULL) {
        dvd_drv_set_error();
        last_error = 0x040800;
        return;
    }

    switch(req[0]) {
        case 0x12: // inquiry
            static uint8_t drive_info[32] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            dvd_drv_send(drive_info, sizeof(drive_info));
            break;

        case 0xA8: // read
            {
                disable_on_rst = true;

                uint32_t addr = __builtin_bswap32(*(uint32_t*)&req[4]) << 2;
                uint32_t len = __builtin_bswap32(*(uint32_t*)&req[8]) & ~0x1F;

                const uint8_t* dat = &disk_data[addr];
                if ((uint32_t)dat + len > XIP_MAX) {
                    dvd_drv_set_error();
                    last_error = 0x056300; // lead-out
                } else {
                    dvd_drv_send(dat, len);
                }
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

        case 0xF4: // custom command
            dvd_request_custom(req);
            break;

        default:
            dvd_drv_set_error();
            last_error = 0x052000;
            break;
    }
}

// custom commands
void dvd_request_custom(uint8_t *req) {
    switch(req[1]) {
        case 0x00: // device info
            static uint8_t device_info[32] = { 0x0D, 0x15, 0xE4, 0x5E, PROGRAM_VERSION_MAJOR, PROGRAM_VERSION_MINOR, 0x00, 0x00 }; // last two bytes are for future use
            dvd_drv_send(device_info, sizeof(device_info));
            break;

        case 0x01: // reboot into bootloader 
            reset_usb_boot(0, 0);
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
    switch(led_state) {
        case 0:
            gpio_put(PICO_DEFAULT_LED_PIN, false);
            break;
        case 1:
            gpio_put(PICO_DEFAULT_LED_PIN, true);
            break;
        case 2:
            gpio_put(PICO_DEFAULT_LED_PIN, time_us_64() & (1 << 19));
            break;
    }
}
