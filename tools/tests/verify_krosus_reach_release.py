"""Check the selected Broken Shore AI's model-derived reach in the actual PE."""
from pathlib import Path
import re, runpy, struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n,ins=p['native'],p['instructions']
reset='?Reset@sceneTrigger_part8AI@sceneTrigger_part8@@'
factory='?GetAI@sceneTrigger_part8@@'
body=ins(reset)
values={o.imm for x in body for o in x.operands if o.type==CS_OP_IMM}
assert {1460,90544,71701,6,0x69} <= values, 'Missing exact actor scope, finite six-coordinate guard or combat reach field'
mapping=(n['args'].release/'worldserver.map').read_text(errors='replace')
match=re.search(re.escape('??_7sceneTrigger_part8AI@sceneTrigger_part8@@6B@')+r'\s+([0-9a-fA-F]{16})',mapping)
assert match
table=int(match[1],16)
refs={x.address+x.size+o.mem.disp for x in ins(factory) for o in x.operands if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP}
assert table in refs and n['symbol'](reset) in struct.unpack_from('<100Q',n['binary'],n['offset'](table)), 'Factory does not select repaired Reset'
float_setters={n['resolve'](v) for k,v in n['symbols'].items() if k.startswith('?SetFloatValue@Object@@')}
assert float_setters & n['calls'](reset), 'No actual public reach field write'
assert sum(x.mnemonic=='maxss' for x in body)>=3, 'Missing both horizontal extents and native reach preservation'
assert sum(x.mnemonic=='mulss' for x in body)>=3, 'Missing display/model/object scale consumers'
# The model pointer is advanced to native CreatureModelData::GeoBox (0x20),
# then min/max X and Y (0/0xc,4/0x10) feed the horizontal envelope.
assert any(x.mnemonic=='lea' and any(o.type==CS_OP_MEM and o.mem.disp==0x20 for o in x.operands) for x in body)
print('PASS selected Broken Shore factory/Reset: exact map/entry/display, native scaled horizontal box, finite guards and public reach write. Client reach/combat traversal remains pending.')
