# Reference index

本目錄是 task-oriented reference knowledge base。`topics/` 是 Codex 日常閱讀入口；`sources/` 只在 provenance、source conflict 或 evidence audit 時閱讀。Machine-readable contract 位於 [../../specs/wf-rac-contracts.json](../../specs/wf-rac-contracts.json)；[../protocol-research.md](../protocol-research.md) 已降級為 high-level legacy/index 入口。

## Default Codex reading policy

1. 先讀本 README。
2. 只讀與任務直接相關的 `topics/*.md`。
3. 只有 provenance、source conflict 或 evidence audit 才讀 `sources/*.md`。
4. 不得預設讀全部 topics、全部 sources 或完整讀 `docs/protocol-research.md`。
5. 若 topic 已足夠，停止擴大 context；evidence 不足時才逐層擴大。

| 任務 | Primary topic |
|---|---|
| Identity / Operator / registration | [topics/identity-registration.md](topics/identity-registration.md) |
| API envelope / handlers / discovery | [topics/api-envelope-discovery.md](topics/api-envelope-discovery.md) |
| HTTP / HTTPS / TLS | [topics/tls-transport.md](topics/tls-transport.md) |
| AirconStat fixed bytes / set-bits | [topics/airconstat-fixed-state.md](topics/airconstat-fixed-state.md) |
| Operation-data | [topics/operation-data.md](topics/operation-data.md) |
| ModelNrRaw / optional capabilities | [topics/capability-model.md](topics/capability-model.md) |
| timestamp / expires / timezone | [topics/timestamp-time.md](topics/timestamp-time.md) |
| temperature conversion | [topics/temperature-conversion.md](topics/temperature-conversion.md) |

## Machine-readable

[../../specs/wf-rac-contracts.json](../../specs/wf-rac-contracts.json)

## Source dossiers

[sources/](sources/)
