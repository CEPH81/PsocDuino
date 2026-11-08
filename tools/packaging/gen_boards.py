#!/usr/bin/env python3
"""Genera boards.txt de PsocDuino.

- Una placa por cada chip con variante incluida (como en las versiones 0.3.x).
- Una placa "PSoC 1 (todos los modelos)" con el menu Microcontrolador para
  cambiar de chip sin salir de la placa (solo los de MENU_CHIPS).
  CHIPS guarda los datos de todos los PSoC 1 de psocprog para agregar mas.
"""
import os

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')

# (nombre para psocprog, flash KB, RAM bytes, pines/encapsulado, variante, puertos o None)
CHIPS = [
    # CY8C21x23
    ('CY8C21123', 4, 256, '8 pines', 'cy8c21123', None),
    ('CY8C21223', 4, 256, '16 pines', 'cy8c21223', None),
    ('CY8C21323', 4, 256, '20 pines', 'cy8c21323', None),
    # CY8C21x12
    ('CY8C21312', 8, 512, '20 pines', 'cy8c21312', None),
    ('CY8C21512', 8, 512, '28 pines', 'cy8c21512', None),
    # CY8C21x34 (variantes incluidas)
    ('CY8C21234', 8, 512, '16 pines', 'cy8c21234', None),
    ('CY8C21334', 8, 512, '20 pines', 'cy8c21534', 4),
    ('CY8C21434', 8, 512, '32 pines', 'cy8c21534', 4),
    ('CY8C21534', 8, 512, '28 pines', 'cy8c21534', None),
    ('CY8C21634', 8, 512, '32 pines', 'cy8c21534', 4),
    # CY8C21x45
    ('CY8C21345', 8, 512, '24 pines', 'cy8c21345', None),
    ('CY8C21645-12xxXE', 8, 512, '48 pines, 12 MHz', 'cy8c21645', None),
    ('CY8C21645-24xxXA', 8, 512, '48 pines, 24 MHz', 'cy8c21645', None),
    # CY8C22x13
    ('CY8C22113', 2, 256, '8 pines', 'cy8c22113', None),
    ('CY8C22213', 2, 256, '16 pines', 'cy8c22213', None),
    # CY8C22x45
    ('CY8C22345', 16, 1024, '28 pines', 'cy8c22345', None),
    ('CY8C223451-24xxXA', 16, 1024, '28 pines', 'cy8c22345', None),
    ('CY8C22545-24xxXI', 16, 1024, '44 pines', 'cy8c22545', None),
    ('CY8C22645-12xxXE', 16, 1024, '48 pines, 12 MHz', 'cy8c22645', None),
    ('CY8C22645-24xxXA', 16, 1024, '48 pines, 24 MHz', 'cy8c22645', None),
    # CY8C23x33
    ('CY8C23033', 8, 256, '32 pines', 'cy8c23033', None),
    ('CY8C23433', 8, 256, '32 pines', 'cy8c23433', None),
    ('CY8C23533', 8, 256, '28 pines', 'cy8c23533', None),
    # CY8C24x23 (silicio anterior a la "A")
    ('CY8C24123', 4, 256, '8 pines', 'cy8c24123', None),
    ('CY8C24223', 4, 256, '20 pines', 'cy8c24223', None),
    ('CY8C24423', 4, 256, '28 pines', 'cy8c24423', None),
    # CY8C24x23A (variantes incluidas)
    ('CY8C24123A', 4, 256, '8 pines', 'cy8c24223a', None),
    ('CY8C24223A', 4, 256, '20 pines', 'cy8c24223a', None),
    ('CY8C24423A', 4, 256, '28 pines', 'cy8c24423a', None),
    # CY8C24x94 (USB)
    ('CY8C24794', 16, 1024, '56 pines, USB', 'cy8c24794', None),
    ('CY8C24894', 16, 1024, '56 pines, USB', 'cy8c24894', None),
    ('CY8C24994', 16, 1024, '68 pines, USB', 'cy8c24994', None),
    # CY8C27x43 (variantes incluidas)
    ('CY8C27143', 16, 256, '8 pines', 'cy8c27243', None),
    ('CY8C27243', 16, 256, '20 pines', 'cy8c27243', None),
    ('CY8C27443', 16, 256, '28 pines', 'cy8c27443', None),
    ('CY8C27543', 16, 256, '44 pines', 'cy8c27443', 6),
    ('CY8C27643', 16, 256, '48 pines', 'cy8c27443', 6),
    # CY8C28xxx
    ('CY8C28000', 16, 1024, 'OCD', 'cy8c28000', None),
    ('CY8C28243', 16, 1024, '20 pines', 'cy8c28243', None),
    ('CY8C28403', 16, 1024, '28 pines', 'cy8c28403', None),
    ('CY8C28413', 16, 1024, '28 pines', 'cy8c28413', None),
    ('CY8C28433', 16, 1024, '28 pines', 'cy8c28433', None),
    ('CY8C28445', 16, 1024, '28 pines', 'cy8c28445', None),
    ('CY8C28452', 16, 1024, '28 pines', 'cy8c28452', None),
    ('CY8C28513', 16, 1024, '44 pines', 'cy8c28513', None),
    ('CY8C28533', 16, 1024, '44 pines', 'cy8c28533', None),
    ('CY8C28545', 16, 1024, '44 pines', 'cy8c28545', None),
    ('CY8C28623', 16, 1024, '48 pines', 'cy8c28623', None),
    ('CY8C28643', 16, 1024, '48 pines', 'cy8c28643', None),
    ('CY8C28645', 16, 1024, '48 pines', 'cy8c28645', None),
    # CY8C29x66 (variante incluida)
    ('CY8C29466', 32, 2048, '28 pines', 'cy8c29466', None),
    ('CY8C29566', 32, 2048, '44 pines', 'cy8c29466', None),
    ('CY8C29666', 32, 2048, '48 pines', 'cy8c29466', None),
    ('CY8C29866', 32, 2048, '100 pines', 'cy8c29466', None),
]

# Placas individuales (las de siempre): chip con variante propia incluida
# Menu Microcontrolador: solo los chips con variante incluida, en este orden
MENU_CHIPS = ['CY8C24423A', 'CY8C24223A', 'CY8C27443', 'CY8C27243', 'CY8C29466', 'CY8C21534', 'CY8C21234']
BOARDS = ['CY8C21234', 'CY8C21534', 'CY8C24223A', 'CY8C24423A', 'CY8C27243', 'CY8C27443', 'CY8C29466']

# Placas con perifericos (User Modules de los proyectos Base2 de Carlos):
# Serial, analogRead, analogWrite y dacWrite con la biblioteca PsocDuinoPerif
PERIF_BOARDS = [('CY8C24423A', 'cy8c24423a_perif', 'UART P2_1/P2_0, PWM P1_6, ADC, DAC6'),
                ('CY8C24223A', 'cy8c24223a_perif', 'UART P1_4/P1_2, PWM P1_6, ADC, DAC8')]

MENUS = [
    ('alim', [('v5', '5 V', {'build.psoc_vcc': '5', 'upload.vdd': '5'}),
              ('v33', '3.3 V', {'build.psoc_vcc': '3', 'upload.vdd': '3.3'})]),
    ('cpu', [('c24', '24 MHz (SysClk/1)', {'build.psoc_cpu': '3', 'build.f_cpu': '24000000'}),
             ('c12', '12 MHz (SysClk/2)', {'build.psoc_cpu': '2', 'build.f_cpu': '12000000'}),
             ('c6', '6 MHz (SysClk/4)', {'build.psoc_cpu': '1', 'build.f_cpu': '6000000'}),
             ('c3', '3 MHz (SysClk/8)', {'build.psoc_cpu': '0', 'build.f_cpu': '3000000'}),
             ('c1', '1.5 MHz (SysClk/16)', {'build.psoc_cpu': '4', 'build.f_cpu': '1500000'})]),
    ('entry', [('power', 'Encendido / POR (D0 controla Vdd)', {'upload.mode': 'power'}),
               ('xres', 'XRES (pin de reset)', {'upload.mode': 'xres'})]),
    ('security', [('none', 'Sin proteccion (permite releer)', {'upload.secure_flag': '--no-secure'}),
                  ('hex', 'Segun el .hex', {'upload.secure_flag': ''}),
                  ('all', 'Proteger todo (W)', {'upload.secure_flag': '--protect'})]),
    ('verify', [('full', 'Completa (byte a byte)', {'upload.psoc_verify': 'full'}),
                ('checksum', 'Rapida (checksum)', {'upload.psoc_verify': 'checksum'})]),
]

INCLUDED = sorted(set(c[4] for c in CHIPS if os.path.isdir(os.path.join(ROOT, 'variants', c[4]))))


def short(name):
    return name.split('-')[0]


def ram_txt(r):
    return '%dK' % (r // 1024) if r >= 1024 else '%d B' % r


def chip_props(c):
    name, fl, ram, pins, var, ports = c
    p = [('build.mcu', short(name)), ('build.board', 'PSOC_' + short(name)), ('build.variant', var),
         ('build.psoc_ports', '-DPSOC_NUM_PORTS=%d' % ports if ports else ''),
         ('upload.chip', name), ('upload.maximum_size', fl * 1024), ('upload.maximum_data_size', ram)]
    return p


def common(L, b):
    for k, v in [('build.core', 'psoc1'), ('build.psoc_ports', ''), ('upload.tool', 'psocprog'),
                 ('upload.tool.default', 'psocprog'), ('upload.protocol', 'issp'),
                 ('upload.port_required', 'false'), ('bootloader.tool', 'psocprog'),
                 ('bootloader.tool.default', 'psocprog')]:
        L.append('%s.%s=%s' % (b, k, v))


def menus(L, b):
    for m, opts in MENUS:
        for oid, label, props in opts:
            L.append('%s.menu.%s.%s=%s' % (b, m, oid, label))
            for k, v in props.items():
                L.append('%s.menu.%s.%s.%s=%s' % (b, m, oid, k, v))


def main():
    L = ['# PsocDuino - placas Cypress PSoC 1 (nucleo M8C)',
         '# GENERADO por tools/packaging/gen_boards.py (edite ese archivo, no este).',
         '# Compilacion: ImageCraft (PSoC Designer 5.x) via psocbuild. Programacion: CH341A + psocprog.',
         '# Alimentacion y reloj de CPU se aplican en cada compilacion (GlobalParams.inc).',
         '',
         'menu.chip=Microcontrolador',
         'menu.alim=Alimentacion del chip',
         'menu.cpu=Reloj de CPU',
         'menu.entry=Entrada a programacion',
         'menu.security=Seguridad de flash',
         'menu.verify=Verificacion',
         '']
    bychip = {c[0]: c for c in CHIPS}
    for n in BOARDS:
        c = bychip[n]
        b = c[4] if c[4] == n.lower() else n.lower()
        L.append('#' + '-' * 60)
        L.append('%s.name=PSoC 1 %s (%dK flash, %s RAM, %s)' % (b, n, c[1], ram_txt(c[2]), c[3]))
        common(L, b)
        for k, v in chip_props(c):
            L.append('%s.%s=%s' % (b, k, v))
        menus(L, b)
        L.append('')

    for n, var, txt in PERIF_BOARDS:
        c = list(bychip[n])
        c[4] = var
        b = var
        L.append('#' + '-' * 60)
        L.append('%s.name=PSoC 1 %s + perifericos (%s)' % (b, n, txt))
        common(L, b)
        for k, v in chip_props(tuple(c)):
            L.append('%s.%s=%s' % (b, k, v))
        menus(L, b)
        L.append('')

    b = 'psoc1'
    L.append('#' + '-' * 60)
    L.append('# Todos los PSoC 1: el chip se elige en Herramientas -> Microcontrolador')
    L.append('%s.name=PSoC 1 (todos los modelos)' % b)
    common(L, b)
    L.append('%s.build.board=PSOC1' % b)
    # solo los chips con variante propia (proyectos de PSoC Designer de Carlos)
    for n in MENU_CHIPS:
        c = bychip[n]
        oid = c[0].lower().replace('-', '_')
        L.append('%s.menu.chip.%s=%s (%dK flash, %s RAM, %s)' % (b, oid, c[0], c[1], ram_txt(c[2]), c[3]))
        for k, v in chip_props(c):
            L.append('%s.menu.chip.%s.%s=%s' % (b, oid, k, v))
    menus(L, b)
    L.append('')
    open(os.path.join(ROOT, 'boards.txt'), 'w').write('\n'.join(L))
    print('boards.txt: %d placas + %d microcontroladores en el menu' % (len(BOARDS) + 1, len(MENU_CHIPS)))


if __name__ == '__main__':
    main()
