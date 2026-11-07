/* I2C_soft - I2C maestro por software (~50 kHz) con patas de drenador abierto.
 * Mismas patas que el I2C por hardware de PSoC Designer: SDA P1_5, SCL P1_7.
 * Necesita resistencias de 4,7 kOhm de SDA y SCL a Vdd.
 * Pocas funciones y sin buferes: ocupa poca flash y nada de RAM. */
#define I2C_SDA P1_5
#define I2C_SCL P1_7

static void i2c_scl_alto(void) {
  unsigned char n = 200;               /* el esclavo puede estirar el reloj */
  digitalWrite(I2C_SCL, HIGH);
  while (digitalRead(I2C_SCL) == LOW && --n) ;
}

void i2c_begin(void) {
  digitalWrite(I2C_SDA, HIGH);         /* HIGH = suelta la linea (alta impedancia) */
  digitalWrite(I2C_SCL, HIGH);
  pinMode(I2C_SDA, OUTPUT_OPEN_DRAIN);
  pinMode(I2C_SCL, OUTPUT_OPEN_DRAIN);
}

void i2c_start(void) {
  digitalWrite(I2C_SDA, HIGH);
  i2c_scl_alto();
  digitalWrite(I2C_SDA, LOW);
  digitalWrite(I2C_SCL, LOW);
}

void i2c_stop(void) {
  digitalWrite(I2C_SDA, LOW);
  i2c_scl_alto();
  digitalWrite(I2C_SDA, HIGH);
}

/* envia un byte; devuelve 1 si el esclavo respondio ACK */
unsigned char i2c_write(unsigned char b) {
  unsigned char i, ack;
  for (i = 0; i < 8; i++) {
    digitalWrite(I2C_SDA, (b & 0x80) ? HIGH : LOW);
    b <<= 1;
    i2c_scl_alto();
    digitalWrite(I2C_SCL, LOW);
  }
  digitalWrite(I2C_SDA, HIGH);
  i2c_scl_alto();
  ack = digitalRead(I2C_SDA) == LOW;
  digitalWrite(I2C_SCL, LOW);
  return ack;
}

/* lee un byte; ack = 1 para pedir otro, 0 en el ultimo */
unsigned char i2c_read(unsigned char ack) {
  unsigned char i, b = 0;
  digitalWrite(I2C_SDA, HIGH);
  for (i = 0; i < 8; i++) {
    i2c_scl_alto();
    b = (unsigned char)((b << 1) | (digitalRead(I2C_SDA) == HIGH));
    digitalWrite(I2C_SCL, LOW);
  }
  digitalWrite(I2C_SDA, ack ? LOW : HIGH);
  i2c_scl_alto();
  digitalWrite(I2C_SCL, LOW);
  digitalWrite(I2C_SDA, HIGH);
  return b;
}
