"""Check the deployed scroll report-use and native goober spell path."""

import bisect
from pathlib import Path
import runpy
import struct


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
symbol, symbols, resolve, calls = (native[key] for key in ("symbol", "symbols", "resolve", "calls"))

report = "?GossipHello@go_wandering_isle_defaced_scroll@@"
guard = "?GossipUse@go_wandering_isle_defaced_scroll@@"
cast = "?CastSpell@GameObject@@"

for caller, target in (
    (report, "?Use@GameObject@@QEAAXPEAVUnit@@@Z"),
    (guard, "?GetQuestStatus@Player@@"),
    (cast, "?CastSpell@Unit@@"),
):
    addresses = {resolve(address) for name, address in symbols.items() if name.startswith(target)}
    assert addresses & calls(caller), f"Missing linked {caller} -> {target}"

address = symbol(cast)
rva = address - native["base"]
begin, end, _ = native["functions"][bisect.bisect_right(native["starts"], rva) - 1]
assert begin <= rva < end
offset = native["offset"](address)
body = native["binary"][offset:offset + end - rva]
assert struct.pack("<I", 104127) in body and struct.pack("<I", 29778) in body, "Missing the scoped scroll cast"
print("PASS: linked defaced scroll forwards report-use and casts its native spell through the using player")
