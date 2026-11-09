/* PWM_Software - brillo variable de un LED en CUALQUIER pata, sin bloques digitales.
 * La CPU genera la senal (~490 Hz): sirve para LEDs; para motores o audio use
 * analogWrite (PWM por hardware, biblioteca PsocDuinoPerif).
 * LED con resistencia de 330 ohm entre P1_2 y GND. */

#define LED P1_2

/* un periodo de ~2 ms con 'brillo' de 0 a 255 (pasos de 8 us) */
static void pwm_periodo(uint8_t brillo)
{
    if (brillo) {
        digitalWrite(LED, HIGH);
        delayMicroseconds((unsigned int)brillo << 3);
    }
    if (brillo != 255) {
        digitalWrite(LED, LOW);
        delayMicroseconds((unsigned int)(255 - brillo) << 3);
    }
}

/* mantiene un brillo durante 'n' periodos */
static void pwm_mantener(uint8_t brillo, uint8_t n)
{
    while (n--) pwm_periodo(brillo);
}

void setup()
{
    pinMode(LED, OUTPUT);
}

void loop()
{
    unsigned int b;
    for (b = 0; b <= 255; b += 5) pwm_mantener((uint8_t)b, 8);   /* sube */
    for (b = 255; b > 0; b -= 5) pwm_mantener((uint8_t)b, 8);    /* baja */
}
