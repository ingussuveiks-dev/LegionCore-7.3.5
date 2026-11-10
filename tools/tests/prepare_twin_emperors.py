from pathlib import Path
import subprocess,sys,json
r=Path(__file__).resolve().parents[2];p='src/server/scripts/Kalimdor/TempleOfAhnQiraj/boss_twinemperors.cpp'
s=(subprocess.check_output(['git','-C',str(r),'show','0f4776548:'+p]).decode('utf8') if len(sys.argv)>2 and sys.argv[2]=='baseline' else (r/p).read_text(encoding='utf8'))
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
(out/'TwinEmperors.inc').write_bytes(s[s.index('#define SPELL_HEAL_BROTHER'):s.index('void AddSC_boss_twinemperors')].encode('utf8'))
paths=[r/'tools/sql/staged/2026_11_10_01_twin_emperors_encounter.sql',r/'sql/updates/world/2026_11_10_01_twin_emperors_encounter.sql']
sql=next(p for p in paths if p.exists()).read_text(encoding='utf8')
sql='\n'.join(line for line in sql.splitlines() if not line.lstrip().startswith('--'))
# These statements contain no quoted semicolon or stored-program body.
(out/'TwinEncounterSql.inc').write_bytes(('\n'.join(json.dumps(part.strip())+',' for part in sql.split(';') if part.strip())+'\n').encode('utf8'))
