"""Verify handshake diagnostic call sites in the linked PE, not an old library."""
from pathlib import Path
import runpy
import sys

reader = Path(__file__).with_name("verify_scenario_lfg_release.py")
sys.argv[0] = str(reader)
native = runpy.run_path(str(reader))
target = native["symbol"]("?LogInitializationState@WorldSocket@@")
for method in ("Start", "InitializeHandler", "Update", "HandleAuthContinuedSession", "HandleEnableEncryptionAck"):
    assert target in native["calls"](f"?{method}@WorldSocket@@"), f"Missing deployed diagnostic at {method}"
for message in (b"World handshake %s:", b"client CONNECT_TO_FAILED serial %u connection %u", b"instance serial %u target %s:%u"):
    assert message in native["binary"], f"Missing linked diagnostic: {message!r}"
print("PASS: actual Release contains five handshake diagnostic call sites and client failure/target diagnostics")
