/* perif_pwm.c - analogWrite con el User Module PWM8 (una salida PWM por placa). */
#include "PsocDuinoPerif.h"
#include "psocduino_perif.h"

#if PSD_HAS_PWM

static unsigned char pwm_on;

void analogWrite(uint8_t pin, uint8_t value)
{
    if (pin != PSD_PWM_PIN) {
        /* otras patas: como en Arduino, alto desde la mitad */
        pinMode(pin, OUTPUT);
        digitalWrite(pin, value >= 128 ? HIGH : LOW);
        return;
    }
    if (!pwm_on) {
        PSD_PWM_START();
        pwm_on = 1;
    }
    PSD_PWM_WRITE(value);
}

#endif
