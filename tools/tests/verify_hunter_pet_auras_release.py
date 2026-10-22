"""Check the actual linked CastPetAuras implementation, resolving PE thunks."""
from pathlib import Path
import bisect
import re
import runpy
import struct

native=runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
address=native["symbol"]("?CastPetAuras@Pet@@")
index=bisect.bisect_right(native["starts"],address-native["base"])-1
begin,end,_=native["functions"][index]
assert begin<=address-native["base"]<end
at=native["offset"](address)
code=native["binary"][at:at+native["base"]+end-address]
# Current native TempSummon layout: m_petType is byte offset 0x3B75.
# MSVC's actual dispatch compares HUNTER_PET (1) before loading bucket -1.
# This intentionally fails if the layout/code changes and needs reinspection.
gate=re.search(rb"\x41\x80\xbf\x75\x3b\x00\x00\x01\x0f\x85....\xe8....\xba\xff\xff\xff\xff",code,re.DOTALL)
assert gate,"Release still uses creation-spell admission instead of the native hunter pet type"
call=gate.end()+3  # mov rcx,rax (3 bytes), then call GetPetAura.
assert code[call]==0xE8
target=native["resolve"](address+call+5+struct.unpack_from("<i",code,call+1)[0])
assert target==native["symbol"]("?GetPetAura@SpellMgr@@"),"Hunter pet type guard does not reach the shared aura bucket"

def body(prefix):
    address=native["symbol"](prefix)
    index=bisect.bisect_right(native["starts"],address-native["base"])-1
    begin,end,_=native["functions"][index]
    assert begin<=address-native["base"]<end
    at=native["offset"](address)
    return address,native["binary"][at:at+native["base"]+end-address]

_,created=body("?InitPetCreateSpells@Pet@@")
assert re.search(rb"\x80\xbb\x75\x3b\x00\x00\x01\x75.\x83\xbb\x18\x0a\x00\x00\x00\x75.\xb2\x01",created,re.DOTALL),"New hunter pets still remove active-pet auras"
assert address in native["calls"]("?InitPetCreateSpells@Pet@@")
loaders=[name for name in native["symbols"] if name.startswith("??R<lambda_") and "LoadPetFromDB@Pet@@" in name]
assert len(loaders)==1
loaded_address,loaded=body(loaders[0])
gate=re.search(rb"\x80\xb9\x75\x3b\x00\x00\x01\x75.\x83\xb9\x18\x0a\x00\x00\x00\x0f\x94\xc2\x45\x33\xc0\xe8",loaded,re.DOTALL)
assert gate,"Loaded hunter pet aura admission still depends on the current lookup flag"
call=gate.end()-1
target=native["resolve"](loaded_address+call+5+struct.unpack_from("<i",loaded,call+1)[0])
assert target==address,"Live loaded hunter pet does not apply its active-pet auras"
print("PASS: linked hunter pet type dispatch, live fresh-pet aura application and current/non-current load lifecycle")
