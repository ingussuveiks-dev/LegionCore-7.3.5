from pathlib import Path
import re,sys
from ten_ten_20261111_scope import ROOT,PLAN,require_fixed
out=Path(sys.argv[1]);require_fixed();e=next(e for e in PLAN['changes']if e['key']=='SinestraIndomitableIsolation');source=e['edits'][0]['new'];assert source.count('GetCaster()->CastSpell(GetTarget(), SPELL_INDOMITABLE_DMG, true);')==1
source=source.replace('GetCaster()->CastSpell(GetTarget(), SPELL_INDOMITABLE_DMG, true);','Observe::CastSpell(GetCaster()->ToCreature(), GetTarget(), SPELL_INDOMITABLE_DMG, true);')
(out/'TenTenFirstNative.inc').write_text('namespace TenTenFirst{enum{SPELL_INDOMITABLE_DMG=90045};'+source+';}\n',encoding='utf8')
print('Prepared actual AuraScript class with a controlled child-cast endpoint;all state/native getters/periodic registration retained')
