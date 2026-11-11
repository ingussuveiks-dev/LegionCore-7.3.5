from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_native_heal_api_release.py')))
rows=p['ins']('?Absorb@spell_ultraxion_time_loop_AuraScript@');op=lambda m,s:any(x.mnemonic==m and x.op_str==s for x in rows)
assert op('mov','ecx, dword ptr [rbx + 0x10]')and op('cmp','rcx, rax')and any(x.mnemonic=='jb'for x in rows),'Actual32incoming must compare against full64health without signed32subtraction'
assert p['sites'](rows,'?CastCustomSpell@Unit@@')and 105992 in p['values'](rows),'Original actual heal cast retained'
print('PASS linked TimeLoop unsigned lethal comparison and retained105992cast;wide full-heal remains OPEN. Complete native-heal/prior chain retained.')
