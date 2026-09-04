# Security Policy

## Supported versions

Security fixes are applied to the latest Heltec V4 release and the current `main` branch. Older releases are not maintained unless explicitly stated in their release notes.

## Reporting a vulnerability

Do not report security vulnerabilities through a public issue. Use GitHub's private vulnerability reporting for this repository.

Include the affected firmware target, Heltec V4 hardware revision, impact, triggering conditions, and a minimal reproduction when possible. Do not include private keys, production credentials, precise node locations, or other sensitive deployment data in public material.

## Scope

In scope for this fork:

- Vulnerabilities reproducible on a retained Heltec V4 firmware target.
- Memory corruption, denial of service, authentication bypass, or encryption bypass triggered remotely.
- Heltec V4-specific storage or power behavior that can predictably corrupt security-sensitive state.
- Unsafe handling of Bluetooth, USB, Wi-Fi, ESP-NOW, GPS, display, or radio input in the retained targets.

General MeshCore protocol vulnerabilities should also be reported to the upstream `meshcore-dev/MeshCore` project so the wider ecosystem can receive the fix.

Out of scope:

- Physical-access attacks such as direct flash extraction or invasive hardware probing.
- Radio jamming and ordinary physical-layer interference.
- Regulatory compliance questions.
- Vulnerabilities that exist solely in an unmodified third-party dependency; report those to the dependency's maintainer.
