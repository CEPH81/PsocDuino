/* LM35_Temperatura - temperatura con un LM35 (10 mV por grado C) enviada por Serial
 * (115200) cada 500 ms, con un decimal. Sin numeros de coma flotante (poca flash).
 *
 *   LM35 (vista frontal, patas hacia abajo):  +Vs   Vout   GND
 *                                              |     |      |
 *                                             Vdd   P0_1   GND
 *   Opcional: 100 nF de Vout a GND y 1 k en serie entre Vout y P0_1 (menos ruido).
 *
 * El LM35 necesita 4 V o mas: use Vdd = 5 V (menu Alimentacion del chip = 5 V).
 * Rango de este circuito: +2 C .. 150 C. Con el ADC de 12 bits a 5 V cada paso
 * es de 1,22 mV = 0,12 C; se promedian 16 lecturas.
 * Placas "+ perifericos". Entradas validas: P0_1, P0_3, P0_7. */
#include <PsocDuinoPerif.h>

#define SENSOR P0_1
#if PSOC_VCC == 3
#define VDD_MV 3300L
#else
#define VDD_MV 5000L
#endif

/* suma de 16 lecturas de 12 bits (0..65520) */
static unsigned int leer_suma16(void)
{
  unsigned int suma = 0;
  unsigned char i;
  for (i = 0; i < 16; i++) suma += (unsigned int)analogRead(SENSOR);
  return suma;
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  Serial.println("LM35 en P0_1");
}

void loop() {
  /* mV x 10 = suma * Vdd(mV) * 10 / (16 * 4096); 10 mV/C -> decimas de grado = mV */
  long decimas = ((long)leer_suma16() * VDD_MV) >> 16;   /* = mV = decimas de C */
  Serial.print("T = ");
  Serial.printNum(decimas / 10);
  Serial.print(".");
  Serial.printNum(decimas % 10);
  Serial.println(" C");
  delay(500);
}
