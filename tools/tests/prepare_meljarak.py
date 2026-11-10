from pathlib import Path
import subprocess,sys,json
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
p='src/server/scripts/Pandaria/HeartFear/boss_lord_meljarak.cpp'
s=(subprocess.check_output(['git','-C',str(r),'show','d83751688:'+p]).decode('utf8') if len(sys.argv)>2 and sys.argv[2]=='baseline' else (r/p).read_text(encoding='utf8'))
(out/'Meljarak.inc').write_bytes(s[s.index('enum eSpells'):s.index('//67053')].encode('utf8'))
s=(r/'src/server/scripts/Pandaria/HeartFear/instance_heart_of_fear.cpp').read_text(encoding='utf8')
(out/'MeljarakInstance.inc').write_bytes(s[s.index('const DoorData'):s.index('void AddSC_instance_heart_of_fear')].encode('utf8'))
p=next(p for p in [r/'tools/sql/staged/2026_11_10_02_meljarak_recklessness_targets.sql',r/'sql/updates/world/2026_11_10_02_meljarak_recklessness_targets.sql'] if p.exists())
sql='\n'.join(line for line in p.read_text(encoding='utf8').splitlines() if not line.lstrip().startswith('--'))
(out/'MeljarakSql.inc').write_bytes(('\n'.join(json.dumps(part.strip())+',' for part in sql.split(';') if part.strip())+'\n').encode('utf8'))
