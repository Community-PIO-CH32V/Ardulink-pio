#include "swio.h"

// Timings.
// T = 1 / 8 MHz
// 0 = T-4T low, T-16T high
// 1 = 6T-64T low, T-16T high

// Note: bitwise ops take up 2 clock cycles (1T).
// nop are 1 cycle (0.5T).

static inline void swio_send_one() {
    /* 0.0T */ SWIO_DDR |= _BV(SWIO_BIT);
    /* 1.0T */ SWIO_PORT &= ~_BV(SWIO_BIT);
    /* 2.0T */ asm("nop"); asm("nop");
    /* 3.0T */ SWIO_PORT |= _BV(SWIO_BIT);
    /* 4.0T */ SWIO_DDR &= ~SWIO_BIT;
}

static inline void swio_send_zero() {
    /* 0.0T */ SWIO_DDR |= _BV(SWIO_BIT);
    /* 1.0T */ SWIO_PORT &= ~_BV(SWIO_BIT);
    /* 2.0T */ asm("nop"); asm("nop");
    /* 3.0T */ asm("nop"); asm("nop");
    /* 4.0T */ asm("nop"); asm("nop");
    /* 5.0T */ asm("nop"); asm("nop");
    /* 6.0T */ asm("nop"); asm("nop");
    /* 7.0T */ asm("nop"); asm("nop");
    /* 8.0T */ SWIO_PORT |= _BV(SWIO_BIT);
    /* 9.0T */ SWIO_DDR &= ~SWIO_BIT;
}

static inline char swio_recv_bit() {
    char x;
    /* 0.0T */ SWIO_DDR |= _BV(SWIO_BIT);
    /* 1.0T */ SWIO_PORT &= ~_BV(SWIO_BIT);
    /* 2.0T */ SWIO_PORT |= _BV(SWIO_BIT); // Precharge when the line is floating.
    /* 3.0T */ SWIO_DDR &= ~_BV(SWIO_BIT);
    /* 4.0T */ asm("nop"); asm("nop");
    /* 5.0T */ asm("nop"); asm("nop");
    /* 6.0T */ x = SWIO_PIN;

    // Wait for the line to come back up if it's down.
    while (!(SWIO_PIN & _BV(SWIO_BIT)))
        ;

    return x & _BV(SWIO_BIT);
}

// Write a register.
void swio_write_reg(uint8_t addr, uint32_t val) {
    char i;

    // Start bit.
    swio_send_one();

    // Send the address.
    for (i = 0; i < 7; i++) {
        if (addr & 0x40)
            swio_send_one();
        else
            swio_send_zero();

        addr <<= 1;
    }

    // Start bit.
    swio_send_one();

    // Send the word.
    for (i = 0; i < 32; i++) {
        if (val & 0x80000000)
            swio_send_one();
        else
            swio_send_zero();

        val <<= 1;
    }

    // Stop bit.
    _delay_us(10);
}

// Read a register.
uint32_t swio_read_reg(uint8_t addr) {
    char i;
    uint32_t x = 0;

    // Start bit.
    swio_send_one();

    // Send the address.
    for (i = 0; i < 7; i++) {
        if (addr & 0x40)
            swio_send_one();
        else
            swio_send_zero();

        addr <<= 1;
    }

    // Start bit.
    swio_send_zero();

    // Receive the word.
    for (i = 0; i < 32; i++) {
        x <<= 1;

        if (swio_recv_bit())
            x |= 1;
    }

    // Stop bit.
    _delay_us(10);

    return x;
}

void swio_init() {
    SWIO_PORT |= _BV(SWIO_BIT);
    SWIO_DDR |= _BV(SWIO_BIT);
    _delay_ms(5);
    SWIO_PORT &= ~_BV(SWIO_BIT);
    _delay_ms(20);
    SWIO_PORT |= _BV(SWIO_BIT);
    SWIO_DDR &= ~_BV(SWIO_BIT);
}
