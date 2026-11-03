/* CalibrarDelay - mida con el osciloscopio:
 *   P1_2: pulso de delayMicroseconds(1000)  -> debe durar 1.000 ms
 *   P1_3: cambia cada millis() % 100 == 0   -> periodo de 200 ms
 * Si P1_2 no mide 1 ms, ajuste PSOCDUINO_CALL_CYCLES (platform.local.txt:
 * compiler.c.extra_flags=-DPSOCDUINO_CALL_CYCLES=NN) y avise el valor. */
#define PULSO P1_2
#define TIC P1_3

unsigned long ultimo = 0;

void setup() {
  pinMode(PULSO, OUTPUT);
  pinMode(TIC, OUTPUT);
}

void loop() {
  unsigned long ahora;
  digitalWrite(PULSO, HIGH);
  delayMicroseconds(1000);
  digitalWrite(PULSO, LOW);
  delayMicroseconds(1000);
  ahora = millis();
  if (ahora - ultimo >= 100) {
    ultimo = ahora;
    digitalToggle(TIC);
  }
}
