#!/usr/bin/env python3
"""Validate the Phase 0 machine-readable WF-RAC contract.

Read-only, deterministic, dependency-free host-side validation. This script
does not access the network and does not implement any WF-RAC runtime.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "specs" / "wf-rac-contracts.json"
EVIDENCE_LEVELS = {
    "CONFIRMED_UPSTREAM",
    "CONFIRMED_LOCAL",
    "INFERRED",
    "UNKNOWN",
    "HARDWARE_TEST_PENDING",
}
CONFIDENCE_ANNOTATIONS = {
    "STRONG_CORROBORATION",
    "PARTIAL_CORROBORATION",
    "EVIDENCE_CONFLICT",
}
FLOATING_REVISIONS = {"main", "master", "HEAD", "latest", "trunk", "develop"}


class ContractError(Exception):
    """A concise contract validation failure."""


def fail(message: str) -> None:
    raise ContractError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def is_nonempty_string(value: Any) -> bool:
    return isinstance(value, str) and bool(value.strip())


def check_evidence(value: Any, location: str) -> None:
    require(value in EVIDENCE_LEVELS, f"{location}: invalid evidence level")


def check_reference(reference: Any, index: int) -> None:
    location = f"references[{index}]"
    require(isinstance(reference, dict), f"{location}: expected object")
    for key in ("repository", "revision", "license", "purpose", "authority_boundary"):
        require(is_nonempty_string(reference.get(key)), f"{location}.{key}: required")
    revision = reference["revision"]
    require(revision not in FLOATING_REVISIONS, f"{location}.revision: floating revision")
    require(len(revision) >= 12, f"{location}.revision: must be pinned")
    check_evidence(reference.get("evidence_role"), f"{location}.evidence_role")


def check_fields(contract: dict[str, Any]) -> None:
    fields = contract["device_model_fields"]
    require(isinstance(fields, list) and fields, "device_model_fields: required non-empty list")
    names = set()
    for index, field in enumerate(fields):
        location = f"device_model_fields[{index}]"
        require(isinstance(field, dict), f"{location}: expected object")
        for key in ("canonical_name", "category", "access", "evidence_level", "validation_status", "caveat"):
            require(is_nonempty_string(field.get(key)), f"{location}.{key}: required")
        name = field["canonical_name"]
        require(name not in names, f"{location}.canonical_name: duplicate")
        names.add(name)
        check_evidence(field["evidence_level"], f"{location}.evidence_level")
        if field["evidence_level"] == "CONFIRMED_LOCAL":
            fail(f"{location}.evidence_level: local hardware claim is not allowed")
        require(field["validation_status"] == "HARDWARE_TEST_PENDING", f"{location}.validation_status: must remain pending")
    for required in ("indoor_compressor_demand", "outdoor_compressor_frequency", "direct_cns_spi_compressor_state"):
        require(required in names, f"device_model_fields: missing {required}")
    require("compressorOn" not in names, "device_model_fields: merged compressorOn is forbidden")


def check_identity_and_registration(identity: dict[str, Any]) -> None:
    require(
        identity["distinct_identities"] == ["MAC Address", "Device ID", "Aircon ID", "Operator ID"],
        "identity: identifiers must remain separate",
    )
    device_id = identity["device_id_semantics"]
    require(device_id["role"] == "client_side_identifier", "identity.device_id_semantics.role: mismatch")
    require(device_id["equivalent_to_mac_address"] is False, "identity.deviceId: cannot equal MAC by contract")
    require(device_id["equivalent_to_aircon_id"] is False, "identity.deviceId: cannot equal Aircon ID by contract")
    require(device_id["fixed_uuid_format_required"] is False, "identity.deviceId: fixed UUID is not universal")
    bootstrap = identity["bootstrap"]
    require(bootstrap["command"] == "getDeviceInfo", "identity.bootstrap: must use getDeviceInfo")
    require(bootstrap["canonical_aircon_id_source"] == "device_reported_airconId", "identity.bootstrap: canonical Aircon ID source mismatch")
    account = identity["account_id_relationship"]
    require(account["registration_account_id_source"] == "operatorId", "identity.account_id_relationship: accountId source mismatch")
    require(account["independent_identity"] is False, "identity.accountId: cannot become an independent identity")
    require(account["confidence"] == "STRONG_CORROBORATION", "identity.accountId: corroboration missing")
    safety = identity["account_table_ownership_safety"]
    require(safety["remote_list_is_reliable_enumeration"] is False, "identity.remoteList: cannot be treated as enumeration")
    require(safety["manage_only_own_operator_id"] is True, "identity.account ownership: must be limited to own operator")
    require(safety["auto_delete_unknown_accounts"] is False, "identity.account ownership: unknown accounts cannot be auto-deleted")
    require(safety["table_full_requires_explicit_action"] is True, "identity.account ownership: table-full action must be explicit")


def check_aircon_stat_r2(contract: dict[str, Any]) -> None:
    r2 = contract["aircon_stat_r2"]
    structure = r2["block_structure"]
    require(structure["state_bytes"] == 18, "aircon_stat_r2: state must be 18 bytes")
    require(structure["receive_offset"] == "command_segment_count_derived", "aircon_stat_r2: receive offset must be derived")
    require(structure["receive_raw_state_is_not_command_raw_state"] is True, "aircon_stat_r2: COMMAND/RECEIVE must remain distinct")
    require(structure["set_bits_reconstructable_from_receive"] is False, "aircon_stat_r2: set bits cannot be reconstructed")
    require(r2["runtime_implemented"] is False, "aircon_stat_r2: runtime implementation is out of scope")
    mapping = r2["fixed_state_map"]
    require(len(mapping) == 18 and [item["index"] for item in mapping] == list(range(18)), "aircon_stat_r2: fixed map must cover bytes 0..17")
    for item in mapping:
        require(item["copy_receive"] is False, f"aircon_stat_r2.fixed_state_map[{item['index']}]: must not copy RECEIVE")
    require(mapping[5]["safe_command_default"] == 255, "aircon_stat_r2.byte5: safe command default must be 0xFF")
    require(mapping[9]["receive"].find("per-indoor compressor demand") >= 0, "aircon_stat_r2.byte9: compressor concept mismatch")
    require(mapping[15]["receive"].find("unreliable") >= 0, "aircon_stat_r2.byte15: self-clean reliability boundary missing")
    require(r2["network_set_bit_mask_hex"] == "00 00 A2 88 80 00 00 00 00 00 00 10 0A 00 00 00 00 00", "aircon_stat_r2: network mask mismatch")
    require(r2["mode_encoding"]["receive"]["auto"] == "0x00" and r2["mode_encoding"]["command"]["auto"] == "0x20", "aircon_stat_r2: mode encoding mismatch")
    require(r2["fan_encoding"]["receive"]["auto"] == "0x07" and r2["fan_encoding"]["command"]["auto"] == "0x0F", "aircon_stat_r2: fan encoding mismatch")
    require("INTERNAL_REFERENCE_CONFLICT" in r2["documentation_conflict_annotations"], "aircon_stat_r2: conflict annotation missing")
    require(r2["safe_noop_query"]["command_block_not_full_write"] is True and r2["safe_noop_query"]["command_byte_5"] == 255, "aircon_stat_r2: safe no-op boundary mismatch")
    require(r2["unknown_command_byte_policy"]["copy_receive"] is False, "aircon_stat_r2: unknown bytes must not copy RECEIVE")
    require(r2["self_clean"]["supported_control_or_status_established"] is False and r2["self_clean"]["initial_device_model_control"] is False, "aircon_stat_r2: self-clean must remain unresolved")
    require(r2["write_strategy"]["full_write_is_universal_protocol_requirement"] is False and r2["write_strategy"]["selective_delta_or_full_reencode"] == "HARDWARE_TEST_PENDING", "aircon_stat_r2: write strategy boundary mismatch")


def check_operation_data_r3(contract: dict[str, Any]) -> None:
    r3 = contract["operation_data_r3"]
    expected_visibility = {"CONFIRMED_UNSOLICITED_PUSH", "CONFIRMED_QUERY_VALUE", "CONFIRMED_QUERY_RESPONSE_NO_VALUE", "CONFIRMED_QUERY_RESPONSE_UNKNOWN_SEMANTIC", "SPECIAL_APP_EXTENSION", "NO_WF_RAC_HTTP_EVIDENCE"}
    require(set(r3["visibility_values"]) == expected_visibility and r3["visibility_classification_is_separate_from_evidence"] is True, "operation_data_r3: visibility vocabulary mismatch")
    require(r3["raw_segment_fields"] == ["code", "op1", "op2", "op3"], "operation_data_r3: raw OP3 preservation missing")
    query = r3["generic_query"]
    require(query["transport_command"] == "setAirconStat" and query["response_must_be_parsed_from_same_transaction"] is True, "operation_data_r3: query transaction boundary mismatch")
    require(query["response_op1_universal_0x10"] is False and query["op1"] == "0xFF", "operation_data_r3: universal OP1 rule forbidden")
    require(r3["generic_operation_data_write"] == "OUT_OF_SCOPE", "operation_data_r3: generic write must remain out of scope")
    entries = {entry["code"] + entry.get("selector", ""): entry for entry in r3["entries"]}
    require(entries["0x80" + "0x10"]["visibility"] == "CONFIRMED_UNSOLICITED_PUSH", "operation_data_r3.0x80/0x10: visibility mismatch")
    require(entries["0x0C"]["visibility"] == "NO_WF_RAC_HTTP_EVIDENCE" and entries["0x05"]["visibility"] == "NO_WF_RAC_HTTP_EVIDENCE", "operation_data_r3: direct CNS visibility boundary missing")
    require(entries["0x13"]["value_quality"].find("calibrated_open_percentage=false") >= 0, "operation_data_r3.0x13: calibrated percentage boundary missing")
    require("UNKNOWN" in entries["0x7C"]["semantic"] and "compressorProtectionActive" in entries["0x7C"]["semantic"], "operation_data_r3.0x7C: unresolved meaning boundary missing")
    require(entries["0x34"]["semantic"].find("OP3") >= 0, "operation_data_r3.0x34: OP3 preservation missing")
    for entry in r3["entries"]:
        check_evidence(entry["evidence_level"], f"operation_data_r3.entries[{entry['code']}].evidence_level")
        require(entry["local_validation"] == "HARDWARE_TEST_PENDING", f"operation_data_r3.entries[{entry['code']}]: local validation must remain pending")


def check_capability_detection_r4(contract: dict[str, Any]) -> None:
    r4 = contract["capability_detection_r4"]
    require(r4["model_nr_raw_source"] == "AirconStat RECEIVE state[0] & 0x7F" and r4["raw_value_must_be_preserved"] is True, "capability_detection_r4: ModelNrRaw preservation missing")
    for key in ("wire_layout_group_is_separate", "app_capability_group_is_separate", "retail_model_not_derivable", "app_flags_are_not_wire_presence_proof"):
        require(r4[key] is True, f"capability_detection_r4.{key}: boundary missing")
    require(r4["app_operation_data_flag_is_generic_query_gate"] is False, "capability_detection_r4: operation_data flag cannot deny generic query")
    example = r4["model_layout_examples"][0]
    require(example == {"model_nr_raw": 3, "wire_layout_group": 2, "app_capability_group": "zt_2025"}, "capability_detection_r4: raw3/ZT boundary mismatch")
    flags = r4["app_capability_table"]
    require(len(flags) == 5 and flags["3"]["group"] == "zt_2025" and flags["3"]["flags"]["operation_data"] is True, "capability_detection_r4: capability table mismatch")
    require(flags["1"]["flags"]["operation_data"] is False, "capability_detection_r4: raw1 operation_data flag mismatch")
    require(r4["official_app_unknown_fallback"] == "separate_2021" and "conservative" in r4["unknown_model_policy"], "capability_detection_r4: unknown model policy mismatch")
    require("self_clean_operation does not establish remote start/status/control" in r4["optional_feature_gates"]["self_clean"], "capability_detection_r4: self-clean boundary missing")
    require(r4["firmware_metadata_role"] == "compatibility_and_quirk_context_only" and r4["retail_model_derivable_from_wire"] is False, "capability_detection_r4: metadata/model boundary mismatch")
    require(r4["known_protocol_modes"] == ["auto", "cool", "heat", "fan", "dry"] and r4["per_model_hvac_mode_discovery"] == "UNKNOWN" and r4["local_model_mode_support"] == "HARDWARE_TEST_PENDING", "capability_detection_r4: HVAC mode boundary mismatch")
    require(r4["local_validation_status"] == "HARDWARE_TEST_PENDING", "capability_detection_r4: local validation must remain pending")


def check_time_architecture_r5(contract: dict[str, Any]) -> None:
    r5 = contract["time_architecture_r5"]
    esp_reference = next((reference for reference in contract["references"] if reference.get("repository") == "espressif/esp-idf"), None)
    require(esp_reference is not None and esp_reference["revision"] == "0276131402f5383545ce5fc74f71d548fe5f3ffc" and "not WF-RAC protocol authority" in esp_reference["authority_boundary"], "time_architecture_r5: ESP-IDF reference boundary mismatch")
    timestamp = r5["wf_rac_timestamp"]
    for key in ("required", "non_zero", "writes_module_time_global", "client_is_time_source"):
        require(timestamp[key] is True, f"time_architecture_r5.timestamp.{key}: invariant missing")
    require(timestamp["unit"] == "unix_seconds" and timestamp["firmware_parse_64_bit"] is True, "time_architecture_r5.timestamp: unit/width mismatch")
    for key in ("freshness_validation_established", "replay_protection_role_established", "cryptographic_authentication_role_established"):
        require(timestamp[key] is False, f"time_architecture_r5.timestamp.{key}: security role cannot be finalized")
    require(timestamp["fixed_timestamp_fallback_allowed"] is False and timestamp["module_has_independent_rtc"] is False, "time_architecture_r5.timestamp: clock source boundary mismatch")
    require(r5["timezone"]["separate_from_unix_epoch"] is True and r5["timezone"]["exact_runtime_effect"] == "UNKNOWN", "time_architecture_r5: timezone boundary mismatch")
    expires = r5["expires"]
    require(expires["duration"] == "UNKNOWN" and expires["refresh_semantics"] == "UNKNOWN" and expires["universal_schedule_constant"] is False, "time_architecture_r5: expires must remain unknown")
    esp = r5["esp_idf"]
    require(esp["rtc_survives_non_power_on_reset"] is True and esp["power_on_reset_resets_rtc_timer"] is True, "time_architecture_r5: reset boundary mismatch")
    require(esp["sntp_is_optional_time_acquisition_method"] is True and "not WF-RAC protocol authority" in esp["authority_boundary"], "time_architecture_r5: ESP-IDF authority boundary mismatch")
    provider = r5["future_time_provider"]
    for key in ("daily_control_requires_internet", "daily_control_requires_dns", "daily_control_requires_ntp"):
        require(provider[key] is False, f"time_architecture_r5.provider.{key}: daily control dependency forbidden")
    require(provider["time_quality_model_required"] is True, "time_architecture_r5: time quality model is required")
    require(provider["portable_boundary_implemented"] is True, "time_architecture_r5: portable TimeProvider boundary must be implemented")
    require(provider["quality_states"] == ["unavailable", "estimated", "trusted"], "time_architecture_r5: implemented TimeQuality states mismatch")
    require(provider["esp_idf_system_time_adapter_implemented"] is True and provider["explicit_seed_adopt_implemented"] is True, "time_architecture_r5: ESP-IDF system-time adapter boundary must be implemented")
    for key in ("time_acquisition_implemented", "persistence_implemented", "power_loss_recovery_implemented", "sntp_implemented"):
        require(provider[key] is False, f"time_architecture_r5.provider.{key}: acquisition/persistence/recovery/SNTP remains pending")
    require(r5["persisted_checkpoint"]["is_trusted_current_time"] is False, "time_architecture_r5: checkpoint cannot be trusted current time")
    require(r5["power_loss"]["exact_elapsed_wall_time_after_power_loss_without_external_source"] == "unavailable", "time_architecture_r5: power-loss boundary mismatch")
    require(r5["external_rtc"]["selected"] is False and r5["external_rtc"]["required_by_current_architecture"] is False, "time_architecture_r5: external RTC boundary mismatch")
    require(r5["registration_correction"]["unknown_account_deletion"] is False, "time_architecture_r5: ownership safety mismatch")
    require(r5["runtime_implemented"] is False, "time_architecture_r5: runtime implementation is out of scope")


def check_temperature_semantics_r6(contract: dict[str, Any]) -> None:
    r6 = contract["temperature_semantics_r6"]
    require(r6["runtime_implemented"] is False, "temperature_semantics_r6: runtime implementation is out of scope")
    basic = r6["basic_concepts"]
    require(basic["target_setpoint"]["decode"] == "raw / 2 C" and basic["target_setpoint"]["is_ambient_measurement"] is False, "temperature_semantics_r6: target boundary mismatch")
    require(basic["controller_room_temperature"]["is_canonical_0x80_indoor_alias"] is False, "temperature_semantics_r6: controller/ambient alias forbidden")
    require(basic["external_room_temperature_injection"]["no_injection_value"] == "0xFF" and basic["external_room_temperature_injection"]["write_safety"] == "HARDWARE_TEST_PENDING", "temperature_semantics_r6: injection boundary mismatch")
    ambient = r6["network_ambient_0x80"]
    require(ambient["decoder"] == "official Smart M-Air derived lookup table" and ambient["table_count"] == 2 and ambient["entries_per_table"] == 256, "temperature_semantics_r6: network lookup metadata mismatch")
    require(ambient["nonlinear"] is True and ambient["fixed_temperature_increment_per_raw_code"] is False, "temperature_semantics_r6: nonlinear lookup boundary mismatch")
    resolution = r6["representation_layer_resolution"]
    require(resolution["network_decoder_conflict_status"] == "resolved_by_representation_layer" and resolution["direct_cns_formula_applicable_to_wfrac_network_op2"] is False, "temperature_semantics_r6: representation resolution mismatch")
    require(r6["lookup_table_process"]["full_lookup_tables_embedded"] is False, "temperature_semantics_r6: full lookup table embedding forbidden")
    diagnostics = r6["diagnostic_channels"]
    for code in ("0x81", "0x87", "0x85"):
        require(diagnostics[code]["raw_preservable"] is True, f"temperature_semantics_r6.{code}: raw preservation missing")
        require(diagnostics[code]["precision_calibration_guaranteed"] is False if code != "0x85" else diagnostics[code]["low_raw_semantics_universally_established"] is False, f"temperature_semantics_r6.{code}: calibration boundary mismatch")
    require(diagnostics["0x81"]["highest_thermometer_supported_raw_anchor"] == 170 and diagnostics["0x81"]["above_anchor"] == "EXTRAPOLATED / INFERRED", "temperature_semantics_r6.0x81: anchor boundary mismatch")
    require(diagnostics["0x87"]["mode_independent_outlet_semantic"] is False, "temperature_semantics_r6.0x87: outlet semantic boundary mismatch")
    require(diagnostics["0x82"]["raw_only"] is True and diagnostics["0x82"]["reuse_indoor_coil_curve"] is False, "temperature_semantics_r6.0x82: raw-only boundary mismatch")
    require(diagnostics["0xB1"]["raw_only"] is True and diagnostics["0xB1"]["celsius_conversion_established"] is False, "temperature_semantics_r6.0xB1: raw-only boundary mismatch")
    require(r6["matter_mapping"] == "PENDING" and r6["local_validation_status"] == "HARDWARE_TEST_PENDING", "temperature_semantics_r6: pending boundaries missing")


def check_transport_security_r7(contract: dict[str, Any]) -> None:
    r7 = contract["transport_security_r7"]
    require(r7["runtime_implemented"] is False and r7["port"] == 51443, "transport_security_r7: runtime/port boundary mismatch")
    require(r7["initial_detection"]["unknown_device_order"] == ["HTTPS", "HTTP compatibility fallback"] and r7["initial_detection"]["probe_command"] == "getDeviceInfo", "transport_security_r7: HTTPS-first detection mismatch")
    require(r7["mdns"]["advertises_transport_scheme"] is False, "transport_security_r7: mDNS cannot declare scheme")
    sticky = r7["https_sticky"]
    require(sticky["automatic_silent_downgrade_to_http"] is False and sticky["application_refusal_triggers_downgrade"] is False and sticky["transient_transport_failure_permanent_downgrade"] is False, "transport_security_r7: HTTPS sticky boundary mismatch")
    profile = r7["compatibility_profile"]
    require(profile["preferred_tls"] == "TLS 1.2" and profile["preferred_http"] == "HTTP/1.1" and profile["universal_firmware_minimum"] is False, "transport_security_r7: compatibility profile mismatch")
    legacy = r7["legacy_tls"]
    require(legacy["exact_min_tls"] == "UNKNOWN" and legacy["exact_cipher_suite_requirements"] == "UNKNOWN" and legacy["global_security_level_downgrade_allowed"] is False, "transport_security_r7: legacy TLS boundary mismatch")
    cert = r7["certificate"]
    require(cert["self_signed_observed"] is True and cert["cn_ip_mismatch_observed"] is True and cert["server_identity_verification_preferred"] is True, "transport_security_r7: certificate observations/policy mismatch")
    require(cert["certificate_identity_strategy"] == "PENDING_HARDWARE_VALIDATION" and cert["unverified_https_classification"] == "encrypted_unauthenticated_transport" and cert["verify_off_preferred"] is False, "transport_security_r7: certificate assurance boundary mismatch")
    http = r7["http_classification"]
    require(http["confidentiality"] == "none" and http["server_identity"] == "none", "transport_security_r7: HTTP security classification mismatch")
    time = r7["r5_time_boundary"]
    require(time["daily_control_requires_internet"] is False and time["daily_control_requires_dns"] is False and time["daily_control_requires_ntp"] is False and time["tls_identity_verification_must_not_require_public_ntp_for_daily_control"] is True, "transport_security_r7: R5 dependency boundary mismatch")
    auth = r7["authentication_boundaries"]
    require(auth["local_api_mutual_tls_required"] is False and auth["cloud_mqtt_security_is_separate_boundary"] is True and auth["operator_id_is_transport_authentication"] is False, "transport_security_r7: authentication boundary mismatch")
    require(r7["local_validation_status"] == "HARDWARE_TEST_PENDING", "transport_security_r7: local validation must remain pending")


def validate(contract: dict[str, Any]) -> None:
    required_sections = {
        "schema_version",
        "status",
        "project_license",
        "evidence_vocabulary",
        "references",
        "api_envelope",
        "identity_and_registration",
        "device_model_fields",
        "command_intents",
        "command_intent_boundaries",
        "result_and_error_taxonomy",
    }
    missing = required_sections - contract.keys()
    require(not missing, f"top-level: missing {', '.join(sorted(missing))}")

    evidence = contract["evidence_vocabulary"]
    require(set(evidence["levels"]) == EVIDENCE_LEVELS, "evidence_vocabulary.levels: mismatch")
    require(evidence["current_confirmed_local_count"] == 0, "evidence_vocabulary.current_confirmed_local_count: must be zero")
    require(set(evidence["confidence_annotations"]) == CONFIDENCE_ANNOTATIONS, "confidence annotations: mismatch")
    require(evidence["confidence_annotations_are_not_evidence_levels"] is True, "confidence annotations: must be separate")

    references = contract["references"]
    require(isinstance(references, list) and references, "references: required non-empty list")
    for index, reference in enumerate(references):
        check_reference(reference, index)

    api = contract["api_envelope"]
    require(api["port"] == 51443, "api_envelope.port: expected 51443")
    require(api["path_pattern"] == "/beaver/command/<command>", "api_envelope.path_pattern: mismatch")
    require(api["method"] == "POST", "api_envelope.method: expected POST")
    require(api["result_semantics"] == "command_specific", "api_envelope.result_semantics: must be command-specific")
    require(api["request_fields"]["contents"]["required"] is False, "api_envelope.contents: must be optional")

    identity = contract["identity_and_registration"]
    check_identity_and_registration(identity)
    require(identity["operator_id_generation_or_persistence_implemented"] is False, "identity: runtime persistence is forbidden")

    check_fields(contract)
    check_aircon_stat_r2(contract)
    check_operation_data_r3(contract)
    check_capability_detection_r4(contract)
    check_time_architecture_r5(contract)
    check_temperature_semantics_r6(contract)
    check_transport_security_r7(contract)

    boundaries = contract["command_intent_boundaries"]
    for key in ("intent_only", "does_not_encode_aircon_stat", "does_not_build_json", "does_not_call_http", "does_not_manage_retry_discovery_socket_or_matter_callbacks"):
        require(boundaries[key] is True, f"command_intent_boundaries.{key}: must be true")

    taxonomy = contract["result_and_error_taxonomy"]
    require(taxonomy["protocol_result_is_command_specific"] is True, "result taxonomy: command-specific rule missing")
    require(taxonomy["global_result_2_meaning_forbidden"] is True, "result taxonomy: global result 2 meaning forbidden")
    contexts = taxonomy["result_two_contexts"]
    for command in ("getDeviceInfo", "getAirconStat", "setAirconStat", "updateAccountInfo", "deleteAccountInfo"):
        require(command in contexts, f"result taxonomy.result_two_contexts: missing {command}")
    require(contexts["getDeviceInfo"] == "UNKNOWN", "result taxonomy: getDeviceInfo result 2 must remain UNKNOWN")
    require(contexts["deleteAccountInfo"] == "UNKNOWN", "result taxonomy: deleteAccountInfo result 2 must remain UNKNOWN")
    require(taxonomy["observed_result_code_family"] == [0, 1, 2, 10, 11, 12, 20, 99, 429], "result taxonomy: observed code family mismatch")
    require(taxonomy["timeout_is_protocol_constant"] is False, "result taxonomy: timeout cannot be universal constant")
    require(taxonomy["retry_count_is_protocol_constant"] is False, "result taxonomy: retry count cannot be universal constant")
    require(taxonomy["tls_verification_policy"] in {"UNKNOWN", "INFERRED", "HARDWARE_TEST_PENDING"}, "result taxonomy.tls_verification_policy: finalized policy is forbidden")
    require(taxonomy["generic_operation_data_write"] == "OUT_OF_SCOPE", "result taxonomy: generic write must be out of scope")

    uncertainties = set(contract["known_uncertainties"])
    require("exact CNS-to-WF-RAC temperature representation transform and local hardware validation" in uncertainties, "known_uncertainties: 0x80 representation boundary missing")
    require("Matter optional feature mapping" in uncertainties, "known_uncertainties: Matter mapping pending")


def main() -> int:
    try:
        with CONTRACT.open("r", encoding="utf-8") as handle:
            contract = json.load(handle)
        require(isinstance(contract, dict), "contract: top-level must be an object")
        validate(contract)
    except FileNotFoundError:
        print(f"FAIL contract: not found at {CONTRACT}", file=sys.stderr)
        return 1
    except json.JSONDecodeError as error:
        print(f"FAIL JSON at line {error.lineno}, column {error.colno}", file=sys.stderr)
        return 1
    except (KeyError, TypeError) as error:
        print(f"FAIL contract structure: {error}", file=sys.stderr)
        return 1
    except ContractError as error:
        print(f"FAIL {error}", file=sys.stderr)
        return 1
    print(f"PASS {CONTRACT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
