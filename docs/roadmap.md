# 開發路線圖

## Phase 0 — 研究／架構

- 0.0 — 專案初始化：**完成**
- 0.1 Reference Deep Review — **Complete**
- Software-only machine-readable foundation — **Complete**（contract、validator、64 個 targeted host tests 與 GitHub Actions software validation gate）
- 0.2 — WF-RAC Protocol / State Map：研究基礎已建立；實機相關工作暫緩
- 0.3 — Matter Room Air Conditioner Mapping：暫緩
- 0.4 — Local API Hardware Validation Plan：待未來恢復實作時進行

## 後續階段

1. WF-RAC Local API PoC
2. WF-RAC Device Model
3. Matter over Wi-Fi
4. Provisioning / WebUI
5. Optional MQTT Adapter
6. Hardening / OTA / Security

## Phase 1A — Pure Protocol Core

- CRC16-CCITT core：**完成**
- AirconStat structural framing：**完成**
- Basic fixed-state decoder、Power/Mode/Target Temperature/Fan command intents、capability profile decoder、operation-data raw segment parser/classifier、Aircon block serializer、safe operation-data query builder、bounded project query batch 1–3、standard Base64 codec 與 generic full AirconStat composition：**完成**
- Vane、diagnostics、request RECEIVE policy、query transaction runtime、diagnostic physical conversions、advanced fixed-state fields、Device Model、JSON、transport/runtime 與 hardware validation：尚未實作

目前狀態：**Phase 1A protocol core 已完成；Reference Deep Review 已完成；實機驗證仍暫緩。**

## Phase 1B — WF-RAC Device Model

- Basic HVAC state projection 與 tri-state app capability hints：**完成**
- Explicit observed state ownership、desired writable-state projection、single pending command skeleton 與 explicit availability state：**完成**
- Explicit observation reconciliation、pending target convergence、field-level desired convergence 與 superseding desired race protection：**完成**
- Device Model command intent → Protocol safe command mapping，含 lossless 0.5°C target-temperature boundary：**完成**
- Observed-baseline control request composition、official no-request sentinel、RECEIVE compatibility projection 與 command byte8 carry：**完成**
- Full safe operation-data query request composition（1–3 bounded query、RECEIVE compatibility baseline 與 no-request sentinel）：**完成**
- Advanced protocol fields、diagnostics projection、Base64/API envelope、TimeProvider/timestamp、Operator ID、automatic command dispatch、queue、transport ACK semantics、retry、timeout clock、polling、availability policy、transport/runtime、Matter mapping 與 hardware validation：尚未實作

## Phase 1C — WF-RAC Transport Abstraction

- Portable transport contract、endpoint/request/response DTO、transport error 與 HTTP status 分離、FakeTransport 與獨立 local host test gate：**完成**
- Portable TimeProvider（trusted／estimated／unavailable）、64-bit Unix timestamp、五個 typed local API request builders、JSON escaping、AirconStat CRC validation／Base64 wrapping、`accountId=operatorId` 與 compatibility `remote=0` serialization：**完成**
- HTTPS-first portable scheme policy、caller-owned per-device in-memory cache、sticky HTTPS／HTTP、explicit `scheme_mismatch`-only HTTP compatibility fallback decision，以及 TLS identity failure taxonomy：**完成**
- ESP-IDF v6.0.2 local toolchain baseline、platform-specific HTTP/TLS error classifier，以及 ESP32-C6／ESP32-C3 repository compile harness／local firmware compile gate：**完成**
- Portable WF-RAC C++ library stack 維持 monorepo boundary；ESP-IDF reference implementation 與未來 Matter bridge firmware 可各自作為 consumer，packaging／versioning／external distribution：尚未開始
- Platform clock source、Operator ID persistence、registration lifecycle／response parsing、persistent scheme cache／NVS、certificate policy、actual HTTP/TLS transport、mDNS、retry/rate limiting、rediscovery trigger、hardware validation 與 Matter：尚未實作
