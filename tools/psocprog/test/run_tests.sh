#!/bin/sh
# psocprog test-suite: runs against the built-in CH341A+PSoC simulator, so
# the whole USB packet encoding and the ISSP algorithm are exercised.
P=${1:-./psocprog}
HERE=$(cd "$(dirname "$0")" && pwd)
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
pass=0; fail=0
ok()   { pass=$((pass+1)); echo "  ok   $1"; }
bad()  { fail=$((fail+1)); echo "  FAIL $1"; }
run()  { PSOCPROG_SIM_STATE="$T/state" "$P" "$@"; }
fresh(){ rm -f "$T/state"; }

echo "1. deteccion de todos los chips"
n=0; nok=0
for c in $("$P" --list | awk 'NR>1{print $1}'); do
  n=$((n+1))
  out=$("$P" -b "sim:$c" 2>&1)
  echo "$out" | grep -q -- "-> $c\$" && nok=$((nok+1)) || echo "     $c: $out"
done
[ "$n" = "$nok" ] && ok "$nok/$n chips detectados por su ID" || bad "deteccion $nok/$n"
dups=$("$P" --list | awk 'NR>1{print $2}' | sort | uniq -d)
[ -z "$dups" ] && ok "sin IDs duplicados" || bad "IDs duplicados: $dups"

echo "2. programar + verificar + checksum + releer"
for spec in CY8C24423A:4096 CY8C27443:16384 CY8C29466:32768 CY8C21312:8192 CY8C22345:16384 CY8C24794:9000; do
  c=${spec%:*}; sz=${spec#*:}
  fresh
  python3 "$HERE/mkhex.py" "$T/p.hex" "$sz"
  if run -b "sim:$c" -c "$c" -w "$T/p.hex" >"$T/log" 2>&1 && grep -q "Checksum OK" "$T/log"; then
    run -b "sim:$c" -r "$T/r.hex" >/dev/null 2>&1
    python3 - "$T/r.hex" "$T/p.hex.bin" <<'EOF' && ok "$c ($sz bytes) escrito, verificado y releido" || bad "$c lectura distinta"
import sys
img = bytearray(65536); n = 0
for l in open(sys.argv[1]):
    if l[7:9] == '00':
        c = int(l[1:3], 16); a = int(l[3:7], 16)
        img[a:a+c] = bytes.fromhex(l[9:9+2*c]); n = max(n, a+c)
ref = open(sys.argv[2], 'rb').read()
sys.exit(0 if img[:len(ref)] == ref and not any(img[len(ref):n]) else 1)
EOF
  else bad "$c: $(tail -1 "$T/log")"; fi
done

echo "3. bits de seguridad"
fresh; python3 "$HERE/mkhex.py" "$T/s.hex" 2048 0xFF
run -b sim:CY8C24423A -w "$T/s.hex" >"$T/log" 2>&1 && grep -q "Checksum OK" "$T/log" \
  && ok "seguridad escrita despues de verificar; checksum OK" || bad "secure: $(tail -1 "$T/log")"
run -b sim:CY8C24423A -r "$T/r.hex" 2>&1 | grep -q "64 bloques protegidos" \
  && ok "lectura detecta los 64 bloques protegidos" || bad "proteccion no detectada"
fresh; run -b sim:CY8C24423A -w "$T/s.hex" --no-secure >/dev/null 2>&1
run -b sim:CY8C24423A -r "$T/r.hex" 2>&1 | grep -q protegidos \
  && bad "--no-secure protegio igual" || ok "--no-secure deja el chip legible"
run -b sim:CY8C24423A -e >/dev/null 2>&1; run -b sim:CY8C24423A -r "$T/r.hex" >/dev/null 2>&1
grep -v ":00000001FF" "$T/r.hex" | cut -c10-41 | grep -vq '^0*$' && bad "borrado incompleto" || ok "borrado deja la flash en 00"

fresh; python3 "$HERE/mkhex.py" "$T/u.hex" 1024
run -b sim:CY8C24423A -w "$T/u.hex" --protect >/dev/null 2>&1; run -b sim:CY8C24423A -r "$T/r.hex" 2>&1 | grep -q "64 bloques protegidos" \
  && ok "--protect protege toda la flash" || bad "--protect"

echo "4. errores y protecciones"
python3 "$HERE/mkhex.py" "$T/big.hex" 5000
run -b sim:CY8C24423A -w "$T/big.hex" 2>&1 | grep -q "solo tiene 4096" && ok "programa demasiado grande rechazado" || bad "tamano no comprobado"
run -b sim:CY8C27443 -c CY8C29466 -w "$T/s.hex" >/dev/null 2>&1 && bad "chip distinto aceptado" || ok "chip distinto al elegido rechazado"
fresh; run -b sim:CY8C27443 -c CY8C29466 --force -w "$T/p.hex" >/dev/null 2>&1 && ok "--force permite forzar el chip" || bad "--force"
python3 "$HERE/mkhex.py" "$T/bc.hex" 1024 --bad-checksum
fresh; run -b sim:CY8C27443 -w "$T/bc.hex" 2>&1 | grep -q "no coincide con los datos" && ok "checksum del .hex incorrecto avisado" || bad "checksum .hex"
printf ':10000000FFFF\n' > "$T/broken.hex"
run -b sim:CY8C27443 -w "$T/broken.hex" 2>&1 | grep -q "error en" && ok ".hex corrupto rechazado" || bad ".hex corrupto"
"$P" -b sim:CY8C27443 -c CY8C99999 2>&1 | grep -q "chip desconocido" && ok "nombre de chip invalido rechazado" || bad "nombre invalido"

echo "5. modos de entrada y tension"
fresh; run -b sim:CY8C29466 -m power -V 3.3 --power-off-ms 1 -w "$T/s.hex" --no-secure >/dev/null 2>&1 \
  && ok "modo power + 3.3 V" || bad "modo power"
"$P" -b "sim:CY8C21234(W)" -c CY8C21234 2>&1 | grep -q "Chip: CY8C21234(W)" && ok "nombre corto CY8C21234 -> CY8C21234(W)" || bad "alias"
fresh; run -b sim:CY8C27443 -k 2>&1 | grep -q "Checksum del chip: 0x" && ok "checksum independiente (-k)" || bad "-k"

echo
echo "Resultado: $pass ok, $fail fallos"
[ "$fail" -eq 0 ]
