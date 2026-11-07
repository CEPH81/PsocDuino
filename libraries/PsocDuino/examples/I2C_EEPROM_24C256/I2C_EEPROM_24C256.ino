/* I2C_EEPROM_24C256 - cuenta los arranques en una EEPROM 24C256 (direccion 0x50),
 * como el ejemplo Example_EEPROM_24C256 de Cypress, con I2C por software
 * en P1_5 (SDA) / P1_7 (SCL). El LED de P1_6 parpadea (arranques % 10) veces;
 * un destello largo indica que la EEPROM no respondio.
 * Patas para los chips de 28 pines (CY8C24423A, CY8C27443, CY8C29466); en los de
 * 16/20 pines (CY8C21234, CY8C24223A, CY8C27243) elija otras que existan en su encapsulado.
 */
#define LED P1_6
#define EEPROM_DIR 0x50

/* espera el fin de la grabacion interna (sondeo por ACK, max ~10 ms) */
unsigned char ee_lista(void) {
  unsigned char n;
  for (n = 0; n < 100; n++) {
    i2c_start();
    if (i2c_write(EEPROM_DIR << 1)) return 1;
    i2c_stop();
    delayMicroseconds(100);
  }
  return 0;
}

unsigned char ee_leer(unsigned int dir) {
  unsigned char v;
  if (!ee_lista()) return 0xFF;
  i2c_write((unsigned char)(dir >> 8));
  i2c_write((unsigned char)dir);
  i2c_start();
  i2c_write((EEPROM_DIR << 1) | 1);
  v = i2c_read(0);
  i2c_stop();
  return v;
}

void ee_escribir(unsigned int dir, unsigned char v) {
  if (!ee_lista()) return;
  i2c_write((unsigned char)(dir >> 8));
  i2c_write((unsigned char)dir);
  i2c_write(v);
  i2c_stop();
}

void destello(unsigned int ms) {
  digitalWrite(LED, HIGH);
  delay(ms);
  digitalWrite(LED, LOW);
  delay(300);
}

void setup() {
  unsigned char n, i;
  pinMode(LED, OUTPUT);
  i2c_begin();
  n = ee_leer(0);
  if (n == 0xFF) n = 0;                /* EEPROM nueva (o sin respuesta) */
  n++;
  ee_escribir(0, n);
  if (ee_leer(0) != n) {
    destello(2000);                    /* error: no se pudo grabar */
    return;
  }
  for (i = 0; i < n % 10; i++) destello(150);
}

void loop() {
}
