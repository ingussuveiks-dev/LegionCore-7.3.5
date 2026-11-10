from pathlib import Path
import subprocess
r=Path(__file__).resolve().parents[2];p='src/server/scripts/Pandaria/TerraceEndlessSpring/boss_tsulong.cpp'
old=subprocess.check_output(['git','-C',str(r),'show','3d9746b2c3d81dd4685756e2c7fd00f96cce9d6e:'+p]).decode('utf8').replace('\r\n','\n');expected=old
expected=expected.replace('        uint8 phase;','        uint8 phase = PHASE_NONE;',1)
for name in ['dayhealth','nighthealth','healtoreduce','currenthealth']:expected=expected.replace('        int32 '+name+';\n','',1)
expected=expected.replace('''        void DamageTaken(Unit* attacker, uint32 &damage, DamageEffectType dmgType)
        {
            currenthealth = me->GetHealth();
            dayhealth = me->GetMaxHealth() - me->GetHealth();
        }
''','',1)
expected=expected.replace('            inFly = false;\n            won = false;','            inFly = false;\n            won = false;\n            phase = PHASE_NONE;',1)
for cache,source,target in [('dayhealth','NIGHT','DAY'),('nighthealth','DAY','NIGHT')]:
 expected=expected.replace('me->SetHealth('+cache+');','// Invert the current native health at the actual transition.\n                // A pre-damage or previous-tick cache can omit the last hit/heal.\n                me->SetHealth(me->GetMaxHealth() - me->GetHealth());',1)
 expected=expected.replace('if (action == PHASE_'+target+'_ACTIVATION)','if (action == PHASE_'+target+'_ACTIVATION && phase == PHASE_'+source+')',1)
expected=expected.replace('                    nighthealth = me->GetMaxHealth() - me->GetHealth();\n','',1)
assert (r/p).read_text(encoding='utf8')==expected,'Outside selected health/phase state source changed'
print('PASS Tsulong source scope: only removal of stale32bit caches,current64bit inversion at two actual phase actions,phase initialization/reset and same-phase replay rejection. Timers,calls,spells,native death/loot/group credit and all other classes unchanged.')
