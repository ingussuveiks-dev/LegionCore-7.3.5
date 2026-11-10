from pathlib import Path
import subprocess
r=Path(__file__).resolve().parents[2];p='src/server/scripts/Northrend/Ulduar/Ulduar/boss_freya.cpp'
old=subprocess.check_output(['git','-C',str(r),'show','278371694:'+p]).decode('utf8').replace('\r\n','\n')
expected=old.replace('#include "ulduar.h"','#include "ulduar.h"\n#include <limits>',1)
expected=expected.replace('                me->SetMaxHealth(me->GetMaxHealth() + (me->GetMaxHealth() * EldersCount * 20 / 100));','''                // Twenty percent is one fifth. Divide before multiplying and
                // bound the bonus by the remaining native health range.
                uint64 const health = me->GetMaxHealth();
                uint64 const room = std::numeric_limits<uint64>::max() - health;
                uint64 const fifth = health / 5;
                uint64 bonus = room;
                if (fifth <= room / EldersCount)
                {
                    bonus = fifth * EldersCount;
                    bonus += std::min(room - bonus, health % 5 * EldersCount / 5);
                }
                me->SetMaxHealth(health + bonus);''',1)
assert (r/p).read_text(encoding='utf8')==expected
print('PASS Freya scope: only full-width overflow-safe authored20percent calculation;actual elder scan,zero-elder branch,fullhealth/loot calls,timers,reset,combat,death,group/owner credit and all other NPC/spell classes preserved. Controlled native count/input tests do not certify full raid/client traversal.')
