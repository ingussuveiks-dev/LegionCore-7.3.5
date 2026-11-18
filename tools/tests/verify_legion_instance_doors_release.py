"""Check table bytes actually passed to LoadDoorData in the final linked PE."""
import json
from pathlib import Path
import runpy
import struct
import tempfile
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RDX, X86_REG_RIP
from prepare_legion_instance_doors import prepare
from sharasdal_erunak_entry_pe import sites

n = runpy.run_path(str(Path(__file__).with_name("verify_smart_linked_cast_retry_release.py")))
with tempfile.TemporaryDirectory(prefix="legion-door-arrays-") as directory:
    output = Path(directory)
    prepare(output)
    scopes = json.loads((output / "door-arrays.json").read_text())

tables = 0
for scope in scopes:
    if not scope["table"]:
        continue
    suffix = f'@{scope["implementation"]}@{scope["script"]}@@'
    bodies = []
    for name in n["symbols"]:
        if name.startswith("?Initialize" + suffix) or name.startswith("??0" + suffix[1:]):
            body = n["instructions"](name)
            for site in sites(n, body, "?LoadDoorData@InstanceScript@@"):
                pointers = [x for x in body[max(0, site-6):site]
                            if x.mnemonic == "lea" and x.operands[0].type == CS_OP_REG
                            and x.operands[0].reg == X86_REG_RDX and x.operands[1].type == CS_OP_MEM
                            and x.operands[1].mem.base == X86_REG_RIP]
                assert len(pointers) == 1, (scope["map"], "Ambiguous native table argument")
                instruction = pointers[0]
                address = instruction.address + instruction.size + instruction.operands[1].mem.disp
                expected = b"".join(struct.pack("<4I", *row) for row in scope["rows"])
                offset = n["offset"](address)
                assert scope["rows"][-1] == [0, 0, 0, 0]
                assert n["binary"][offset:offset+len(expected)] == expected, scope["map"]
                bodies.append(address)
    assert len(bodies) == 1, (scope["map"], "Missing/ambiguous linked door loader")
    tables += 1

cathedral = n["instructions"]("?OnGameObjectCreate@instance_cathedral_of_eternal_night_InstanceMapScript@instance_cathedral_of_eternal_night@@")
door_call, = sites(n, cathedral, "?AddDoor@InstanceScript@@")
# MSVC folds adjacent case labels into a subtract/compare decision tree.
# Follow that actual tree for each gate, rather than demanding four literals.
indices = {x.address: i for i, x in enumerate(cathedral)}
start = next(i for i, x in enumerate(cathedral) if x.mnemonic == "cmp" and x.op_str.startswith("eax,"))
for entry in (268680, 268709, 268710, 269221):
    eax, index, equal, above = entry, start, False, False
    for _ in range(60):
        if index == door_call:
            break
        x = cathedral[index]
        if x.mnemonic in ("sub", "cmp") and x.op_str.startswith("eax,"):
            assert x.operands[1].type == CS_OP_IMM
            value = x.operands[1].imm
            equal, above = eax == value, eax > value
            if x.mnemonic == "sub":
                eax = (eax - value) & 0xffffffff
        elif x.mnemonic in ("je", "jne", "ja", "jmp"):
            if {"je": equal, "jne": not equal, "ja": above, "jmp": True}[x.mnemonic]:
                index = indices[x.operands[0].imm]
                continue
        else:
            assert x.mnemonic == "mov", (entry, x.mnemonic, x.op_str)
        index += 1
    assert index == door_call, (entry, "Gate did not dispatch to AddDoor")
assert tables == 12
print("PASS final linked PE: all12 native dungeon door tables, exact rows and zero terminators are passed to LoadDoorData; Cathedral registers all4 gate entries. Client traversal remains separate.")
