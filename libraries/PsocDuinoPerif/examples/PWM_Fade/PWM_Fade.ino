/* PWM_Fade - un LED en P1_6 (con resistencia de 330 ohm a GND) sube y baja de brillo.
 * El PWM es por hardware (User Module PWM8): 31,25 kHz, 0..255. */
#include <PsocDuinoPerif.h>

#define LED_PWM P1_6

void setup() {
}

void loop() {
  unsigned char v;
  for (v = 0; v < 255; v++) {
    analogWrite(LED_PWM, v);
    delay(4);
  }
  for (v = 255; v > 0; v--) {
    analogWrite(LED_PWM, v);
    delay(4);
  }
}
