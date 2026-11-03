/* main.c - arranque Arduino para PSoC 1.
 * boot.asm (PSoC Designer) carga la configuracion global y llama a _main. */
#include "Arduino.h"

void main(void)
{
    psocduino_init();
    setup();
    for (;;) loop();
}
