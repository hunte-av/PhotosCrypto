# Security Policy

## Reporting a vulnerability

Please **do not open a public issue** for security problems.
Use GitHub's private reporting instead: *Security → Report a vulnerability* on this repository.

Include the photoscrypto version (`photoscrypto --version`), your distribution and steps to reproduce.
You should get a response within 7 days.

## Design summary

| Concern | Mitigation |
|---|---|
| Brute-force attacks on the password | scrypt (N=2¹⁷, r=8, p=1), random 16-byte salt per file |
| Confidentiality and integrity | AES-256-GCM with a random 12-byte IV per file and a 16-byte tag |
| Password leaking via `ps` or shell history | Read from `/dev/tty` with echo off; no `--password=` argument |
| Secrets left in memory | Key, password and decrypted photo are wiped with `OPENSSL_cleanse` |
| Overwriting and symlink attacks | `O_EXCL` / `O_NOFOLLOW`, mode `0600` |
| Malformed or oversized input | Strict symbol-alphabet parsing, file size limits, regular-file check |
| Memory-safety bugs | Hardened build (FORTIFY_SOURCE=3, PIE, full RELRO, stack protector); CI runs the tests under ASan and UBSan |

## Out of scope

- Weak or reused passwords.
- A compromised machine (keyloggers, root access, swap or core dumps).
- Hiding the existence of the file (this is not steganography).

## Limitations of self-destruct

The optional self-destruct counter (1-10 wrong passwords, chosen when encoding) is stored inside the
encoded file. Anyone who copies the file can try passwords on every copy offline, so self-destruct only
protects against casual access, not against an attacker working on copies. The real protection is a
strong password: the key is derived with scrypt (N=2^18, r=8, p=1) and the data is sealed with
AES-256-GCM. The app shows a strength indicator while you choose a password; the minimum length
stays 6, but 12+ characters with mixed character types are recommended.
