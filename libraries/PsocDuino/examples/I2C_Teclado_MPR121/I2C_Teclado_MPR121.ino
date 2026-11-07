/* I2C_Teclado_MPR121 - teclado tactil de 9 teclas con MPR121 (direccion 0x5A),
 * como el ejemplo PSoC1duino_Touch de PSoC Designer, pero sin User Modules:
 * I2C por software en P1_5 (SDA) / P1_7 (SCL).
 * Al tocar una tecla, el LED de P1_6 parpadea tantas veces como su numero.
 * Patas para los chips de 28 pines (CY8C24423A, CY8C27443, CY8C29466); en los de
 * 16/20 pines (CY8C21234, CY8C24223A, CY8C27243) elija otras que existan en su encapsulado.
 */
#define LED P1_6
#define MPR121 0x5A

/* electrodo de cada tecla 1..9 (igual que touchpad.c) */
static const unsigned char tecla_electrodo[9] = {8, 5, 2, 7, 4, 1, 6, 3, 0};
/* filtros de las secciones A y B de touchpad.c: registro, valor */
static const unsigned char mpr_cfg[18] = {
  0x2B, 0x01, 0x2C, 0x01, 0x2D, 0x00, 0x2E, 0x00,   /* datos > linea base */
  0x2F, 0x01, 0x30, 0x01, 0x31, 0xFF, 0x32, 0x02,   /* datos < linea base */
  0x5B, 0x33                                        /* antirrebote 3/3 */
};

void mpr_write(unsigned char reg, unsigned char val) {
  i2c_start();
  i2c_write(MPR121 << 1);
  i2c_write(reg);
  i2c_write(val);
  i2c_stop();
}

unsigned int mpr_touch(void) {
  unsigned char lo, hi;
  i2c_start();
  i2c_write(MPR121 << 1);
  i2c_write(0x00);                     /* estado de los electrodos 0-7 y 8-11 */
  i2c_start();
  i2c_write((MPR121 << 1) | 1);
  lo = i2c_read(1);
  hi = i2c_read(0);
  i2c_stop();
  return ((unsigned int)hi << 8) | lo;
}

void mpr_begin(void) {
  unsigned char r;
  mpr_write(0x5E, 0x00);               /* modo reposo para configurar */
  for (r = 0; r < sizeof(mpr_cfg); r += 2) mpr_write(mpr_cfg[r], mpr_cfg[r + 1]);
  for (r = 0x41; r < 0x59; r += 2) {   /* umbral de toque 0x0F y de soltar 0x0A */
    mpr_write(r, 0x0F);
    mpr_write((unsigned char)(r + 1), 0x0A);
  }
  mpr_write(0x5D, 0x04);               /* filtro ESI2 */
  mpr_write(0x5E, 0x0A);               /* electrodos 0-9 en marcha */
}

unsigned char leer_tecla(void) {
  unsigned int t = mpr_touch();
  unsigned char k;
  for (k = 0; k < 9; k++)
    if (t & (1U << tecla_electrodo[k])) return (unsigned char)(k + 1);
  return 0;
}

void setup() {
  pinMode(LED, OUTPUT);
  i2c_begin();
  mpr_begin();
}

void loop() {
  static unsigned char anterior = 0;
  unsigned char k = leer_tecla(), i;
  if (k != anterior && k != 0) {
    for (i = 0; i < k; i++) {
      digitalWrite(LED, HIGH);
      delay(150);
      digitalWrite(LED, LOW);
      delay(150);
    }
  }
  anterior = k;
  delay(20);
}
