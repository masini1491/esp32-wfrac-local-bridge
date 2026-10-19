"""D2-P1-T post-link evidence; inspect only, NEVER execute the target ELF."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess

PINS = {".": "7101770dc6db2667b3c477cc31365dd1acd6db4e",
        "components/lwip/lwip": "fd432e4ee2cfb7f7f1c7eb7227e0173412e7b84e",
        "components/mbedtls/mbedtls": "6cc42afad309e861f4c07e6f106e2ab14a9cb8e5"}
PORT = ("mbedtls_net_send", "mbedtls_net_recv", "mbedtls_net_recv_timeout", "mbedtls_net_free")
APIS = ("mbedtls_ssl_config_defaults", "mbedtls_ssl_conf_authmode", "mbedtls_ssl_conf_ca_chain",
        "mbedtls_ssl_setup", "mbedtls_ssl_set_hostname", "mbedtls_ssl_set_bio", "mbedtls_ssl_free")
UNLINKED = ("mbedtls_net_poll", "mbedtls_net_close")


def require(value, message):
    if not value:
        raise RuntimeError(message)


def run(args, cwd=None):
    result = subprocess.run([str(a) for a in args], cwd=cwd, check=True,
                            capture_output=True, text=True, timeout=60)
    require(len(result.stdout) < 8 * 1024 * 1024, "unbounded inspection output")
    return result.stdout


def argv(entry):
    if "arguments" in entry:
        return entry["arguments"]
    args = shlex.split(entry["command"], posix=os.name != "nt")
    return [a[1:-1] if os.name == "nt" and a.startswith('"') and a.endswith('"') else a for a in args]


def object_path(entry):
    args = argv(entry)
    require(args.count("-o") == 1, "ambiguous compile object")
    return (Path(entry["directory"]) / args[args.index("-o") + 1]).resolve()


def macros(entry):
    args = argv(entry)
    filtered = []
    i = 0
    while i < len(args):
        if args[i] in ("-o", "-MF", "-MT", "-MQ"):
            i += 2
        elif args[i] in ("-c", "-MD", "-MMD", "-MP"):
            i += 1
        else:
            filtered.append(args[i]); i += 1
    return run(filtered + ["-E", "-dM"], entry["directory"])


def main(build, target):
    sdk = Path(os.environ["IDF_PATH"]).resolve()
    for subdir, pin in PINS.items():
        require(run(["git", "-C", sdk / subdir, "rev-parse", "HEAD"]).strip() == pin,
                "SDK/submodule identity mismatch: " + subdir)
    config_lines = (build / "sdkconfig").read_text().splitlines()
    require('CONFIG_IDF_TARGET="' + target + '"' in config_lines, "wrong generated target")
    require("CONFIG_LWIP_ENABLE=y" in config_lines, "actual target lacks lwIP")
    commands = json.loads((build / "compile_commands.json").read_text())
    port_source = (sdk / "components/mbedtls/port/net_sockets.c").resolve()
    upstream = (sdk / "components/mbedtls/mbedtls/library/net_sockets.c").resolve()
    port_entries = [e for e in commands if Path(e["file"]).resolve() == port_source]
    tls_entries = [e for e in commands if Path(e["file"]).name == "tls_bio_link.cpp"]
    require(len(port_entries) == 1 and len(tls_entries) == 1, "ambiguous/missing actual compile inputs")
    require(not any(Path(e["file"]).resolve() == upstream for e in commands), "upstream net source unexpectedly selected")
    # Verify actual selected port bytes against its pinned parent Git tree.
    content = port_source.read_bytes().replace(b"\r\n", b"\n")
    blob = hashlib.sha1(b"blob " + str(len(content)).encode() + b"\0" + content).hexdigest()
    expected = run(["git", "-C", sdk, "rev-parse", "HEAD:components/mbedtls/port/net_sockets.c"]).strip()
    require(blob == expected, "selected port source differs from pinned SDK")
    defs = macros(tls_entries[0]).splitlines()
    require(not any(l.startswith("#define MBEDTLS_NET_C") for l in defs), "upstream NET_C unexpectedly defined")
    config_macro = [l for l in defs if l.startswith("#define MBEDTLS_CONFIG_FILE ")]
    require(len(config_macro) == 1 and '"mbedtls/esp_config.h"' in config_macro[0], "unverified effective mbedTLS config")
    elf = build / "wfrac_a5_link_only.elf"
    symbols = run(["riscv32-esp-elf-nm", "-g", "--defined-only", elf])
    defined = {l.split()[-1] for l in symbols.splitlines() if l.split()}
    require(set((*PORT, *APIS, "wfrac_d2_tls_link_anchor", "tcpip_try_callback",
                 "dns_gethostbyname_addrtype")).issubset(defined), "required retained ELF graph missing")
    require(not set(UNLINKED).intersection(defined), "unexpected poll/close implementation; source assumption changed")
    obj_symbols = run(["riscv32-esp-elf-nm", "-g", "--defined-only", object_path(port_entries[0])])
    port_defs = {l.split()[-1] for l in obj_symbols.splitlines() if l.split()}
    require(set(PORT).issubset(port_defs) and not set(UNLINKED).intersection(port_defs), "port object does not match expected implementation")
    relocs = run(["riscv32-esp-elf-objdump", "-r", object_path(tls_entries[0])])
    for symbol in (*PORT, *APIS):
        require(any(l.split() and l.split()[-1].split("+")[0] == symbol for l in relocs.splitlines()),
                "test object lacks actual reference: " + symbol)
    require(not any(symbol in relocs for symbol in UNLINKED), "prototype-only helper acquired a link reference")
    mapping = (build / "wfrac_a5_link_only.map").read_text()
    require("Cross Reference Table" in mapping, "missing linker cross-reference evidence")
    users = {}; current = None
    for line in mapping.split("Cross Reference Table", 1)[1].splitlines():
        if line and not line[0].isspace():
            parts = line.split(None, 1); current = parts[0]; users[current] = line
        elif current:
            users[current] += "\n" + line
    for symbol in (*PORT, *APIS):
        require("tls_bio_link.cpp.obj" in users.get(symbol, ""), "map lacks fixture reference: " + symbol)
    settings = {key: next((l.split("=", 1)[1] for l in config_lines if l.startswith(key + "=")), "UNSET")
                for key in ("CONFIG_IDF_TARGET", "CONFIG_LWIP_ENABLE", "CONFIG_MBEDTLS_TLS_ENABLED",
                            "CONFIG_MBEDTLS_SSL_PROTO_TLS1_2", "CONFIG_MBEDTLS_SSL_PROTO_TLS1_3")}
    print("D2_P1_LINK " + json.dumps({"target": target, "tier": "SDK_COMPILE_ONLY", "pins": PINS,
          "config": settings, "MBEDTLS_NET_C": "UNDEFINED", "config_macro": config_macro[0],
          "port_source": str(port_source), "port_blob": blob,
          "port_object": str(object_path(port_entries[0])), "port_symbols": PORT,
          "prototype_only_not_linked": UNLINKED, "fixture_relocations_and_map": "PASS",
          "elf_sha256": hashlib.sha256(elf.read_bytes()).hexdigest()}, sort_keys=True), flush=True)
    print("D2_P1 NOT_RUN: target execution, TLS handshake, DNS/socket/network/hardware; native owner/quiescence UNPROVEN")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("--target", required=True, choices=("esp32c3", "esp32c6"))
    args = parser.parse_args()
    main(args.build.resolve(), args.target)
