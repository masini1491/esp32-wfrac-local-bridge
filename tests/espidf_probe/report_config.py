"""Report only A5's six allowlisted generated settings; never print sdkconfig."""
import argparse
import hashlib
import json
from pathlib import Path

KEYS = (
    "CONFIG_LWIP_TCPIP_CORE_LOCKING",
    "CONFIG_LWIP_CHECK_THREAD_SAFETY",
    "CONFIG_LWIP_TCPIP_RECVMBOX_SIZE",
    "CONFIG_LWIP_DNS_SUPPORT_MDNS_QUERIES",
    "CONFIG_LWIP_DNS_MAX_SERVERS",
    "CONFIG_LWIP_SO_LINGER",
)


def settings(path):
    result = dict.fromkeys(KEYS, "ABSENT")
    seen = set()
    for line in path.read_text(encoding="utf-8").splitlines():
        for key in KEYS:
            if line.startswith(key + "=") or line == "# " + key + " is not set":
                if key in seen:
                    raise SystemExit("Duplicate allowlisted setting: " + key)
                seen.add(key)
                value = "DISABLED" if line.startswith("#") else line.split("=", 1)[1]
                if value not in ("y", "n", "DISABLED") and not value.isdecimal():
                    raise SystemExit("Unexpected allowlisted value for " + key)
                result[key] = value
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("sdkconfig", type=Path)
    parser.add_argument("--cell", required=True)
    parser.add_argument("--compare", type=Path)
    args = parser.parse_args()
    result = settings(args.sdkconfig)
    if args.compare is not None and result != settings(args.compare):
        raise SystemExit("Probe configuration differs from generated firmware settings")
    payload = json.dumps(result, sort_keys=True)
    print("A5_CONFIG " + json.dumps({"cell": args.cell, "generated_path": str(args.sdkconfig),
          "settings": result, "allowlist_sha256": hashlib.sha256(payload.encode()).hexdigest()}, sort_keys=True))


if __name__ == "__main__":
    main()
