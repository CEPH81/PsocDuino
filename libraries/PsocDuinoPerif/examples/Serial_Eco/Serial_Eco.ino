/* Serial_Eco - eco por el UART a 115200 baudios con un adaptador USB-serie (3,3/5 V).
 * Placa "PSoC 1 CY8C24423A + perifericos": TX P2_1, RX P2_0.
 * Placa "PSoC 1 CY8C24223A + perifericos": TX P1_4, RX P1_2.
 * Una el TX del PSoC al RX del adaptador, el RX al TX y las tierras. */
#include <PsocDuinoPerif.h>

void setup() {
  Serial.begin(115200);
  Serial.println("PsocDuino: escriba algo");
}

void loop() {
  int c = Serial.read();
  if (c >= 0) {
    Serial.print("recibido: ");
    Serial.write((unsigned char)c);
    Serial.print("  codigo ");
    Serial.printlnNum(c);
  }
}
