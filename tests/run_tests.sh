#!/usr/bin/env bash
# End-to-end tests for photoscrypto v1.1. Usage: tests/run_tests.sh ./photoscrypto-cli
set -euo pipefail

BIN="$(realpath "${1:-./photoscrypto-cli}")"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

PASS="correct horse battery"
pass=0
fail=0

ok()   { echo "  [PASS] $1"; pass=$((pass + 1)); }
bad()  { echo "  [FAIL] $1"; fail=$((fail + 1)); }
expect_fail() { if "$@" >/dev/null 2>&1; then return 1; else return 0; fi; }

# Create a compressible test PNG file
printf '\x89PNG\r\n\x1a\n' > photo.png
# Highly repetitive pattern (typical of images with flat areas or UI)
python3 -c 'import sys; sys.stdout.buffer.write(b"ABCDEFGH12345678" * 20000)' >> photo.png

echo "Running photoscrypto v1.1 tests..."

# 1. Test encoding with password
echo "$PASS" | "$BIN" encode photo.png enc_pass.txt --password-stdin >/dev/null
[[ -f enc_pass.txt ]] && ok "encode with password creates output" || bad "encode with password creates output"
[[ "$(stat -c %a enc_pass.txt)" == "600" ]] && ok "output has mode 600" || bad "output has mode 600"

# 2. Test compression efficiency:
# Original photo is ~320 KB. Encrypted and compressed output should be WAY smaller than 320 KB!
ORIG_SIZE=$(stat -c %s photo.png)
ENC_SIZE=$(stat -c %s enc_pass.txt)
if [[ $ENC_SIZE -lt $((ORIG_SIZE / 2)) ]]; then
    ok "zlib compression active (file size shrank from ${ORIG_SIZE}B to ${ENC_SIZE}B)"
else
    bad "compression did not shrink compressible file"
fi

# 3. Test round-trip with password
echo "$PASS" | "$BIN" decode enc_pass.txt restored_pass --password-stdin >/dev/null
cmp -s photo.png restored_pass.png && ok "round-trip with password is byte-identical (.png detected)" || bad "round-trip with password"

# 4. Test wrong password rejection
if expect_fail sh -c "echo 'wrong password!' | '$BIN' decode enc_pass.txt x --password-stdin"; then
    ok "wrong password rejected"; else bad "wrong password rejected"; fi
[[ ! -e x.bin && ! -e x && ! -e x.png ]] && ok "no output on wrong password" || bad "no output on wrong password"

# 5. Test optional password feature (--no-password)
"$BIN" encode photo.png enc_nopass.txt --no-password >/dev/null
[[ -f enc_nopass.txt ]] && ok "encode with --no-password succeeds" || bad "encode with --no-password succeeds"

# Decoding unprotected file should not even ask for password!
"$BIN" decode enc_nopass.txt restored_nopass >/dev/null
cmp -s photo.png restored_nopass.png && ok "round-trip without password is byte-identical" || bad "round-trip without password"

# 6. Test tampering rejection (tampered ciphertext must fail authenticated tag verification)
python3 - <<'EOF'
data = bytearray(open("enc_pass.txt", "rb").read())
i = len(data) // 2
data[i] = ord("Z") if data[i] != ord("Z") else ord("A")
open("tampered.txt", "wb").write(data)
EOF
if expect_fail sh -c "echo '$PASS' | '$BIN' decode tampered.txt t --password-stdin"; then
    ok "tampered file rejected by AES-GCM tag verification"; else bad "tampered file rejected"; fi

# 7. Test overwrite refusal without --force
if expect_fail sh -c "echo '$PASS' | '$BIN' encode photo.png enc_pass.txt --password-stdin"; then
    ok "refuses to overwrite without --force"; else bad "refuses to overwrite without --force"; fi
echo "$PASS" | "$BIN" encode photo.png enc_pass.txt --password-stdin --force >/dev/null \
    && ok "--force overwrites" || bad "--force overwrites"

# 8. Test symlink protection
ln -s photo.png link.txt
if expect_fail sh -c "echo '$PASS' | '$BIN' encode photo.png link.txt --password-stdin --force"; then
    ok "does not write through symlinks"; else bad "does not write through symlinks"; fi

# 9. Test unknown options
if expect_fail "$BIN" encode photo.png --bogus; then ok "unknown option rejected"; else bad "unknown option rejected"; fi

echo
echo "Result: $pass passed, $fail failed"
[[ $fail -eq 0 ]]
