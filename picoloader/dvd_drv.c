#include "dvd_drv.h"

#include <stdio.h>
#include <hardware/pio.h>
#include <hardware/gpio.h>
#include <hardware/dma.h>
#include <hardware/clocks.h>
#include <pico/stdlib.h>
#include "drive_interface.pio.h"
#include "dvd.h"

#define PIO_RECV_SM 0
#define PIO_SEND_SM 1
#define PIO_DIR_SM 2
#define PIO_PASSTHROUGH_SM 3

static int send_chan;

void dvd_drv_init_gpios();
void dvd_drv_init_pio();

void dvd_drv_clear_state();

void dvd_drv_gpio_irq(void);
void dvd_drv_dir_irq_in(void);
void dvd_drv_dir_irq_out(void);

void dvd_drv_init() {
    // dbg pin
    gpio_init(22);
    gpio_set_dir(22, true);
    gpio_put(22, 0);

    dvd_drv_init_gpios();

    // hold drive in reset and keep dir low
    gpio_set_outover(pin_dvd_reset, GPIO_OVERRIDE_LOW);
    gpio_set_outover(pin_dvd_dir, GPIO_OVERRIDE_LOW);

    dvd_drv_init_pio();

    // passthrough for dstrb
    {
        uint gpclk = 0;
        if      (pin_gc_dstrb == 21) gpclk = clk_gpout0;
        else if (pin_gc_dstrb == 23) gpclk = clk_gpout1;
        else if (pin_gc_dstrb == 24) gpclk = clk_gpout2;
        else if (pin_gc_dstrb == 25) gpclk = clk_gpout3;

        clocks_hw->clk[gpclk].div = 1 << CLOCKS_CLK_GPOUT0_DIV_INT_LSB;

        // set another clock source first so that the first clock cycles of dstrb are not missing
        clocks_hw->clk[gpclk].ctrl = (CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_XOSC_CLKSRC << CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_LSB) | CLOCKS_CLK_GPOUT0_CTRL_ENABLE_BITS;
        sleep_us(10);
        clocks_hw->clk[gpclk].ctrl = (CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0 << CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_LSB) | CLOCKS_CLK_GPOUT0_CTRL_ENABLE_BITS;
    }

    // dma for sending data
    send_chan = dma_claim_unused_channel(true);
    {
        dma_channel_config c = dma_channel_get_default_config(send_chan);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
        channel_config_set_dreq(&c, pio_get_dreq(pio0, PIO_SEND_SM, true));
        channel_config_set_read_increment(&c, true);
        channel_config_set_write_increment(&c, false);
        dma_channel_set_config(send_chan, &c, false);
        dma_channel_set_write_addr(send_chan, &pio0->txf[PIO_SEND_SM], false);
    }
    

    // gpio irq's for detecting a reset and break signal
    gpio_set_irq_enabled(pin_gc_reset, GPIO_IRQ_LEVEL_LOW, true);
    gpio_set_irq_enabled(pin_gc_brk, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    irq_set_exclusive_handler(IO_IRQ_BANK0, dvd_drv_gpio_irq);

    // irq handlers for direction changes
    irq_set_exclusive_handler(PIO0_IRQ_0, dvd_drv_dir_irq_in);
    irq_set_enabled(PIO0_IRQ_0, true);
    pio_set_irq0_source_enabled(pio0, PIO_INTR_SM0_LSB, true);
    irq_set_exclusive_handler(PIO0_IRQ_1, dvd_drv_dir_irq_out);
    irq_set_enabled(PIO0_IRQ_1, true);
    pio_set_irq1_source_enabled(pio0, PIO_INTR_SM1_LSB, true);


    // close cover
    gpio_set_outover(pin_gc_cover, GPIO_OVERRIDE_LOW);

    // start ode
    irq_set_enabled(IO_IRQ_BANK0, true);
    pio_enable_sm_mask_in_sync(pio0, (1 << PIO_RECV_SM) | (1 << PIO_SEND_SM) | (1 << PIO_DIR_SM));
}

void dvd_drv_init_gpios() {
    // gpios that are always inputs
    static uint8_t inputs[] = { pin_gc_dir, pin_hstrb, pin_dvd_dstrb, pin_dvd_err, pin_dvd_cover, pin_gc_reset };
    for (int i = 0; i < sizeof(inputs); i++) {
        gpio_set_function(inputs[i], GPIO_FUNC_PIO0);
        gpio_set_oeover(inputs[i], GPIO_OVERRIDE_LOW);
        pio_sm_set_consecutive_pindirs(pio0, 0, inputs[i], 1, false);
        gpio_set_pulls(inputs[i], false, false);
    }
    gpio_set_function(pin_dvd_dstrb, GPIO_FUNC_GPCK);
    gpio_set_pulls(pin_gc_reset, true, false);

    // gpios that are always outputs
    static uint8_t outputs[] = { pin_dvd_dir, pin_gc_dstrb, pin_gc_err, pin_gc_cover, pin_dvd_reset };
    for (int i = 0; i < sizeof(outputs); i++) {
        gpio_set_function(outputs[i], GPIO_FUNC_PIO0);
        gpio_set_oeover(outputs[i], GPIO_OVERRIDE_HIGH);
        pio_sm_set_consecutive_pindirs(pio0, 0, outputs[i], 1, true);
        gpio_set_pulls(outputs[i], false, false);
        gpio_set_slew_rate(outputs[i], GPIO_SLEW_RATE_FAST);
        gpio_set_drive_strength(outputs[i], GPIO_DRIVE_STRENGTH_12MA);
    }

    // gpios that are bidirectional
    static uint8_t bidirectionals[] = { pin_d0, pin_d1, pin_d2, pin_d3, pin_d4, pin_d5, pin_d6, pin_d7 };
    for (int i = 0; i < sizeof(bidirectionals); i++) {
        gpio_set_function(bidirectionals[i], GPIO_FUNC_PIO0);
        pio_sm_set_consecutive_pindirs(pio0, 0, bidirectionals[i], 1, false);
        gpio_set_pulls(bidirectionals[i], false, false);
        gpio_set_slew_rate(bidirectionals[i], GPIO_SLEW_RATE_FAST);
        gpio_set_drive_strength(bidirectionals[i], GPIO_DRIVE_STRENGTH_12MA);
    }

    // open drain gpios
    gpio_init(pin_gc_brk);
    gpio_set_outover(pin_gc_brk, GPIO_OVERRIDE_LOW);
    gpio_set_oeover(pin_dvd_brk, GPIO_OVERRIDE_LOW); // TODO: not yet implemented
}

void dvd_drv_init_pio() {
    // sm for passthrough of dir, err, cover and reset
    {
        int offset = pio_add_program(pio0, &passthrough_program);
        int sm = PIO_PASSTHROUGH_SM;
        pio_sm_claim(pio0, PIO_PASSTHROUGH_SM);
        pio_sm_config c = passthrough_program_get_default_config(offset);
        
        #define MIN4(a, b, c, d) (MIN(MIN(a, b), MIN(b, c)))
        sm_config_set_in_pins(&c, MIN4(pin_gc_dir, pin_dvd_err, pin_dvd_cover, pin_gc_reset));
        sm_config_set_out_pins(&c, MIN4(pin_dvd_dir, pin_gc_err, pin_gc_cover, pin_dvd_reset), 4);
        
        pio_sm_init(pio0, sm, offset, &c);
    }

    // sm for receiving data
    {
        int offset = pio_add_program(pio0, &bus_recv_program);
        int sm = PIO_RECV_SM;
        pio_sm_claim(pio0, sm);
        pio_sm_config c = bus_recv_program_get_default_config(offset);
        sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
        sm_config_set_in_shift(&c, false, true, 32);
        sm_config_set_in_pin_base(&c, pin_d0);
        sm_config_set_set_pins(&c, pin_gc_err, 1);
        sm_config_set_mov_status(&c, STATUS_RX_LESSTHAN, 3);
        pio_sm_init(pio0, sm, offset, &c);

        // store offset into y for easy reset
        pio_sm_exec(pio0, sm, pio_encode_set(pio_y, offset));

        pio_sm_set_consecutive_pindirs(pio0, sm, pin_gc_err, 1, true);
        pio_sm_exec(pio0, sm, pio_encode_set(pio_pins, 1));
    }

    // sm for sending data
    {
        int offset = pio_add_program(pio0, &bus_send_program);
        int sm = PIO_SEND_SM;
        pio_sm_claim(pio0, sm);
        pio_sm_config c = bus_send_program_get_default_config(offset);
        sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
        sm_config_set_out_shift(&c, true, true, 32);
        sm_config_set_out_pins(&c, pin_d0, 8);
        sm_config_set_sideset(&c, 2, true, false);
        sm_config_set_sideset_pins(&c, pin_gc_dstrb);
        pio_sm_init(pio0, sm, offset, &c);

        // store offset into y for easy reset
        pio_sm_exec(pio0, sm, pio_encode_set(pio_y, offset));

        pio_sm_set_consecutive_pindirs(pio0, sm, pin_gc_dstrb, 1, true);
    }

    // sm for changing directions of data pins
    {
        // sm must be able to mask dstrb
        assert(PIO_DIR_SM > PIO_SEND_SM);

        int offset = pio_add_program(pio0, &dir_program);
        int sm = PIO_DIR_SM;
        pio_sm_claim(pio0, sm);
        pio_sm_config c = dir_program_get_default_config(offset);
        sm_config_set_out_pins(&c, pin_d0, 8);
        sm_config_set_out_shift(&c, false, false, 32);
        sm_config_set_sideset(&c, 2, true, false);
        sm_config_set_sideset_pins(&c, pin_gc_dstrb);
        // disable until first reset
        pio_sm_init(pio0, sm, offset + dir_offset_disable, &c);

        // store offset into y for easy reset
        pio_sm_exec(pio0, sm, pio_encode_set(pio_y, offset));

        // store offset+disale into x for easy disable
        pio_sm_exec(pio0, sm, pio_encode_set(pio_x, offset + dir_offset_disable));

        pio_sm_set_consecutive_pindirs(pio0, sm, pin_gc_dir, 1, false);
        pio_sm_set_consecutive_pindirs(pio0, sm, pin_d0, 8, false);
    }
}

// reset all state machines and wait for the next transfer
void dvd_drv_clear_state() {
    // reset recv pio
    pio_sm_set_enabled(pio0, PIO_RECV_SM, false);
    pio_sm_exec(pio0, PIO_RECV_SM, pio_encode_mov(pio_exec_mov, pio_y));
    pio_sm_clear_fifos(pio0, PIO_RECV_SM);
    pio_sm_exec(pio0, PIO_RECV_SM, pio_encode_mov(pio_isr, pio_null));
    pio_sm_set_enabled(pio0, PIO_RECV_SM, true);

    // stop remaining send
    dma_channel_abort(send_chan);

    // reset send pio
    pio_sm_set_enabled(pio0, PIO_SEND_SM, false);
    pio_sm_clear_fifos(pio0, PIO_SEND_SM);
    pio_sm_exec(pio0, PIO_SEND_SM, pio_encode_mov(pio_exec_mov, pio_y));
    pio_sm_restart(pio0, PIO_SEND_SM);
    pio_sm_set_enabled(pio0, PIO_SEND_SM, true);

    // reset dir sm
    pio_sm_exec(pio0, PIO_DIR_SM, pio_encode_mov(pio_exec_mov, pio_y));
}

void dvd_drv_gpio_irq(void) {
    uint32_t reset_status = gpio_get_irq_event_mask(pin_gc_reset);

    // reset
    if (reset_status & GPIO_IRQ_LEVEL_LOW) {
        // force dstrb high until we are done
        gpio_set_outover(pin_gc_dstrb, GPIO_OVERRIDE_HIGH);

        // reset error
        pio_sm_exec(pio0, PIO_RECV_SM, pio_encode_set(pio_pins, 1));

        // wait for high signal next
        gpio_set_irq_enabled(pin_gc_reset, GPIO_IRQ_LEVEL_LOW, false);
        gpio_set_irq_enabled(pin_gc_reset, GPIO_IRQ_LEVEL_HIGH, true);

    } else if (reset_status & GPIO_IRQ_LEVEL_HIGH) {
        // wait for low signal next
        gpio_set_irq_enabled(pin_gc_reset, GPIO_IRQ_LEVEL_HIGH, false);
        gpio_set_irq_enabled(pin_gc_reset, GPIO_IRQ_LEVEL_LOW, true);

        // perform the reset
        dvd_drv_clear_state();
        dvd_reset();

        // release dstrb
        gpio_set_outover(pin_gc_dstrb, GPIO_OVERRIDE_NORMAL);
    }

    uint32_t brk_status = gpio_get_irq_event_mask(pin_gc_brk);
    if (brk_status & GPIO_IRQ_EDGE_RISE) {
        printf("brk rise\n");
        gpio_acknowledge_irq(pin_gc_brk, GPIO_IRQ_EDGE_RISE);
    }
    if(brk_status & GPIO_IRQ_EDGE_FALL) {
        printf("brk fall\n");
        gpio_acknowledge_irq(pin_gc_brk, GPIO_IRQ_EDGE_FALL);
    }
}

// dir changed to drive -> host
void dvd_drv_dir_irq_out(void) {
    pio_sm_set_enabled(pio0, PIO_RECV_SM, false);

    if (pio_sm_get_rx_fifo_level(pio0, PIO_RECV_SM) >= 3) {
        static uint32_t recv_buf[3];
        for (int i = 0; i < 3; i++)
            recv_buf[i] = __builtin_bswap32(pio0->rxf[PIO_RECV_SM]);

        dvd_request((uint8_t*)recv_buf);
    } else {
        dvd_request(NULL);
    }    

    // reset recv pio
    pio_sm_exec(pio0, PIO_RECV_SM, pio_encode_mov(pio_exec_mov, pio_y));
    pio_sm_clear_fifos(pio0, PIO_RECV_SM);
    pio_sm_exec(pio0, PIO_RECV_SM, pio_encode_mov(pio_isr, pio_null));

    pio_interrupt_clear(pio0, 1);
}

// dir changed to host -> drive
void dvd_drv_dir_irq_in(void) {
    // stop remaining send
    dma_channel_abort(send_chan);

    // reset send pio
    pio_sm_clear_fifos(pio0, PIO_SEND_SM);
    pio_sm_exec(pio0, PIO_SEND_SM, pio_encode_mov(pio_exec_mov, pio_y));
    pio_sm_restart(pio0, PIO_SEND_SM);

    pio_sm_set_enabled(pio0, PIO_RECV_SM, true);

    pio_sm_exec(pio0, PIO_DIR_SM, pio_encode_mov(pio_exec_mov, pio_y));
    pio_interrupt_clear(pio0, 0);
}

void dvd_drv_send(const void* data, uint32_t len) {
    assert((data & 0b11) == 0 && (len & 0b11) == 0);
    dma_channel_set_read_addr(send_chan, data, false);
    dma_channel_set_trans_count(send_chan, len / 4, true);
}

void dvd_drv_set_error() {
    pio_sm_exec(pio0, PIO_RECV_SM, pio_encode_set(pio_pins, 0));
}

void dvd_drv_enable_passthrough()
{
    //irq_set_enabled(IO_IRQ_BANK0, false);
    gpio_set_irq_enabled(pin_gc_reset, GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH, false);
    irq_set_enabled(PIO0_IRQ_0, false);
    irq_set_enabled(PIO0_IRQ_1, false);

    // change data lines to input
    pio_sm_exec(pio0, PIO_DIR_SM, pio_encode_mov(pio_exec_mov, pio_x));

    pio_sm_set_enabled(pio0, PIO_SEND_SM, false);
    pio_sm_set_enabled(pio0, PIO_RECV_SM, false);

    pio_sm_set_enabled(pio0, PIO_PASSTHROUGH_SM, true);
    gpio_set_function(pin_gc_dstrb, GPIO_FUNC_GPCK);

    gpio_set_outover(pin_dvd_dir, GPIO_OVERRIDE_NORMAL);
    gpio_set_outover(pin_gc_cover, GPIO_OVERRIDE_NORMAL);
    gpio_set_outover(pin_dvd_reset, GPIO_OVERRIDE_NORMAL);
}
