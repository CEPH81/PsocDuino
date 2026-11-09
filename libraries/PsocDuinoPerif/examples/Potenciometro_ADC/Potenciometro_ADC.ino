/* Potenciometro_ADC - lee un potenciometro en P0_3 (extremos a Vdd y GND, cursor a P0_3)
 * y envia el valor (0..1023) por Serial cada 200 ms. Entradas validas: P0_1, P0_3, P0_7.
 * Placas "+ perifericos": ADC de 12 bits. Demas placas: SAR de 6 bits por registros
 * (64 niveles, pasos de 16 en 0..1023). CY8C21x34: sin ADC (devuelve 0). */
#include <PsocDuinoPerif.h>

void setup() {
  Serial.begin(115200);
}

void loop() {
  int v = analogRead(P0_3);
  Serial.print("P0_3 = ");
  Serial.printNum(v);
  Serial.print("  (");
  Serial.printNum((long)v * 5000 / 1023);   /* milivoltios con Vdd = 5 V */
  Serial.println(" mV)");
  delay(200);
}
