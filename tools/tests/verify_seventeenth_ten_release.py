"""Linked production reset writes; native/client coverage stays separately scoped."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_sixteenth_ten_release.py')));ins=p['ins']
checks={
'boss_karsh_steelbenderAI':[('dword ptr [rbx + 0x64]','0x7d0')],
'boss_ivanyrAI':[('dword ptr [rbx + 0x174]','0x3e8')],
'boss_millificent_manastormAI':[('dword ptr [rbx + 0x17c]','0x7d0')],
'npc_elder_murk_eyeAI':[('dword ptr [rbx + 0x60]','0x7d0'),('dword ptr [rbx + 0x64]','0x1388')],
'npc_en_nightmother':[('dword ptr [rbx + 0x58]','0')],
'npc_argent_championAI':[('dword ptr [rcx + 0x40]','0x2710')],
'npc_blackholeAI':[('word ptr [rcx + 0x48]','0')],
'npc_daakara_vortexAI':[('byte ptr [rcx + 0x38]','0'),('dword ptr [rcx + 0x3c]','0x7d0')],
'npc_empyreal_focusAI':[('byte ptr [rcx + 0x40]','1')],
'npc_pavalak_amber_sapper':[('dword ptr [rbx + 0x38]','0')]}
for cls,ops in checks.items():
 rows=ins('?Reset@'+cls+'@')
 for dst,value in ops:assert any(x.mnemonic=='mov' and x.op_str==dst+', '+value for x in rows),(cls,dst,value)
print('PASS actual Release seventeenth ten:12 missing reset state writes in10 independent controllers;prior167 selected paths retained. Source/native selected-state tests do not certify full encounter/client traversal.')
