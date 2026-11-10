from pathlib import Path
import subprocess,sys
r=Path(__file__).resolve().parents[2];p='src/server/scripts/Northrend/Ulduar/Ulduar/boss_freya.cpp'
s=(subprocess.check_output(['git','-C',str(r),'show','278371694:'+p]).decode('utf8')
   if len(sys.argv)>2 and sys.argv[2]=='baseline' else (r/p).read_text(encoding='utf8'))
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
(out/'FreyaElderHealth.inc').write_bytes(s[s.index('enum Yells'):s.index('class npc_elder_brightleaf')].encode('utf8'))
