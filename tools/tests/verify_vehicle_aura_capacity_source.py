"""Certify the capacity repair preserves every unrelated native aura rule."""
from pathlib import Path
import subprocess
r=Path(__file__).resolve().parents[2];path='src/server/game/Spells/Auras/SpellAuras.cpp'
old=subprocess.check_output(['git','-C',str(r),'show','798e39578:'+path],encoding='utf-8');new=(r/path).read_text()
start='    if (HasEffectType(SPELL_AURA_CONTROL_VEHICLE) && existingAura->HasEffectType(SPELL_AURA_CONTROL_VEHICLE))'
def block(s):
 a=s.index(start);b=s.index('{',a)+1;depth=1
 while depth:depth+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
expected='''    if (HasEffectType(SPELL_AURA_CONTROL_VEHICLE) && existingAura->HasEffectType(SPELL_AURA_CONTROL_VEHICLE))
    {
        // Compatibility must not evict existing riders to make room. Native
        // Vehicle::AddPassenger owns capacity and explicit-seat replacement;
        // its aborted join removes only the unsuccessful incoming aura.
        return true;
    }'''
assert block(new)==expected,'Unreviewed vehicle-aura compatibility change'
assert ''.join(new.replace(expected,block(old),1).split())==''.join(old.split()),'Unrelated native aura code changed'
print('PASS whole native aura source scope:pinned798e39578 except exact control-vehicle pair compatibility;native capacity/explicit seat admission and unrelated stacking/single-target/proc rules retained')
