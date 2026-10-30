"""Verify existing addon consumer and SmartAI cleanup in the selected Release."""
from pathlib import Path
import runpy,bisect
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
n=runpy.run_path(str(Path(__file__).with_name('verify_nexus_azuregos_receiver_release.py')))
native=n['n']
for caller,callee in (
 ('?LoadCreaturesAddon@Creature@@','?GetCreatureAddon@ObjectMgr@@'),
 ('?LoadCreaturesAddon@Creature@@','?GetCreatureTemplateAddon@ObjectMgr@@'),
 ('?LoadCreaturesAddon@Creature@@','?AddAura@Unit@@QEAAPEAVAura@@I'),
 ('?ProcessAction@SmartScript@@','?RemoveAllAuras@Unit@@'),
):assert native['symbol'](callee) in native['calls'](caller),(caller,callee)
a=native['symbol']('?LoadCreaturesAddon@Creature@@');i=bisect.bisect_right(native['starts'],a-native['base'])-1;b,e,u=native['functions'][i]
d=Cs(CS_ARCH_X86,CS_MODE_64);body=list(d.disasm(native['binary'][native['offset'](a):native['offset'](a)+native['base']+e-a],a))
assert any(x.mnemonic=='cmp' and x.op_str=='qword ptr [rcx + 0x3a88], 0' for x in body)
assert any(x.mnemonic=='lea' and x.op_str=='rdx, [rdi + 0x3a88]' for x in body)
print('PASS actual Release persistent spawn addon preference, native aura loading and SmartAI all-aura cleanup; original missingLINK2 and actual rendered visuals remain separate.')
