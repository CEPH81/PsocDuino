/* Prueba en PC del nucleo (registros simulados). gcc -std=c89 -pedantic */
#include <stdio.h>
#include "Arduino.h"
unsigned char psd_host_regs[512];
void setup(void) {}
void loop(void) {}
static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FALLO linea %d: %s\n", __LINE__, #c); fails++; } } while (0)
static int dm(int port, int bit) {
    int m = 1 << bit;
    return ((psd_host_regs[0x003 + 4 * port] & m) ? 4 : 0) | ((psd_host_regs[0x101 + 4 * port] & m) ? 2 : 0) |
           ((psd_host_regs[0x100 + 4 * port] & m) ? 1 : 0);
}
int main(void) {
    pinMode(P1_2, OUTPUT);        CHECK(dm(1, 2) == 1);
    digitalWrite(P1_2, HIGH);     CHECK(psd_host_regs[0x004] == 0x04);
    digitalWrite(P1_3, HIGH);     CHECK(psd_host_regs[0x004] == 0x0C);
    digitalWrite(P1_2, LOW);      CHECK(psd_host_regs[0x004] == 0x08);
    digitalToggle(P1_3);          CHECK(psd_host_regs[0x004] == 0x00);
    pinMode(P0_7, INPUT_PULLUP);  CHECK(dm(0, 7) == 3); CHECK(psd_host_regs[0x000] & 0x80);
    pinMode(P2_0, INPUT);         CHECK(dm(2, 0) == 2);
    pinMode(P2_1, INPUT_PULLDOWN); CHECK(dm(2, 1) == 0);
    pinMode(P2_2, OUTPUT_OPEN_DRAIN); CHECK(dm(2, 2) == 7);
    pinMode(P2_3, INPUT_ANALOG);  CHECK(dm(2, 3) == 6);
    CHECK(dm(2, 0) == 2 && dm(2, 1) == 0);          /* otros bits intactos */
    psd_host_regs[0x008] = 0x01;  CHECK(digitalRead(P2_0) == HIGH); CHECK(digitalRead(P2_1) == LOW);
    pinMode(40, OUTPUT); digitalWrite(40, HIGH);     /* puerto 5 > PSOC_NUM_PORTS: ignorado */
    CHECK(psd_host_regs[0x014] == 0);
    CHECK(map(512, 0, 1023, 0, 255) == 127);
    CHECK(constrain(300, 0, 255) == 255);
    printf(fails ? "%d fallos\n" : "nucleo: todas las pruebas OK\n", fails);
    return fails != 0;
}
