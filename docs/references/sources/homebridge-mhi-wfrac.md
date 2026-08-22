# Homebridge MHI WF-RAC

- Repository：`JobDoesburg/homebridge-mhi-wfrac`
- Revision：`3d649dd6c6adbf50d405bb60961ffe820eb6241b`
- License：Apache-2.0
- Priority：PRIMARY
- Authority boundary：independent current local API、transport、registration、queue corroboration；不是 universal protocol authority。
- Important files / evidence areas：local HTTP/HTTPS transport、registration、retry、request pacing、current integration behavior。
- Topics：api-envelope、identity-registration、tls-transport、reliability；observed HTTPS-first、TLS 1.2、ALPN HTTP/1.1、Connection close、keepAlive false、rejectUnauthorized false、header-casing compatibility。
- Known limitations / conflicts：firmware branches vary；observed implementation behavior is not every device requirement。
- Do-not-assume：not local hardware evidence；不把 exact timeout/retry 或 TLS policy universalize。
