/* Boton - un pulsador a GND en P0_7 (pull-up interno) enciende un LED en P1_2. */
#define BOTON P0_7
#define LED P1_2

void setup() {
  pinMode(BOTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
}

void loop() {
  digitalWrite(LED, digitalRead(BOTON) == LOW ? HIGH : LOW);
}
