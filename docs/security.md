# Security

Potentially sensitive data includes Wi-Fi credentials, Operator ID/controller identity, Matter fabric data, commissioning secrets, OTA signing keys and future WebUI authentication secrets.

Never commit or expose real credentials in Git, README, documentation examples, logs, serial output or crash reports. Current Phase 0 does not implement encrypted NVS, Secure Boot or Flash Encryption; these are future hardening roadmap items. Local-first control must not turn cloud credentials into a daily runtime dependency.
