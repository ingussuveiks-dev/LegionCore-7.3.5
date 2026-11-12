"""Derive native Player member expectations from the linked teleport accessor.

Only explicitly named legacy teleport fields are remapped. The inspected
instructions are never rewritten; callers still compare raw physical operands.
"""
from pathlib import Path
import functools,re,struct,sys
from capstone import Cs,CS_ARCH_X86,CS_MODE_64,CS_OP_MEM
from capstone.x86_const import X86_REG_RCX

@functools.lru_cache(None)
def teleport_offsets(release):
 root=Path(release);binary=(root/'worldserver.exe').read_bytes();u16=lambda a:struct.unpack_from('<H',binary,a)[0];u32=lambda a:struct.unpack_from('<I',binary,a)[0];pe=u32(0x3c);optional=pe+24;base=struct.unpack_from('<Q',binary,optional+24)[0];sections=[]
 for i in range(u16(pe+6)):
  at=optional+u16(pe+20)+40*i;sections.append((u32(at+12),max(u32(at+8),u32(at+16)),u32(at+20)))
 def offset(address):
  for start,size,raw in sections:
   if start<=address-base<start+size:return raw+address-base-start
  raise AssertionError('Native accessor outside PE')
 address=None
 with(root/'worldserver.map').open(errors='replace')as stream:
  for line in stream:
   match=re.match(r'\s+\w+:\w+\s+(\?HasPendingTeleport@Player@@\S*)\s+([0-9a-fA-F]{16})\s+f',line)
   if match:assert address is None;address=int(match[2],16)
 assert address is not None,'Linked native HasPendingTeleport accessor missing'
 for _ in range(16):
  at=offset(address)
  if binary[at]!=0xe9:break
  address+=5+struct.unpack_from('<i',binary,at+1)[0]
 else:raise AssertionError('Native accessor thunk cycle')
 decoder=Cs(CS_ARCH_X86,CS_MODE_64);decoder.detail=True;fields=[];finished=False
 for instruction in decoder.disasm(binary[at:at+96],address):
  fields.extend(o.mem.disp for o in instruction.operands if o.type==CS_OP_MEM and o.mem.base==X86_REG_RCX and o.size==1)
  if instruction.mnemonic=='ret':finished=True;break
 assert finished and len(fields)==3 and len(set(fields))==3,('Native three-flag accessor contract changed',fields)
 assert fields[1]==fields[0]+1 and fields[2]==fields[0]+9,('Native near/far/delayed field relationships changed',fields)
 return dict(zip((0x6848,0x6849,0x6851),fields))

def player_offset(legacy,release=None):
 return teleport_offsets(str(release or sys.argv[1]))[legacy]

def player_operand(expected,release=None):
 fields=teleport_offsets(str(release or sys.argv[1]))
 return re.sub(r'0x(?:6848|6849|6851)\b',lambda m:hex(fields[int(m[0],16)]),expected)

@functools.lru_cache(None)
def unit_queue_offset():
 # This canonical Unit accessor is retained as an outlined wrapper. Read its
 # real member address and cross-check the independently derived Player delta.
 from five_ten_20261111_pe import ins
 from capstone import CS_OP_IMM
 offsets=[]
 for x in ins('?AddDelayedEvent@Unit@@'):
  if len(x.operands)!=2 or x.reg_name(x.operands[0].reg)!='rcx':continue
  other=x.operands[1]
  if x.mnemonic=='add'and other.type==CS_OP_IMM:offsets.append(other.imm)
  if x.mnemonic=='lea'and other.type==CS_OP_MEM and other.mem.base==X86_REG_RCX:offsets.append(other.mem.disp)
 assert len(offsets)==1 and offsets[0]==0x36a0+player_offset(0x6848)-0x6848,('Native Unit queue/Player layout relationship changed',offsets)
 return offsets[0]

def unit_queue_operand(expected):
 return re.sub(r'0x36a0\b',hex(unit_queue_offset()),expected)


def inherited_unit_delta():
 return unit_queue_offset()-0x36a0


def creature_loot_operand(expected):
 return expected.replace('0x39b8',hex(14776+inherited_unit_delta()))


# Reviewed physical members of Unit-derived Creature/Pet/Player objects in the
# retained operand certificates. The only inherited layout delta is the two
# history arrays, cross-checked against real Unit and Player accessors and the
# actual Creature constructor/loot-counter native regression.
INHERITED_MEMBERS={0x39c5,0x39c8,0x3a80,0x3a88,0x3a9d,0x3ac8,0x3acc,0x3ad0,0x3ad4,0x3b14,0x3b18,0x3b1c,0x3b20,0x3b64,0x3d18,0x4ff0,0x6830,0x6834,0x6838,0x683c,0x6840,0x6ba8,0x6bb0,0x6bb4,0x6bb8,0x6bc0,0x6bc8,0x6bcc,0x6bd0,0x6bd4,0x6bd8,0x6bdc}
def inherited_operand(expected):
 def replace(match):
  value=int(match[0],16)
  return hex(value+inherited_unit_delta())if value in INHERITED_MEMBERS else match[0]
 return re.sub(r'0x[0-9a-f]+\b',replace,expected)


def inherited_member_offset(legacy):
 assert legacy in INHERITED_MEMBERS
 return legacy+inherited_unit_delta()
