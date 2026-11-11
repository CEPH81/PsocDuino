/* NTC_Temperatura - temperatura con un termistor NTC de 10 k (B = 3950) en un divisor
 * con una resistencia fija de 10 k, enviada por Serial (115200) cada 500 ms.
 *
 *   Vdd ---[ 10 k fija ]---+---[ NTC 10 k ]--- GND
 *                          |
 *                        P0_7          (entradas validas: P0_1, P0_3, P0_7)
 *   Opcional: 100 nF de P0_7 a GND (menos ruido).
 *
 * La medida es proporcional a Vdd (no importa si es 5 V o 3,3 V). En lugar del
 * logaritmo (que necesitaria coma flotante y mucha flash) se usa una tabla de
 * -20 C a 105 C cada 5 C con interpolacion lineal: error < 0,3 C en todo el rango.
 * Para otro NTC o resistencia fija recalcule TABLA con:
 *   adc = 4096 * Rntc / (Rntc + Rfija),  Rntc = R25 * exp(B * (1/(T+273,15) - 1/298,15))
 * Placas "+ perifericos". */
#include <PsocDuinoPerif.h>

#define SENSOR   P0_7
#define T_MIN    (-20)          /* grados de la primera entrada de la tabla */
#define T_PASO   5              /* grados entre entradas */
#define N_TABLA  26

/* ADC de 12 bits esperado a -20, -15, ... 105 C (en flash) */
static const unsigned int TABLA[N_TABLA] = {
  3741, 3630, 3496, 3338, 3157, 2956, 2739, 2511, 2278, 2048, 1825, 1614, 1419,
  1241, 1082,  940,  816,  707,  613,  532,  462,  401,  350,  305,  267,  234
};

static unsigned int leer_promedio(void)
{
  unsigned int suma = 0;
  unsigned char i;
  for (i = 0; i < 16; i++) suma += (unsigned int)analogRead(SENSOR);
  return suma >> 4;
}

/* decimas de grado; -1000 = sensor abierto o en corto */
static int ntc_decimas(unsigned int v)
{
  unsigned char i;
  unsigned int a, b;
  if (v >= 4000 || v <= 100) return -1000;
  if (v >= TABLA[0]) return T_MIN * 10;                     /* mas frio que la tabla */
  for (i = 1; i < N_TABLA; i++) {
    b = TABLA[i];
    if (v >= b) {
      a = TABLA[i - 1];
      /* interpolar entre (i-1) e i: 50 decimas por paso de 5 C */
      return (int)((T_MIN + (int)(i - 1) * T_PASO) * 10 +
                   (int)(((unsigned long)(a - v) * (T_PASO * 10)) / (a - b)));
    }
  }
  return (T_MIN + (N_TABLA - 1) * T_PASO) * 10;             /* mas caliente */
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  Serial.println("NTC 10k en P0_7");
}

void loop() {
  unsigned int v = leer_promedio();
  int t = ntc_decimas(v);
  Serial.print("ADC = ");
  Serial.printNum(v);
  if (t == -1000) {
    Serial.println("   sensor desconectado o en corto");
  } else {
    Serial.print("   T = ");
    if (t < 0) { Serial.print("-"); t = -t; }
    Serial.printNum(t / 10);
    Serial.print(".");
    Serial.printNum(t % 10);
    Serial.println(" C");
  }
  delay(500);
}
