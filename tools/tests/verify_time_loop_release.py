from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_native_heal_api_release.py')))
rows=p['ins']('?Absorb@spell_ultraxion_time_loop_AuraScript@');op=lambda m,s:any(x.mnemonic==m and x.op_str==s for x in rows)
assert op('mov','rcx, qword ptr [rbx + 0x10]')and op('cmp','rcx, rax')and any(x.mnemonic=='jb'for x in rows),'Actual incoming record must compare against full64health without signed32subtraction'
assert p['sites'](rows,'?CastCustomSpell@Unit@@')and 105992 in p['values'](rows),'Original actual heal cast retained'
assert op('mov','qword ptr [rsp + 0x40], rax')and op('mov','qword ptr [rdx + 8], rax')and 0x40 in p['values'](rows),'Full max health must enter the exact pair without float/int32 payload'
print('PASS linked TimeLoop unsigned lethal comparison,exact64max-health payload and retained105992cast/one-use cleanup. Complete native-heal/prior chain retained;actual client encounter remains pending.')
