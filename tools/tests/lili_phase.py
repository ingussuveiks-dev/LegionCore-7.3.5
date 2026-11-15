"""A shared phase for the authored Li Li/father DATA exchange, not new LINKs."""
from pathlib import Path
import argparse,json
from gameobject_name_encoding import fingerprint
ROOT=Path(__file__).resolve().parents[2]
FILE='2026_11_15_87_lili_father_phase.sql'
SCOPES={'creature':'g.guid IN(340237,372111,382407)','smart_scripts':'g.source_type=0 AND g.entryorguid IN(100475,100481)','creature_template':'g.entry IN(100475,100481)','conditions':'g.SourceTypeOrReferenceId=23 AND g.SourceGroup=7902','phase_definitions':'g.zoneId=7902'}
def load():return json.loads((ROOT/'tools/tests/LiliPhase20261115.json').read_text(encoding='utf-8'))
def cols(p,t):return list(p['spawns'][0]) if t=='creature' else p['tables'][t]
def sql(p):
 s=['-- Preserve shared base-phase visibility and enable the authored father/daughter','-- actor-to-actor target19 callbacks. No missing LINK payload is reconstructed.','START TRANSACTION;SET SESSION group_concat_max_len=10000000;','SET @lili_phase_safe=1;']
 for t,proof in p['proofs'].items():s.append(f"SET @lili_phase_safe=@lili_phase_safe AND ((SELECT COUNT(*)={proof['count']} AND SHA2(GROUP_CONCAT({fingerprint(cols(p,t))} ORDER BY {fingerprint(cols(p,t))} SEPARATOR ','),256)='{proof['hash']}' FROM `{t}` g WHERE {SCOPES[t]}));")
 for r in p['spawns']:
  states="'6735','5910 6735'" if r['guid']==340237 else "'"+r['PhaseId']+"'";s.append(f"SET @lili_phase_safe=@lili_phase_safe AND ((SELECT COUNT(*) FROM creature g WHERE g.guid={r['guid']} AND g.PhaseId IN({states}) AND {fingerprint([c for c in r if c!='PhaseId'])}='{p['spawn_fingerprints'][str(r['guid'])]}')=1);")
 s+= ["SET @lili_phase_guard=IF(COALESCE(@lili_phase_safe,0),'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Li Li phase context differs; review before updating''');PREPARE lili_phase_guard FROM @lili_phase_guard;EXECUTE lili_phase_guard;DEALLOCATE PREPARE lili_phase_guard;","UPDATE creature SET PhaseId='5910 6735' WHERE @lili_phase_safe AND guid=340237 AND PhaseId='6735';",'COMMIT;']
 return '\n'.join(s)+'\n'
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--sql-file',type=Path,required=True);ap.add_argument('--generate',action='store_true');a=ap.parse_args();p=load();assert len(p['spawns'])==3 and p['proofs']['smart_scripts']['count']==12
 if a.generate:a.sql_file.parent.mkdir(parents=True,exist_ok=True);a.sql_file.write_text(sql(p),encoding='utf-8',newline='\n')
 else:assert a.sql_file.read_text(encoding='utf-8')==sql(p)
 print('PASS one scoped phase union; original12 SmartAI rows,22 native phase definitions/48 conditions and3 spawn contexts guarded; unknown LINKs untouched')
if __name__=='__main__':main()
