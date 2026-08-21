# 安全性

可能包含敏感資訊的項目包括 Wi-Fi credentials、Operator ID／controller identity、Matter fabric data、commissioning secrets、OTA signing keys，以及未來的 WebUI authentication secrets。

不得在 Git、README、文件範例、log、serial output 或 crash report 中提交或暴露真實 credentials。目前 Phase 0 不實作 encrypted NVS、Secure Boot 或 Flash Encryption；這些列為未來 hardening roadmap 項目。Local-first 控制不得把 cloud credentials 變成日常 runtime dependency。
