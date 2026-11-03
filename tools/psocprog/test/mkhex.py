#!/usr/bin/env python3
"""mkhex.py OUT.hex SIZE [SECBYTE] [--bad-checksum]: PSoC Designer style hex"""
import sys, random
out, size = sys.argv[1], int(sys.argv[2], 0)
sec = int(sys.argv[3], 0) if len(sys.argv) > 3 and not sys.argv[3].startswith('--') else None
random.seed(size)
data = bytes(random.randrange(256) for _ in range(size))
def rec(addr, typ, payload):
    b = bytes([len(payload), addr >> 8, addr & 0xFF, typ]) + payload
    return ':' + b.hex().upper() + '%02X' % ((-sum(b)) & 0xFF)
lines = [rec(a, 0, data[a:a+64]) for a in range(0, size, 64)]
if sec is not None:
    lines.append(rec(0, 4, b'\x00\x10'))
    lines.append(rec(0, 0, bytes([sec]) * 64))
cs = sum(data) & 0xFFFF
if '--bad-checksum' in sys.argv: cs ^= 0x1234
lines.append(rec(0, 4, b'\x00\x20'))
lines.append(rec(0, 0, bytes([cs >> 8, cs & 0xFF])))
lines.append(':00000001FF')
open(out, 'w').write('\n'.join(lines) + '\n')
open(out + '.bin', 'wb').write(data)
