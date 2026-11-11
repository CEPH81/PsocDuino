/* DAC_Rampa - rampa de 0 V a Vdd en P0_5 con el DAC (vease con el osciloscopio).
 * CY8C24423A: DAC6 (64 escalones); CY8C24223A: DAC8 (256 escalones).
 * Ponga un potenciometro en P0_3 para cambiar la velocidad de la rampa. */
#include <PsocDuinoPerif.h>

void setup() {
}

void loop() {
  unsigned char v = 0;
  unsigned int espera = (unsigned int)analogRead(P0_3) * 4;   /* 0..4 ms por escalon */
  do {
    dacWrite(v);
    delayMicroseconds(espera);
  } while (++v);
}
