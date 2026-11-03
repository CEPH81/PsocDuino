#!/usr/bin/env python3
"""Regenera memory_tpl.h desde un memory.inc de PSoC Designer: gen_memory_tpl.py memory.inc"""
import sys
src = open(sys.argv[1], encoding='latin1').read().replace('\r\n', '\n').split('\n')
a = [i for i, l in enumerate(src) if l.startswith('SYSTEM_STACK_PAGE:')][0]
b = [i for i, l in enumerate(src) if l.startswith('SYSTEM_MULTIPAGE_STACK:')][0]
tpl = src[:a] + ['@SYSINC@'] + src[b + 1:]
out = ['/* Plantilla de memory.inc de PSoC Designer (macros genericas del modelo de memoria).',
       ' * Generado por tools/psocbuild/gen_memory_tpl.py; @SYSINC@ = equ del proyecto. */',
       'static const char MEMORY_INC_TPL[] =']
out += ['    "%s\\r\\n"' % l.replace('\\', '\\\\').replace('"', '\\"').replace('\t', '\\t') for l in tpl]
out[-1] += ';'
open('memory_tpl.h', 'w').write('\n'.join(out) + '\n')
