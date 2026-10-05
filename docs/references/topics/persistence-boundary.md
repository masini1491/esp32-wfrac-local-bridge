# Typed persistence boundary

## Project-owned software contract

`wfrac_persistence::IPersistence` 僅 load/store/erase 三種 records，沒有 arbitrary public key-value／credential vault。`wfrac_persistence_espidf::NvsPersistence` 依賴 portable contract；existing identity／SchemePolicy／time owner不反向依賴 adapter，也不自動 adoption。

| Surface | Bounded record / adoption boundary |
|---|---|
| Per-device Operator ID | 1..128-byte valid UTF-8、無 embedded NUL；opaque且與 MAC／Device ID／Aircon ID 分離。Load caller buffer需含 terminating NUL，size不含 NUL，容量不足不 truncate。128是 project storage bound，不是 universal firmware identity limit。 |
| Per-device scheme | 只存 `http_confirmed`／`https_confirmed`；unknown store為 invalid argument，clear必須 explicit erase。Load failure輸出unknown但error仍保留 not-found／incompatible／corrupt／storage failure，不當作 confirmed fallback evidence。 |
| Global time checkpoint | 正值 int64 Unix seconds，包含record schema版本／CRC驗證；historical metadata而非 TimeSample，無 trusted quality、elapsed-time推定或自動 seed/adopt。Future caller需明確選擇 degraded估計策略，不能當可信現在時間。 |

`DeviceKey` 是 caller-owned stable opaque namespace token：1..12 ASCII `[A-Za-z0-9_-]`。Caller負責 uniqueness，不能直接把 sensitive payload當key；本 Stage不選定 canonical persistence device identity。NVS布局：default partition，`wd_<token>` namespace的 `operator`／`scheme`；global `wf_time` namespace的 `checkpoint`。Namespace≤15字元，固定keys≤15字元，不保存敏感值作key。

Codec v1使用 `WP` magic、uint8 version、uint8 kind、LE16 payload length、typed payload與既有 CRC16-CCITT（LE16）。Checkpoint payload是LE64正值；schema future mismatch為 `incompatible_schema`，bad magic/kind/length/CRC/UTF-8/value為 `corrupt_record`；超出136-byte bound也不接受。Version mismatch不以current CRC/layout猜future格式，不做migration。CRC只檢查accidental corruption，不是tamper protection。

Output metadata在entry／failure reset，caller-owned buffers／record輸入輸出不可overlap。Portable codec／NVS scratch有bounded storage、不用explicit heap、不log payload。所有 store在backend mutation前驗證；NVS read以 READONLY open、不建namespace，write/erase explicit commit並close每個成功open的handle。Storage failure不能吞成not-found，也不保證rollback／durability／secure deletion。Caller負責serialization；erase的namespace存在檢查不是並行transaction保證。

## Evidence / runtime limits

ESP-IDF v6.0.2 `components/nvs_flash/include/nvs.h` 的 `nvs_open/get_blob/set_blob/erase_key/commit/close` 與16-byte含NUL key/namespace bound為direct API evidence；project schema／key policy是本專案決策，不是ESP-IDF或WF-RAC upstream要求。Adapter建構不呼叫NVS、不initialize partition、不自動erase/recover/migrate；future explicit runtime owner先取得必要authority與partition/security readiness。

Fake/in-memory host tests只驗證typed contract／fault classification，不提供flash durability、power-loss或hardware evidence。Compile-smoke不執行API call path；actual NVS／registration／production persistence／clock recovery尚未驗證。Security見 [security](../../security.md)，identity見 [identity](identity-registration.md)，time見 [timestamp](timestamp-time.md)。
