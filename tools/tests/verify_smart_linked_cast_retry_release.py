"""Verify native call paths in the linked x64 PE, resolving incremental thunks."""
import argparse
import bisect
import hashlib
import json
from pathlib import Path
import re
import struct

parser = argparse.ArgumentParser()
parser.add_argument("release", type=Path)
args = parser.parse_args()
exe = args.release / "worldserver.exe"
binary = exe.read_bytes()
u16 = lambda offset: struct.unpack_from("<H", binary, offset)[0]
u32 = lambda offset: struct.unpack_from("<I", binary, offset)[0]
pe = u32(0x3C)
optional = pe + 24
base = struct.unpack_from("<Q", binary, optional + 24)[0]
sections = []
for i in range(u16(pe + 6)):
    at = optional + u16(pe + 20) + 40 * i
    sections.append((u32(at + 12), max(u32(at + 8), u32(at + 16)), u32(at + 20)))


def offset(address):
    rva = address - base
    for start, size, raw in sections:
        if start <= rva < start + size:
            return raw + rva - start
    raise ValueError(f"Address outside PE: {address:x}")


def resolve(address):
    for _ in range(16):
        at = offset(address)
        if binary[at] != 0xE9:
            return address
        address += 5 + struct.unpack_from("<i", binary, at + 1)[0]
    raise ValueError("Incremental-link thunk cycle")


symbols = {}
with (args.release / "worldserver.map").open(errors="replace") as stream:
    for line in stream:
        match = re.match(r"\s+\w+:\w+\s+(\?\S+)\s+([0-9a-fA-F]{16})\s+f", line)
        if match:
            symbols[match[1]] = int(match[2], 16)


def symbol(prefix):
    matches = [address for name, address in symbols.items() if name.startswith(prefix)]
    if len(matches) != 1:
        raise ValueError(f"Expected one symbol for {prefix}, got {len(matches)}")
    return resolve(matches[0])


pdata = offset(base + u32(optional + 112 + 3 * 8))
count = u32(optional + 112 + 3 * 8 + 4) // 12
functions = [struct.unpack_from("<III", binary, pdata + 12 * i) for i in range(count)]
starts = [row[0] for row in functions]


def root(begin, unwind):
    for _ in range(16):
        at = offset(base + unwind)
        if not binary[at] & 0x20:
            return begin
        chain = at + 4 + 4 * ((binary[at + 2] + 1) // 2)
        begin, _, unwind = struct.unpack_from("<III", binary, chain)
    raise ValueError("Unwind chain cycle")


def calls(prefix):
    address = symbol(prefix)
    index = bisect.bisect_right(starts, address - base) - 1
    begin, end, unwind = functions[index]
    if not begin <= address - base < end:
        # x64 leaf tail-call wrappers need no unwind entry. Bound these tiny
        # wrappers by the next public symbol and reject larger unknown bodies.
        following = min(value for value in symbols.values() if value > address)
        if following - address > 64:
            raise ValueError(f"Missing runtime function bounds: {prefix}")
        end = following - base
    else:
        origin = root(begin, unwind)
        for tail_begin, tail_end, tail_unwind in functions[index + 1:]:
            if tail_begin != end or root(tail_begin, tail_unwind) != origin:
                break
            end = tail_end
    at = offset(address)
    result = set()
    for i in range(base + end - address - 4):
        if binary[at + i] in (0xE8, 0xE9):
            try:
                result.add(resolve(address + i + 5 + struct.unpack_from("<i", binary, at + i + 1)[0]))
            except (ValueError, struct.error, IndexError):
                pass
    return result


checks = [
    ("?ProcessAction@SmartScript@@", "?BeginLinkedCastAttempt@SmartScript@@"),
    ("?ProcessAction@SmartScript@@", "?FinishLinkedCastAttempt@SmartScript@@"),
    ("?ProcessAction@SmartScript@@", "?QueueLinkedCastRetry@SmartScript@@"),
    ("?OnUpdate@SmartScript@@", "?UpdateLinkedCastRetries@SmartScript@@"),
    ("?OnCorpseUpdate@SmartScript@@", "?UpdateLinkedCastRetries@SmartScript@@"),
    ("?UpdateCorpseAI@SmartAI@@", "?OnCorpseUpdate@SmartScript@@"),
    ("?UpdateLinkedCastRetries@SmartScript@@", "?ProcessEvent@SmartScript@@"),
    ("?UpdateLinkedCastRetries@SmartScript@@", "?GetUnit@ObjectAccessor@@"),
    ("?UpdateLinkedCastRetries@SmartScript@@", "?GetGameObject@ObjectAccessor@@"),
    ("?OnReset@SmartScript@@", "?ResetLinkedCastRetries@SmartScript@@"),
    ("?OnInitialize@SmartScript@@", "?ResetLinkedCastRetries@SmartScript@@"),
]
for caller, target in checks:
    if symbol(caller) != symbol(target) and symbol(target) not in calls(caller):
        raise AssertionError(f"Linked executable lacks {caller} -> {target}")
for message in (b"linked cast %u expired without success.", b"linked cast %u failed after ten retries."):
    assert message in binary

from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM, CS_OP_REG
md=Cs(CS_ARCH_X86, CS_MODE_64);md.detail=True

def instructions(prefix):
    address=symbol(prefix)
    index=bisect.bisect_right(starts,address-base)-1
    begin,end,unwind=functions[index]
    if not begin<=address-base<end:
        end=min(value for value in symbols.values() if value>address)-base
        assert base+end-address<=64
    else:
        origin=root(begin,unwind)
        for following,following_end,following_unwind in functions[index+1:]:
            if following!=end or root(following,following_unwind)!=origin:break
            end=following_end
    return list(md.disasm(binary[offset(address):offset(address)+base+end-address],address))

# Resolve the actual virtual slot from the linked SmartAI vtable, including
# incremental thunks. Then find its virtual dispatch in Creature::Update.
maptext=(args.release/'worldserver.map').read_text(errors='replace')
vtable=re.findall(r"\s+\w+:\w+\s+(\?\?_7SmartAI@@6B@)\s+([0-9a-fA-F]{16})",maptext)
assert len(vtable)==1
vaddress=int(vtable[0][1],16)
slots=struct.unpack_from('<'+str(160)+'Q',binary,offset(vaddress))
slot=[i for i,value in enumerate(slots) if value==symbol('?UpdateCorpseAI@SmartAI@@') or value in symbols.values() and resolve(value)==symbol('?UpdateCorpseAI@SmartAI@@')]
assert len(slot)==1
creature=instructions('?Update@Creature@@')
assert any(x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==slot[0]*8 for x in creature)
assert symbol('?OnUpdate@SmartScript@@') not in calls('?OnCorpseUpdate@SmartScript@@')
loader=instructions('?IsEventValid@SmartAIMgr@@')
site=[i for i,x in enumerate(loader) if x.mnemonic=='bt' and x.operands[1].type==CS_OP_IMM and x.operands[1].imm==9]
assert len(site)==1
window=loader[site[0]:site[0]+22]
for displacement,value in ((0x30,61),(8,0),(0x70,1)):
    assert any(x.mnemonic=='cmp' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==displacement and x.operands[1].type==CS_OP_IMM and x.operands[1].imm==value for x in window),(displacement,value,[(x.mnemonic,x.op_str) for x in window])
assert any(x.mnemonic=='test' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0x3c and x.operands[1].type==CS_OP_IMM and x.operands[1].imm==1 for x in window)
assert any(x.mnemonic=='cmp' and x.operands[1].type==CS_OP_IMM and x.operands[1].imm==11 for x in window)
flag=loader[site[0]].operands[0]
assert flag.type==CS_OP_REG
assert any(x.mnemonic=='mov' and x.operands[0].type==CS_OP_REG and x.operands[0].reg==flag.reg and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.disp==0x5c for x in loader[site[0]-8:site[0]])
assert any(x.mnemonic=='cmp' and x.operands[0].type==CS_OP_REG and x.operands[0].reg==flag.reg and x.operands[1].type==CS_OP_IMM and x.operands[1].imm==512 for x in window)
print('PASS linked PE:11 retry/cleanup/GUID calls, actual SmartAI corpse vtable/Creature dispatch, strict512 LINK/SELF/once/creature loader, finite diagnostics')
print(json.dumps({'exe_sha256':hashlib.sha256(binary).hexdigest(),'verified_calls':len(checks),'corpse_vtable_slot':slot[0]}))
