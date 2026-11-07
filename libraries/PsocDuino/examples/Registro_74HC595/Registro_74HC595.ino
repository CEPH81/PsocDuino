/* Registro_74HC595 - contador binario en 8 LEDs con un 74HC595 (salida serie
 * tipo SPI con shiftOut, sin User Module SPIM):
 *   P1_3 = DS (datos), P1_4 = SH_CP (reloj), P1_2 = ST_CP (latch).
 * Patas para los chips de 28 pines (CY8C24423A, CY8C27443, CY8C29466); en los de
 * 16/20 pines (CY8C21234, CY8C24223A, CY8C27243) elija otras que existan en su encapsulado.
 */
#define DATOS P1_3
#define RELOJ P1_4
#define LATCH P1_2

void setup() {
  pinMode(DATOS, OUTPUT);
  pinMode(RELOJ, OUTPUT);
  pinMode(LATCH, OUTPUT);
}

void loop() {
  static unsigned char n = 0;
  digitalWrite(LATCH, LOW);
  shiftOut(DATOS, RELOJ, MSBFIRST, n++);
  digitalWrite(LATCH, HIGH);           /* las 8 salidas cambian a la vez */
  delay(200);
}
