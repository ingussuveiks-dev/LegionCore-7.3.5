from pathlib import Path
import runpy
from native_heal_pe import verify
p=runpy.run_path(str(Path(__file__).with_name('verify_meljarak_mend_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values']
root=n
while 'binary'not in root:root=root['n']
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
decoder=Cs(CS_ARCH_X86,CS_MODE_64);decoder.detail=True
verify(ins,sites,values,decoder,root)
postheal=runpy.run_path(str(Path(__file__).with_name('verify_creature_heal_applied_release.py')))
print('PASS complete native heal/prior Mending/target-profile/Cordana/46group PE chain plus preserved post-heal lock/selected layout slots;full81slot comparison requires standalone --previous check. Native and client scopes remain separate')
