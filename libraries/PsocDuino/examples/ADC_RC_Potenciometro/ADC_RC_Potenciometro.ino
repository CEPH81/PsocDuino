/* ADC_RC_Potenciometro - "leer" un potenciometro, LDR o termistor SIN bloques
 * analogicos, midiendo el tiempo de carga de un condensador (cualquier pata).
 *
 *   Vdd --[ potenciometro 10k..100k como resistencia variable ]--+-- P1_5
 *                                                                 |
 *                                                          100 nF ===
 *                                                                 |
 *                                                                GND
 *   y una resistencia de 1 k en serie con el potenciometro (protege la pata
 *   cuando descarga el condensador con el potenciometro al minimo).
 *
 * La pata descarga el condensador (OUTPUT LOW), luego pasa a entrada y cuenta
 * vueltas hasta leer HIGH (~0,5..0,6 Vdd): mas resistencia -> mas cuenta.
 * Es aproximado (depende de Vdd, del condensador y de la temperatura), pero
 * suficiente para una perilla. El LED en P1_2 parpadea mas lento con mas cuenta.
 * Para una medida en voltios use analogRead (biblioteca PsocDuinoPerif). */

#define RC_PIN P1_5
#define LED    P1_2

static unsigned int rc_leer(void)
{
    unsigned int n = 0;
    pinMode(RC_PIN, OUTPUT);
    digitalWrite(RC_PIN, LOW);
    delay(5);                                  /* descarga el condensador */
    pinMode(RC_PIN, INPUT);
    while (!digitalRead(RC_PIN))
        if (++n == 60000U) break;              /* sin potenciometro: no colgarse */
    return n;
}

void setup()
{
    pinMode(LED, OUTPUT);
}

void loop()
{
    unsigned int n = rc_leer();
    digitalToggle(LED);
    delay((n >> 3) + 20);                      /* ajuste la escala a su circuito */
}
