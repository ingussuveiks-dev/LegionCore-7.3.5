from pathlib import Path
import subprocess
r=Path(__file__).resolve().parents[2];path='src/server/scripts/Legion/ReturnToKarazhan/karazhan_chess.cpp'
old=subprocess.check_output(['git','-C',str(r),'show','be9d35ed166a2d07fd5360b2438e76a0c291b739:'+path]).decode('utf8').replace('\r\n','\n')
new=(r/path).read_text(encoding='utf8')
assert old[old.index('// 115388'):].rstrip('\n')==new[new.index('// 115388'):].rstrip('\n'), 'King/intro/registrations changed'
for start,end in [('case NPC_CHESS_HORSE:', 'case NPC_CHESS_ELEPHANT_DARK:'),
                  ('case NPC_CHESS_ELEPHANT_DARK:', 'case NPC_CHESS_ROOK:'),
                  ('case NPC_CHESS_QUEEN:', '\n            }\n        }')]:
 def section(s):
  a=s.index(start);return s[a:s.index(end,a)]
 assert section(old)==section(new),start
assert new.count('if (aiLifetimeGuard.expired()) return;')==6
assert new.count('aiLifetimeGuard = std::weak_ptr<uint8>(_delayedAILifetimeGuard)')==6
assert 'Scripted_NoMovementAI::UpdateAI(diff);' in new
print('PASS Chess scope: King/intro/registration and four shared figure branches byte-preserved; six existing weak guards retained. Rook has no delayed AI capture; actual complete class tested separately. Old35 audit/source projections remain historical, not rewritten as new gameplay evidence.')
