# 安全性

可能包含敏感資訊的項目包括 Wi-Fi credentials、Operator ID／controller identity、Matter fabric data、commissioning secrets、OTA signing keys，以及未來的 WebUI authentication secrets。

不得在 Git、README、文件範例、log、serial output 或 crash report 中提交或暴露真實 credentials。目前 Phase 0 不實作 encrypted NVS、Secure Boot 或 Flash Encryption；這些列為未來 hardening roadmap 項目。Local-first 控制不得把 cloud credentials 變成日常 runtime dependency。

Generic operation-data write 可能直接影響 AC main board；未知 code 禁止 exploratory write。`setNetworkInfo` 可修改 WF-RAC Wi-Fi provisioning，不屬於日常 runtime control，未來若使用需獨立安全設計。`updateFirmware` 本 project 不應自行操作，除非未來有明確需求與安全研究。

HTTPS self-signed certificate 與 legacy TLS compatibility 需要明確 security policy。不同 firmware branch 的 TLS strategy 可能不同；hostname verification、certificate verification、TLS compatibility 或 OpenSSL security level workaround 都只是上游 implementation compatibility strategy，不是 universal protocol requirement。Production 不應在沒有文件化決策前永久停用 TLS verification。Operator ID 仍視為敏感設定。

Direct CNS/SPI write semantics 不能直接視為 WF-RAC HTTP write 已驗證；任何未經 Smart M-Air／WF-RAC path 驗證的 write 仍需獨立 safety review。
