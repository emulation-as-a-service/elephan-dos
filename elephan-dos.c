// SPDX-License-Identifier: Unlicense

#include <stdint.h>

asm(".code16gcc");

#define NAME "ELEPHAN-DOS"

static __attribute__((section(".text"))) char start_message[] =
    "\r\nStarting " NAME "...\r\n\r\n";

// General helper functions
// See: https://en.wikipedia.org/wiki/BIOS_interrupt_call
// See: https://www.ctyme.com/intr/int.htm

static void int10(uint8_t ah, uint8_t al) {
  uint16_t ax = ah << 8 | al;
  asm volatile("int $0x10" ::"a"(ax));
}

static uint8_t inb(uint16_t port) {
  uint8_t al;
  asm volatile("inb %1, %0" : "=a"(al) : "d"(port));
  return al;
}

static void outb(uint16_t port, uint8_t value) {
  asm volatile("outb %0, %1" : : "a"(value), "d"(port));
}

// Time functions

static uint16_t time() {
  uint16_t ax = 0, cx, dx;
  asm volatile("int $0x1a" : "+a"(ax), "=c"(cx), "=d"(dx));
  return dx;
}

static void sleep1s() {
  uint16_t timeStart = time();
  while ((time() - timeStart) < 18)
    ;
}

// Output functions

static void put(char c) { int10(0x0e, c); }
static void com(char c) { outb(0x3f8, c); }

static void hexOut(void(outFn)(char c), uint8_t value) {
  outFn(value > 9 ? value + 0x57 : value + 0x30);
}

static char toHexDigit(uint8_t value) {
  return value > 9 ? value + 0x57 : value + 0x30;
}

static void putX(char c) { put(c > 9 ? c + 0x57 : c + 0x30); }
static void comX(char c) { com(c > 9 ? c + 0x57 : c + 0x30); }

static void nl(void(outFn)(char c)) {
  outFn('\r');
  outFn('\n');
}

static void printHex(void(outFn)(char c), uint8_t value) {
  outFn('0');
  outFn('x');
  outFn(toHexDigit((value >> 4) & 0xf));
  outFn(toHexDigit(value & 0xf));
  nl(outFn);
}

static void printHex32(void(outFn)(char c), uint32_t value) {
  for (int i = 0; i < 4; i++) {
    printHex(outFn, value);
    value = value >> 8;
  }
}

static void printString(void(outFn)(char c), char *str) {
  while (*str) {
    outFn(*str);
    str++;
  }
}

// Video BIOS functions

static uint8_t biosGetMode() {
  uint16_t ax = 0x0f00, bx;
  asm volatile("int $0x10" : "+a"(ax), "=b"(bx));
  return ax;
}

static void vbeSetMode(uint16_t mode) {
  asm volatile("int $0x10" ::"a"((uint16_t)0x4f02), "b"(mode));
}

static void draw(uint16_t x, uint16_t y, uint8_t color) {
  uint16_t ax = 0x0c << 8 | color;
  asm volatile("int $0x10" ::"c"(x), "d"(y), "a"(ax), "b"(0));
}

static void drawBox(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h,
                    uint16_t color) {
  uint16_t x1 = x0 + w, y1 = y0 + h;
  for (uint16_t x = x0; x < x1; x++)
    for (uint16_t y = y0; y < y1; y++)
      draw(x, y, color ? y : x);
}

// Sound functions

static void sound(uint8_t div) {
  outb(0x43, 0b10110110);
  outb(0x42, (uint8_t)0);
  outb(0x42, (uint8_t)div);
  outb(0x61, inb(0x61) | 0b11);
}

// Main program

int main(void) {
  com(biosGetMode());
  printString(put, start_message);
  sound(30);
  sleep1s();
  asm volatile("int $0x10" ::"a"((uint16_t)0x0b00), "b"((uint16_t)0x0021));
  com(biosGetMode());
  sleep1s();

  for (int i = 0; i < 2; i++) {
    vbeSetMode(0x0100);
    com(biosGetMode());
    sound(40);
    sleep1s();
    drawBox(0, 0, 100, 30, 0);
    sleep1s();
    // vbeSetMode(0x0100 | 1 << 15);
    vbeSetMode(0x0115);
    sound(60);
    sleep1s();
    drawBox(200, 0, 100, 30, 1);
    sleep1s();
    // vbeSetMode(0x0115);
    // sleep1s();
    // drawBox(100, 0, 20, 90, 1);
    // sleep1s();
    // vbeSetMode(0x010d);
    // sleep1s();
  }
  com('#');
  asm volatile("int $0x19");
}
