/* Potenciometro_ADC - lee un potenciometro y envia por Serial (115200) el valor del
 * ADC, los milivoltios y el porcentaje de giro, cada 200 ms.
 *
 *   Vdd ---[ potenciometro 1k..100k ]--- GND
 *                    |
 *                  cursor ---> P0_3      (entradas validas: P0_1, P0_3, P0_7)
 *
 * Placas "+ perifericos" (ADC de 12 bits, rango 0 V..Vdd). La tension de Vdd se
 * toma del menu Herramientas -> Alimentacion del chip. */
#include <PsocDuinoPerif.h>

#define POT P0_3
#if PSOC_VCC == 3
#define VDD_MV 3300L
#else
#define VDD_MV 5000L
#endif

/* promedio de 8 lecturas de 12 bits (0..4095): menos ruido */
static unsigned int leer_promedio(void)
{
  unsigned int suma = 0;
  unsigned char i;
  for (i = 0; i < 8; i++) suma += (unsigned int)analogRead(POT);
  return suma >> 3;
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  Serial.println("Potenciometro en P0_3");
}

void loop() {
  unsigned int v = leer_promedio();
  long mv = ((long)v * VDD_MV) >> 12;             /* 0..Vdd en mV */
  Serial.print("ADC = ");
  Serial.printNum(v);
  Serial.print("   ");
  Serial.printNum(mv);
  Serial.print(" mV   ");
  Serial.printNum(((long)v * 100) / 4095);         /* 0..100 % */
  Serial.println(" %");
  delay(200);
}
