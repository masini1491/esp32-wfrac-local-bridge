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
    require(identity["distinct_identities"] == ["MAC Address", "Device ID", "Aircon ID", "Operator ID"], "identity: identifiers must remain separate")
    require(identity["operator_id_generation_or_persistence_implemented"] is False, "identity: runtime persistence is forbidden")

    check_fields(contract)

    boundaries = contract["command_intent_boundaries"]
    for key in ("intent_only", "does_not_encode_aircon_stat", "does_not_build_json", "does_not_call_http", "does_not_manage_retry_discovery_socket_or_matter_callbacks"):
        require(boundaries[key] is True, f"command_intent_boundaries.{key}: must be true")

    taxonomy = contract["result_and_error_taxonomy"]
    require(taxonomy["protocol_result_is_command_specific"] is True, "result taxonomy: command-specific rule missing")
    require(taxonomy["global_result_2_meaning_forbidden"] is True, "result taxonomy: global result 2 meaning forbidden")
    require(taxonomy["timeout_is_protocol_constant"] is False, "result taxonomy: timeout cannot be universal constant")
    require(taxonomy["retry_count_is_protocol_constant"] is False, "result taxonomy: retry count cannot be universal constant")
    require(taxonomy["tls_verification_policy"] in {"UNKNOWN", "INFERRED", "HARDWARE_TEST_PENDING"}, "result taxonomy.tls_verification_policy: finalized policy is forbidden")
    require(taxonomy["generic_operation_data_write"] == "OUT_OF_SCOPE", "result taxonomy: generic write must be out of scope")

    uncertainties = set(contract["known_uncertainties"])
    require("0x80 outdoor temperature conversion conflict" in uncertainties, "known_uncertainties: 0x80 conflict missing")
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
