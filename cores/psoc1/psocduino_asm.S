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

;-----------------------------------------------------------------------------
; Acceso indexado a registros (reemplaza los switch por puerto: mucho menos flash)
;   BYTE psocduino_reg_rd(BYTE addr)      banco 0, direccion en A
;   BYTE psocduino_reg1_rd(BYTE addr)     banco 1
;   void psocduino_reg_wr(WORD av)        X = direccion (MSB), A = valor (LSB)
;   void psocduino_reg1_wr(WORD av)       banco 1
;-----------------------------------------------------------------------------
export  _psocduino_reg_rd
export   psocduino_reg_rd
export  _psocduino_reg1_rd
export   psocduino_reg1_rd
export  _psocduino_reg_wr
export   psocduino_reg_wr
export  _psocduino_reg1_wr
export   psocduino_reg1_wr

 psocduino_reg_rd:
_psocduino_reg_rd:
    mov   X, A
    mov   A, reg[X+0]
    ret

 psocduino_reg1_rd:
_psocduino_reg1_rd:
    mov   X, A
    or    F, 10h
    mov   A, reg[X+0]
    and   F, EFh
    ret

 psocduino_reg_wr:
_psocduino_reg_wr:
    mov   reg[X+0], A
    ret

 psocduino_reg1_wr:
_psocduino_reg1_wr:
    or    F, 10h
    mov   reg[X+0], A
    and   F, EFh
    ret

;-----------------------------------------------------------------------------
; BYTE psocduino_mask(BYTE pin)        1 << (pin & 7) con una tabla (sin bucle)
;-----------------------------------------------------------------------------
export  _psocduino_mask
export   psocduino_mask

 psocduino_mask:
_psocduino_mask:
    and   A, 07h
    index psd_bits
    ret
psd_bits:
    db    01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
