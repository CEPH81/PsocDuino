;-----------------------------------------------------------------------------
; psocduino_asm.S - rutinas con ciclos exactos para el nucleo PsocDuino (M8C)
; Ensamblador ImageCraft (iasm8c). Convencion fastcall16: parametro BYTE en A,
; resultado WORD en X (MSB) : A (LSB). No usan RAM, validas en SMM y LMM.
; Ciclos M8C: dec A 4, jnz 5, inc A 4, jnc 5, inc X 4, jmp 5,
;             tst reg[expr],expr 9.
;-----------------------------------------------------------------------------
export  _psocduino_spin9
export   psocduino_spin9
export  _psocduino_measure_tick
export   psocduino_measure_tick

AREA text (ROM, REL)

;-----------------------------------------------------------------------------
; void psocduino_spin9(BYTE n)   espera 9*n ciclos (n = 0: vuelve enseguida)
;-----------------------------------------------------------------------------
 psocduino_spin9:
_psocduino_spin9:
    cmp   A, 0
    jz    psd_spin_end
psd_spin_loop:
    dec   A
    jnz   psd_spin_loop
psd_spin_end:
    ret

;-----------------------------------------------------------------------------
; WORD psocduino_measure_tick(void)
; Cuenta iteraciones de 23 ciclos (+9 cada 256) durante un tic del Sleep
; Timer (bit 6 de INT_CLR0, 0DAh). El latch debe estar habilitado en INT_MSK0
; y las interrupciones globales deshabilitadas.
;-----------------------------------------------------------------------------
 psocduino_measure_tick:
_psocduino_measure_tick:
    and   reg[0DAh], BFh           ; borrar pendiente (escribir 0 borra)
psd_sync:
    tst   reg[0DAh], 40h           ; esperar el primer tic para sincronizar
    jz    psd_sync
    and   reg[0DAh], BFh
    mov   X, 0
    mov   A, 0
psd_cal_loop:
    tst   reg[0DAh], 40h
    jnz   psd_cal_done
    inc   A
    jnc   psd_cal_loop
    inc   X
    jmp   psd_cal_loop
psd_cal_done:
    and   reg[0DAh], BFh
    ret
