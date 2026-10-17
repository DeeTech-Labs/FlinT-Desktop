# Security Policy

## Supported versions

FlinT Desktop is in **alpha**. Security fixes target the latest `main` branch.

## Reporting a vulnerability

Do not open a public issue for security-sensitive reports.

1. Use **Security → Report a vulnerability** in this repository.
2. If unavailable, contact maintainers through the
   [DeeTech Labs](https://github.com/DeeTech-Labs) organization on GitHub.

Include application commit/version, OS and Qt versions, device/firmware versions,
impact and reproduction steps. A minimal malformed protocol reply or capture
file is useful for input-handling bugs; remove private data before sharing it.

## Scope

The application treats USB replies as untrusted data and exports files only
when requested. Report parsing, device-connection, export or dependency bugs
with a reproducible software impact. Device firmware has its own
[security policy](https://github.com/DeeTech-Labs/FlinT-OS/blob/main/SECURITY.md).
