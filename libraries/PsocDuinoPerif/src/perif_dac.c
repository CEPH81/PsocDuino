/* perif_dac.c - dacWrite (0..255) con el User Module DAC6 o DAC8, salida en P0_5. */
#include "PsocDuinoPerif.h"
#include "psocduino_perif.h"
#if PSD_HW
#include "perif_hw.h"
#endif

#if PSD_HAS_DAC

static unsigned char dac_on;

void dacWrite(uint8_t value)
{
    if (!dac_on) {
        PSD_DAC_START();
        dac_on = 1;
    }
    PSD_DAC_WRITE(value);
}

#else
/* este chip no tiene el bloque necesario (CY8C21x34: sin DAC ni SAR) */
void dacWrite(uint8_t value) { (void)value; }
#endif
