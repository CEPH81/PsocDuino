/* LCD_HD44780 - pantalla de caracteres 16x2 en modo de 4 bits, con las mismas
 * conexiones que el User Module LCD de PSoC Designer (todo en el puerto 2):
 *   P2_0..P2_3 = D4..D7, P2_4 = E, P2_5 = RS, P2_6 = R/W (siempre en bajo).
 * Muestra un texto y los segundos desde el arranque. Sin buferes en RAM.
 * Patas para los chips de 28 pines (CY8C24423A, CY8C27443, CY8C29466); en los de
 * 16/20 pines (CY8C21234, CY8C24223A, CY8C27243) elija otras que existan en su encapsulado.
 */
#define LCD_E  P2_4
#define LCD_RS P2_5
#define LCD_RW P2_6

void lcd_nibble(unsigned char n) {
  digitalWrite(P2_0, n & 1);
  digitalWrite(P2_1, (n >> 1) & 1);
  digitalWrite(P2_2, (n >> 2) & 1);
  digitalWrite(P2_3, (n >> 3) & 1);
  digitalWrite(LCD_E, HIGH);
  delayMicroseconds(2);
  digitalWrite(LCD_E, LOW);
}

void lcd_byte(unsigned char b, unsigned char rs) {
  digitalWrite(LCD_RS, rs);
  lcd_nibble((unsigned char)(b >> 4));
  lcd_nibble(b);
  delayMicroseconds(50);               /* la mayoria de ordenes tarda 37 us */
}

void lcd_cmd(unsigned char c) {
  lcd_byte(c, LOW);
  if (c < 4) delay(2);                 /* borrar / inicio: 1,5 ms */
}

void lcd_begin(void) {
  unsigned char p;
  for (p = P2_0; p <= P2_6; p++) {
    digitalWrite(p, LOW);
    pinMode(p, OUTPUT);
  }
  delay(50);                           /* encendido del controlador */
  lcd_nibble(3); delay(5);             /* secuencia de inicio a 4 bits */
  lcd_nibble(3); delayMicroseconds(150);
  lcd_nibble(3); delayMicroseconds(150);
  lcd_nibble(2); delayMicroseconds(150);
  lcd_cmd(0x28);                       /* 4 bits, 2 lineas, 5x8 */
  lcd_cmd(0x0C);                       /* pantalla encendida, sin cursor */
  lcd_cmd(0x06);                       /* avanzar a la derecha */
  lcd_cmd(0x01);                       /* borrar */
}

void lcd_pos(unsigned char col, unsigned char fila) {
  lcd_cmd((unsigned char)(0x80 | (fila ? 0x40 : 0x00) | col));
}

/* ImageCraft: "const char *" apunta a la FLASH (los textos entre comillas);
 * no pasar aqui un arreglo de RAM. */
void lcd_print(const char *s) {
  while (*s) lcd_byte((unsigned char)*s++, HIGH);
}

void lcd_num(unsigned int v) {         /* sin printf: ahorra flash */
  char d[5];                           /* digitos en RAM: se envian uno a uno */
  unsigned char i = 0;
  do {
    d[i++] = (char)('0' + v % 10);
    v /= 10;
  } while (v && i < 5);
  while (i) lcd_byte((unsigned char)d[--i], HIGH);
}

void setup() {
  lcd_begin();
  lcd_pos(0, 0);
  lcd_print("PsocDuino");
}

void loop() {
  static unsigned long t = 0;          /* sin dividir millis() entre 1000: */
  static unsigned int seg = 0;         /* evita la division de 32 bits */
  if (millis() - t >= 1000) {
    t += 1000;
    seg++;
    lcd_pos(0, 1);
    lcd_num(seg);
    lcd_print(" s ");
  }
}
