# 安全性

可能包含敏感資訊的項目包括 Wi-Fi credentials、Operator ID／controller identity、Matter fabric data、commissioning secrets、OTA signing keys，以及未來的 WebUI authentication secrets。

不得在 Git、README、文件範例、log、serial output 或 crash report 中提交或暴露真實 credentials。目前 Phase 0 不實作 encrypted NVS、Secure Boot 或 Flash Encryption；這些列為未來 hardening roadmap 項目。Local-first 控制不得把 cloud credentials 變成日常 runtime dependency。

Generic operation-data write 可能直接影響 AC main board；未知 code 禁止 exploratory write。`setNetworkInfo` 可修改 WF-RAC Wi-Fi provisioning，不屬於日常 runtime control，未來若使用需獨立安全設計。`updateFirmware` 本 project 不應自行操作，除非未來有明確需求與安全研究。

HTTPS self-signed certificate 與 legacy TLS compatibility 需要明確 security policy。不同 firmware branch 的 TLS strategy 可能不同；hostname verification、certificate verification、TLS compatibility 或 OpenSSL security level workaround 都只是上游 implementation compatibility strategy，不是 universal protocol requirement。Production 不應在沒有文件化決策前永久停用 TLS verification。Operator ID 仍視為敏感設定。

Direct CNS/SPI write semantics 不能直接視為 WF-RAC HTTP write 已驗證；任何未經 Smart M-Air／WF-RAC path 驗證的 write 仍需獨立 safety review。

Diagnostic WebUI exception 僅屬 `wfrac_bootstrap_probe`，兩個 enable switches 均 default-off。Temporary SoftAP 必須由本地 build configuration 提供 unique 8–63-byte WPA2 passphrase（預設空值 fail closed，不得提交或記錄）；STA/WF-RAC inputs 只接受 bounded POST、RAM-only Wi-Fi storage，driver NVS 與 PHY calibration persistence 關閉，不新增 application credential persistence。UI 僅接受 AP interface、固定 Host／same-origin POST 與 single-use nonce，不對 joined STA LAN 提供介面；nonce 是防 replay/CSRF guard，不是 production authentication。Local HTTP 的 confidentiality 只依隔離 lab 的 WPA2 link，不是 end-to-end TLS，AP passphrase 存在本地 configuration／firmware image；browser／build host 與其 artifacts 的秘密保護由操作者負責，不能宣稱 AP secret 僅在 RAM。

每次 explicit submit 最多一次 bounded STA connect 與一次 read-only getDeviceInfo，HTTPS 需 explicit PEM trust／正常 identity verification，不 downgrade；numeric IPv4 endpoint 不需 DNS。完成或失敗後清除 firmware-local form/request/response buffers、STA configuration 並回 AP-only；cleanup failure 禁止後續 submit。Result 只含 sanitized stage／numeric codes／raw command-specific result／parse flags，不回顯 IDs、credentials 或 response body。此 implementation／offline test／compile evidence 不授權 actual WebUI/Wi-Fi/WF-RAC/hardware execution，也不是 production Provisioning/WebUI 或敏感 provisioning／firmware mutation 的授權。
