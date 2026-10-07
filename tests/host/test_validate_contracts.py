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
                "exact CNS-to-WF-RAC temperature representation transform and local hardware validation"
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

    def test_operation_data_direct_cns_visibility_is_not_wf_rac_confirmed(self) -> None:
        self.assert_rejected(lambda c: next(e for e in c["operation_data_r3"]["entries"] if e["code"] == "0x0C").update(visibility="CONFIRMED_QUERY_VALUE"))

    def test_operation_data_query_uses_same_set_transaction(self) -> None:
        self.assert_rejected(lambda c: c["operation_data_r3"]["generic_query"].update(transport_command="getAirconStat"))

    def test_operation_data_response_must_preserve_op3(self) -> None:
        self.assert_rejected(lambda c: c["operation_data_r3"].update(raw_segment_fields=["code", "op1", "op2"]))

    def test_operation_data_7c_remains_unresolved(self) -> None:
        self.assert_rejected(lambda c: next(e for e in c["operation_data_r3"]["entries"] if e["code"] == "0x7C").update(semantic="compressorProtectionActive"))

    def test_operation_data_7c_negative_semantic_guards_are_required(self) -> None:
        def entry(contract):
            return next(e for e in contract["operation_data_r3"]["entries"] if e["code"] == "0x7C")

        mutations = (
            lambda c: entry(c).update(visibility="NO_WF_RAC_HTTP_EVIDENCE"),
            lambda c: entry(c).update(scope="shared_outdoor"),
            lambda c: entry(c).update(value_quality="raw_only"),
            lambda c: entry(c).update(evidence_level="INFERRED"),
            lambda c: entry(c).update(local_validation="CONFIRMED_LOCAL"),
            lambda c: entry(c)["interpretation_guards"].remove("zero_does_not_mean_protection_inactive_or_false"),
            lambda c: entry(c)["interpretation_guards"].remove("nonzero_meaning_unknown"),
            lambda c: entry(c)["interpretation_guards"].remove("not_general_protection_active_boolean"),
            lambda c: entry(c)["interpretation_guards"].remove("not_overload_clamp_boolean"),
            lambda c: entry(c)["interpretation_guards"].remove("not_speed_limit_frost_defrost_or_generic_fault_boolean"),
        )
        for mutation in mutations:
            with self.subTest(mutation=mutation):
                self.assert_rejected(mutation)

    def test_operation_data_1e_1f_response_no_value_guards(self) -> None:
        def entry(contract, code):
            return next(e for e in contract["operation_data_r3"]["entries"] if e["code"] == code)

        for code in ("0x1E", "0x1F"):
            mutations = (
                lambda c, code=code: entry(c, code).update(visibility="CONFIRMED_QUERY_VALUE"),
                lambda c, code=code: entry(c, code).update(response_shape="00 00 00"),
                lambda c, code=code: entry(c, code).update(semantic="universally unsupported"),
                lambda c, code=code: entry(c, code).update(scope="per_indoor"),
                lambda c, code=code: entry(c, code).update(value_quality="zero"),
                lambda c, code=code: entry(c, code).update(universally_unsupported=True),
                lambda c, code=code: entry(c, code).update(universal_sentinel=True),
                lambda c, code=code: entry(c, code).update(no_value_cause="CONFIRMED_DB6_SELECTOR"),
                lambda c, code=code: entry(c, code).update(cause_hypothesis="DB6 selector confirmed"),
                lambda c, code=code: entry(c, code).update(local_validation="CONFIRMED_LOCAL"),
            )
            for mutation in mutations:
                with self.subTest(code=code, mutation=mutation):
                    self.assert_rejected(mutation)

    def test_unknown_operation_data_response_shapes_are_not_generalized(self) -> None:
        def entry(contract, code):
            return next(e for e in contract["operation_data_r3"]["entries"] if e["code"] == code)

        for code in ("0x0D", "0x21", "0x32", "0x34", "0x35"):
            mutations = (
                lambda c, code=code: entry(c, code).update(semantic="temperature"),
                lambda c, code=code: entry(c, code).update(scope="shared outdoor"),
                lambda c, code=code: entry(c, code).update(value_quality="raw_only"),
                lambda c, code=code: entry(c, code).update(response_shape="00 00 00"),
                lambda c, code=code: entry(c, code).update(op1_is_universal_fixed_0x10=True),
                lambda c, code=code: entry(c, code).update(observed_shape_is_universal_constant=True),
                lambda c, code=code: entry(c, code).update(ff_is_universal_sentinel=True),
                lambda c, code=code: entry(c, code).update(raw_preserved_fields=["code", "OP1", "OP2"]),
                lambda c, code=code: entry(c, code).update(local_validation="CONFIRMED_LOCAL"),
            )
            for mutation in mutations:
                with self.subTest(code=code, mutation=mutation):
                    self.assert_rejected(mutation)

        self.assert_rejected(lambda c: next(e for e in c["operation_data_r3"]["entries"] if e["code"] == "0x34").update(single_byte_only_interpretation=True))
        self.assert_rejected(lambda c: next(e for e in c["operation_data_r3"]["entries"] if e["code"] == "0x34").update(op3_participates_in_observed_shape=False))

    def test_operation_data_13_is_not_calibrated_percentage(self) -> None:
        self.assert_rejected(lambda c: next(e for e in c["operation_data_r3"]["entries"] if e["code"] == "0x13").update(value_quality="calibrated_open_percentage=true"))

    def test_operation_data_generic_write_remains_out_of_scope(self) -> None:
        self.assert_rejected(lambda c: c["operation_data_r3"].update(generic_operation_data_write="SUPPORTED"))

    def test_capability_uses_preserved_raw_model_number(self) -> None:
        self.assert_rejected(lambda c: c["capability_detection_r4"].update(raw_value_must_be_preserved=False))

    def test_raw3_capability_is_not_overwritten_by_wire_layout_group(self) -> None:
        self.assert_rejected(lambda c: c["capability_detection_r4"]["model_layout_examples"][0].update(app_capability_group="high_end_for_japanese_2023"))

    def test_app_operation_data_false_does_not_deny_generic_query(self) -> None:
        self.assert_rejected(lambda c: c["capability_detection_r4"].update(app_operation_data_flag_is_generic_query_gate=True))

    def test_self_clean_flag_does_not_establish_remote_control(self) -> None:
        self.assert_rejected(lambda c: c["capability_detection_r4"]["optional_feature_gates"].update(self_clean="supported remote start"))

    def test_unknown_model_policy_is_conservative(self) -> None:
        self.assert_rejected(lambda c: c["capability_detection_r4"].update(unknown_model_policy="fallback and enable all"))

    def test_firmware_metadata_is_not_a_capability_map(self) -> None:
        self.assert_rejected(lambda c: c["capability_detection_r4"].update(firmware_metadata_role="feature capability map"))

    def test_hvac_mode_support_remains_per_model_pending(self) -> None:
        self.assert_rejected(lambda c: c["capability_detection_r4"].update(per_model_hvac_mode_discovery="CONFIRMED_LOCAL"))

    def test_timestamp_required_and_nonzero(self) -> None:
        for key in ("required", "non_zero"):
            with self.subTest(key=key):
                self.assert_rejected(lambda c, key=key: c["time_architecture_r5"]["wf_rac_timestamp"].update({key: False}))

    def test_timestamp_security_roles_remain_unestablished(self) -> None:
        self.assert_rejected(lambda c: c["time_architecture_r5"]["wf_rac_timestamp"].update(replay_protection_role_established=True))

    def test_timezone_is_not_epoch_adjustment(self) -> None:
        self.assert_rejected(lambda c: c["time_architecture_r5"]["timezone"].update(separate_from_unix_epoch=False))

    def test_expires_does_not_gain_invented_ttl(self) -> None:
        self.assert_rejected(lambda c: c["time_architecture_r5"]["expires"].update(duration=86400))

    def test_daily_control_does_not_require_ntp(self) -> None:
        self.assert_rejected(lambda c: c["time_architecture_r5"]["future_time_provider"].update(daily_control_requires_ntp=True))

    def test_time_provider_boundary_and_quality_states_are_implemented(self) -> None:
        provider = self.canonical["time_architecture_r5"]["future_time_provider"]
        self.assertIs(provider["portable_boundary_implemented"], True)
        self.assertEqual(provider["quality_states"], ["unavailable", "estimated", "trusted"])
        self.assertIs(provider["esp_idf_system_time_adapter_implemented"], True)
        self.assertIs(provider["explicit_seed_adopt_implemented"], True)

    def test_time_acquisition_persistence_recovery_and_sntp_remain_pending(self) -> None:
        mutations = (
            lambda c: c["time_architecture_r5"]["future_time_provider"].update(portable_boundary_implemented=False),
            lambda c: c["time_architecture_r5"]["future_time_provider"].update(quality_states=["PENDING"]),
            lambda c: c["time_architecture_r5"]["future_time_provider"].update(esp_idf_system_time_adapter_implemented=False),
            lambda c: c["time_architecture_r5"]["future_time_provider"].update(explicit_seed_adopt_implemented=False),
            lambda c: c["time_architecture_r5"]["future_time_provider"].update(time_acquisition_implemented=True),
            lambda c: c["time_architecture_r5"]["future_time_provider"].update(persistence_implemented=True),
            lambda c: c["time_architecture_r5"]["future_time_provider"].update(power_loss_recovery_implemented=True),
            lambda c: c["time_architecture_r5"]["future_time_provider"].update(sntp_implemented=True),
            lambda c: c["time_architecture_r5"].update(runtime_implemented=True),
        )
        for index, mutation in enumerate(mutations):
            with self.subTest(index=index):
                self.assert_rejected(mutation)

    def test_checkpoint_is_not_trusted_current_time(self) -> None:
        self.assert_rejected(lambda c: c["time_architecture_r5"]["persisted_checkpoint"].update(is_trusted_current_time=True))

    def test_fixed_timestamp_fallback_is_forbidden(self) -> None:
        self.assert_rejected(lambda c: c["time_architecture_r5"]["wf_rac_timestamp"].update(fixed_timestamp_fallback_allowed=True))

    def test_power_loss_cannot_reconstruct_exact_wall_time(self) -> None:
        self.assert_rejected(lambda c: c["time_architecture_r5"]["power_loss"].update(exact_elapsed_wall_time_after_power_loss_without_external_source="known"))

    def test_timestamp_requires_64_bit_capability(self) -> None:
        self.assert_rejected(lambda c: c["time_architecture_r5"]["wf_rac_timestamp"].update(firmware_parse_64_bit=False))

    def test_esp_idf_is_not_wf_rac_authority(self) -> None:
        self.assert_rejected(lambda c: c["references"][-1].update(authority_boundary="WF-RAC protocol authority"))

    def test_network_ambient_uses_lookup_not_cns_formula(self) -> None:
        self.assert_rejected(lambda c: c["temperature_semantics_r6"]["network_ambient_0x80"].update(decoder="direct CNS linear formula"))

    def test_controller_room_temperature_is_not_indoor_ambient_alias(self) -> None:
        self.assert_rejected(lambda c: c["temperature_semantics_r6"]["basic_concepts"]["controller_room_temperature"].update(is_canonical_0x80_indoor_alias=True))

    def test_temperature_lookup_shape_remains_nonlinear(self) -> None:
        self.assert_rejected(lambda c: c["temperature_semantics_r6"]["network_ambient_0x80"].update(fixed_temperature_increment_per_raw_code=True))

    def test_coil_diagnostics_preserve_raw_and_bounded_calibration(self) -> None:
        self.assert_rejected(lambda c: c["temperature_semantics_r6"]["diagnostic_channels"]["0x81"].update(raw_preservable=False))
        self.assert_rejected(lambda c: c["temperature_semantics_r6"]["diagnostic_channels"]["0x87"].update(precision_calibration_guaranteed=True))

    def test_coil_curve_cannot_be_reused_for_outdoor_sensor(self) -> None:
        self.assert_rejected(lambda c: c["temperature_semantics_r6"]["diagnostic_channels"]["0x82"].update(reuse_indoor_coil_curve=True))

    def test_b1_remains_raw_only(self) -> None:
        self.assert_rejected(lambda c: c["temperature_semantics_r6"]["diagnostic_channels"]["0xB1"].update(celsius_conversion_established=True))

    def test_discharge_low_end_semantics_remain_unresolved(self) -> None:
        self.assert_rejected(lambda c: c["temperature_semantics_r6"]["diagnostic_channels"]["0x85"].update(low_raw_semantics_universally_established=True))

    def test_outlet_semantic_is_not_mode_independent(self) -> None:
        self.assert_rejected(lambda c: c["temperature_semantics_r6"]["diagnostic_channels"]["0x87"].update(mode_independent_outlet_semantic=True))

    def test_transport_detection_is_https_first(self) -> None:
        self.assert_rejected(lambda c: c["transport_security_r7"]["initial_detection"].update(unknown_device_order=["HTTP", "HTTPS"]))

    def test_https_success_stays_sticky(self) -> None:
        self.assert_rejected(lambda c: c["transport_security_r7"]["https_sticky"].update(automatic_silent_downgrade_to_http=True))

    def test_application_refusal_cannot_trigger_http_fallback(self) -> None:
        self.assert_rejected(lambda c: c["transport_security_r7"]["https_sticky"].update(application_refusal_triggers_downgrade=True))

    def test_tls_profile_is_not_universal_minimum(self) -> None:
        self.assert_rejected(lambda c: c["transport_security_r7"]["compatibility_profile"].update(universal_firmware_minimum=True))

    def test_tls_cipher_requirements_remain_unknown(self) -> None:
        self.assert_rejected(lambda c: c["transport_security_r7"]["legacy_tls"].update(exact_cipher_suite_requirements="KNOWN"))

    def test_unverified_https_is_not_authenticated(self) -> None:
        self.assert_rejected(lambda c: c["transport_security_r7"]["certificate"].update(unverified_https_classification="verified_secure_transport"))

    def test_http_is_not_confidential(self) -> None:
        self.assert_rejected(lambda c: c["transport_security_r7"]["http_classification"].update(confidentiality="confidential"))

    def test_tls_identity_does_not_require_ntp(self) -> None:
        self.assert_rejected(lambda c: c["transport_security_r7"]["r5_time_boundary"].update(daily_control_requires_ntp=True))

    def test_operator_id_is_not_tls_authentication(self) -> None:
        self.assert_rejected(lambda c: c["transport_security_r7"]["authentication_boundaries"].update(operator_id_is_transport_authentication=True))


if __name__ == "__main__":
    unittest.main()
