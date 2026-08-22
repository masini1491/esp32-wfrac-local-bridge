"""Targeted host tests for the Phase 0 contract validator."""

from __future__ import annotations

import copy
import json
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import validate_contracts  # noqa: E402


CONTRACT_PATH = ROOT / "specs" / "wf-rac-contracts.json"


class ContractValidatorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        with CONTRACT_PATH.open("r", encoding="utf-8") as handle:
            cls.canonical = json.load(handle)

    def assert_rejected(self, mutate) -> None:
        candidate = copy.deepcopy(self.canonical)
        mutate(candidate)
        with self.assertRaises(validate_contracts.ContractError):
            validate_contracts.validate(candidate)

    def test_canonical_contract_is_accepted(self) -> None:
        validate_contracts.validate(copy.deepcopy(self.canonical))

    def test_invalid_evidence_value_is_rejected(self) -> None:
        self.assert_rejected(lambda c: c["references"][0].update(evidence_role="NOT_EVIDENCE"))

    def test_confidence_annotation_cannot_enter_evidence_vocabulary(self) -> None:
        self.assert_rejected(
            lambda c: c["evidence_vocabulary"]["levels"].append("EVIDENCE_CONFLICT")
        )

    def test_confirmed_local_hardware_claim_is_rejected(self) -> None:
        self.assert_rejected(
            lambda c: c["device_model_fields"][0].update(evidence_level="CONFIRMED_LOCAL")
        )

    def test_reference_metadata_must_be_pinned_and_attributed(self) -> None:
        mutations = (
            lambda c: c["references"][0].pop("revision"),
            lambda c: c["references"][0].pop("license"),
            lambda c: c["references"][0].pop("authority_boundary"),
            lambda c: c["references"][0].update(revision="main"),
        )
        for mutation in mutations:
            with self.subTest(mutation=mutation):
                self.assert_rejected(mutation)

    def test_identity_merge_is_rejected(self) -> None:
        self.assert_rejected(
            lambda c: c["identity_and_registration"].update(
                distinct_identities=["compressorOn"]
            )
        )

    def test_device_id_cannot_be_declared_as_aircon_identity(self) -> None:
        self.assert_rejected(
            lambda c: c["identity_and_registration"]["device_id_semantics"].update(
                equivalent_to_aircon_id=True
            )
        )

    def test_aircon_id_bootstrap_source_is_get_device_info(self) -> None:
        self.assert_rejected(
            lambda c: c["identity_and_registration"]["bootstrap"].update(
                canonical_aircon_id_source="macAddress"
            )
        )

    def test_account_id_is_not_a_fifth_identity(self) -> None:
        self.assert_rejected(
            lambda c: c["identity_and_registration"]["account_id_relationship"].update(
                independent_identity=True
            )
        )

    def test_unknown_accounts_cannot_be_auto_deleted(self) -> None:
        self.assert_rejected(
            lambda c: c["identity_and_registration"]["account_table_ownership_safety"].update(
                auto_delete_unknown_accounts=True
            )
        )

    def test_result_two_universal_meaning_is_rejected(self) -> None:
        self.assert_rejected(
            lambda c: c["result_and_error_taxonomy"].update(
                global_result_2_meaning_forbidden=False
            )
        )

    def test_result_two_contexts_must_remain_command_specific(self) -> None:
        self.assert_rejected(
            lambda c: c["result_and_error_taxonomy"]["result_two_contexts"].update(
                getDeviceInfo="account_table_full"
            )
        )

    def test_timeout_and_retry_constants_are_rejected(self) -> None:
        for key in ("timeout_is_protocol_constant", "retry_count_is_protocol_constant"):
            with self.subTest(key=key):
                self.assert_rejected(
                    lambda c, key=key: c["result_and_error_taxonomy"].update({key: True})
                )

    def test_outdoor_temperature_conflict_must_remain_known(self) -> None:
        self.assert_rejected(
            lambda c: c["known_uncertainties"].remove(
                "0x80 outdoor temperature conversion conflict"
            )
        )

    def test_tls_policy_must_remain_unfinalized(self) -> None:
        self.assert_rejected(
            lambda c: c["result_and_error_taxonomy"].update(
                tls_verification_policy="PRODUCTION_FINALIZED"
            )
        )

    def test_matter_mapping_must_remain_pending(self) -> None:
        self.assert_rejected(
            lambda c: c["known_uncertainties"].remove("Matter optional feature mapping")
        )

    def test_hardware_validation_must_remain_pending(self) -> None:
        self.assert_rejected(
            lambda c: c["device_model_fields"][0].update(validation_status="PASS")
            )

    def test_aircon_stat_receive_command_boundary_is_preserved(self) -> None:
        self.assert_rejected(lambda c: c["aircon_stat_r2"]["block_structure"].update(receive_raw_state_is_not_command_raw_state=False))

    def test_aircon_stat_byte5_no_injection_default_is_preserved(self) -> None:
        self.assert_rejected(lambda c: c["aircon_stat_r2"]["fixed_state_map"][5].update(safe_command_default=0))

    def test_aircon_stat_unknown_command_bytes_cannot_copy_receive(self) -> None:
        self.assert_rejected(lambda c: c["aircon_stat_r2"]["fixed_state_map"][13].update(copy_receive=True))

    def test_aircon_stat_mode_layers_remain_distinct(self) -> None:
        self.assert_rejected(lambda c: c["aircon_stat_r2"]["mode_encoding"]["receive"].update(auto="0x20"))

    def test_aircon_stat_compressor_demand_is_not_universal_running_state(self) -> None:
        self.assert_rejected(lambda c: c["aircon_stat_r2"]["fixed_state_map"][9].update(receive="outdoor compressor running"))

    def test_aircon_stat_full_write_is_not_universal(self) -> None:
        self.assert_rejected(lambda c: c["aircon_stat_r2"]["write_strategy"].update(full_write_is_universal_protocol_requirement=True))

    def test_aircon_stat_self_clean_remains_unresolved(self) -> None:
        self.assert_rejected(lambda c: c["aircon_stat_r2"]["self_clean"].update(supported_control_or_status_established=True))


if __name__ == "__main__":
    unittest.main()
