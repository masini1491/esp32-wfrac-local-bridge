# Identity / registration

## Scope

Identity、Operator ID、account table ownership 與 registration lifecycle。

## Authoritative project conclusions

- MAC Address、Device ID、Aircon ID、Operator ID 必須分離。`deviceId` 是 stable、non-empty、opaque client identifier，不等於 MAC／Aircon ID，UUID 不是 universal requirement。
- `getDeviceInfo` 不需預先知道 Aircon ID；canonical Aircon ID 是 device-reported `airconId`。
- Self-managed 模式持久化自身 Operator ID 並使用 `updateAccountInfo`／`deleteAccountInfo`；mirror 模式使用既有 Operator ID，不自行 register/deregister。`accountId = operatorId` 為 strong corroboration。
- 觀察 account slots 上限為 4；`remoteList` 不可靠，不得自動刪除未知 accounts，只管理自己的 Operator ID。正常 reboot 不應自動 deregister。`aws` bypass 不是 project architecture。
- `result = 2` 只可在 command context 解讀；expires lifecycle、remote 完整語意與 timezone effect 仍未知。時間細節見 [timestamp-time.md](timestamp-time.md)。

## High-risk invariants

不得把四種 identity 合併、把 accountId 當第五種 identity，或把 upstream registration evidence 宣稱為 local validation。

## Known unknowns / pending validation

Device-specific hidden semantics、remote `0/1`、expires calculation／refresh、registration expiry recovery：`UNKNOWN`／`HARDWARE_TEST_PENDING`。

## Related machine-readable contract

`identity_and_registration`、`result_and_error_taxonomy`。

## Provenance pointers

見 [mitsubishi-wf-rac-integration.md](../sources/mitsubishi-wf-rac-integration.md) 與 [homebridge-mhi-wfrac.md](../sources/homebridge-mhi-wfrac.md)。
