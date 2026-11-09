/* DAC_Rampa - rampa de 0 V a Vdd en P0_5 con el DAC (vease con el osciloscopio).
 * Placas "+ perifericos": DAC6 (24423A, 64 escalones) o DAC8 (256 escalones).
 * Demas placas: DAC6 por registros (64 escalones). CY8C21x34: sin DAC.
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
