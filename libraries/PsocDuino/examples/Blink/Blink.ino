/* Blink - parpadea un LED en P1_2 (cambie el pin a su placa).
 * PsocDuino compila con ImageCraft: el sketch debe ser C (sin clases). */
#define LED P1_2

void setup() {
  pinMode(LED, OUTPUT);
}

void loop() {
  digitalWrite(LED, HIGH);
  delay(500);
  digitalWrite(LED, LOW);
  delay(500);
}
