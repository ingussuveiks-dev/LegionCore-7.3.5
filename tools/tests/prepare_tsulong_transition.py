from pathlib import Path
import subprocess,sys
r=Path(__file__).resolve().parents[2];p='src/server/scripts/Pandaria/TerraceEndlessSpring/boss_tsulong.cpp'
s=(subprocess.check_output(['git','-C',str(r),'show','3d9746b2c3d81dd4685756e2c7fd00f96cce9d6e:'+p]).decode('utf8')
   if len(sys.argv)>2 and sys.argv[2]=='baseline' else (r/p).read_text(encoding='utf8'))
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
(out/'TsulongTransition.inc').write_bytes(s[s.index('enum eTsulongEvents'):s.index('class npc_sunbeam')].encode('utf8'))
