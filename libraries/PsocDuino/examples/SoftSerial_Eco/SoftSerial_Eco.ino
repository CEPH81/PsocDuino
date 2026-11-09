/* SoftSerial_Eco - puerto serie por software (bit-bang), en cualquier pata y en
 * cualquier PSoC 1, sin bloques digitales. 9600 baudios 8N1.
 *
 * Conexion con un adaptador USB-serie de la misma tension que el PSoC:
 *   TX (P1_4) -> RX del adaptador     RX (P1_3) <- TX del adaptador     GND comun
 * Abra el Monitor Serie a 9600: el PSoC saluda y devuelve cada caracter.
 *
 * Mientras envia o espera un byte la CPU no hace otra cosa y las interrupciones
 * se apagan durante el byte (millis() no pierde el tic: se atiende al terminar).
 * Si los caracteres llegan mal, mida un bit con osciloscopio (104 us a 9600) y
 * ajuste AJUSTE_US. Para algo mas rapido use la biblioteca PsocDuinoPerif (UART
 * por hardware). */

#define TX_PIN    P1_4
#define RX_PIN    P1_3
#define BAUD      9600UL
#define BIT_US    ((unsigned int)(1000000UL / BAUD))
/* tiempo de las instrucciones de cada bit (crece al bajar la frecuencia) */
#define AJUSTE_US ((unsigned int)(3UL * 24000000UL / F_CPU))

static uint8_t tx_a, tx_m, rx_a, rx_m;

static void soft_begin(void)
{
    pinMode(TX_PIN, OUTPUT);
    digitalWrite(TX_PIN, HIGH);                /* reposo en alto */
    pinMode(RX_PIN, INPUT_PULLUP);
    tx_a = (uint8_t)((TX_PIN >> 3) << 2);      /* registro de datos del puerto */
    tx_m = psocduino_mask(TX_PIN);
    rx_a = (uint8_t)((RX_PIN >> 3) << 2);
    rx_m = psocduino_mask(RX_PIN);
}

static void soft_write(uint8_t c)
{
    uint8_t i, hi, lo;
    hi = (uint8_t)(psocduino_shadow[TX_PIN >> 3] | tx_m);
    lo = (uint8_t)(psocduino_shadow[TX_PIN >> 3] & (uint8_t)~tx_m);
    noInterrupts();
    PSD_WR(tx_a, lo);                          /* bit de inicio */
    delayMicroseconds(BIT_US - AJUSTE_US);
    for (i = 0; i < 8; i++) {                  /* LSB primero */
        PSD_WR(tx_a, (c & 1) ? hi : lo);
        c >>= 1;
        delayMicroseconds(BIT_US - AJUSTE_US);
    }
    PSD_WR(tx_a, hi);                          /* bit de parada */
    interrupts();
    delayMicroseconds(BIT_US);
}

static void soft_print(const char *s)
{
    while (*s) soft_write((uint8_t)*s++);
}

/* espera un byte hasta 'espera' vueltas; -1 si no llega */
static int soft_read(unsigned int espera)
{
    uint8_t i, c = 0;
    while (psocduino_reg_rd(rx_a) & rx_m)      /* esperar el bit de inicio */
        if (--espera == 0) return -1;
    noInterrupts();
    delayMicroseconds((BIT_US >> 1) - AJUSTE_US);   /* mitad del bit de inicio */
    for (i = 0; i < 8; i++) {
        delayMicroseconds(BIT_US - AJUSTE_US);
        c >>= 1;
        if (psocduino_reg_rd(rx_a) & rx_m) c |= 0x80;
    }
    interrupts();
    delayMicroseconds(BIT_US >> 1);            /* hasta el bit de parada */
    return c;
}

void setup()
{
    soft_begin();
    soft_print("PsocDuino: serie por software a 9600\r\n");
}

void loop()
{
    int c = soft_read(60000U);
    if (c >= 0) soft_write((uint8_t)c);
}
