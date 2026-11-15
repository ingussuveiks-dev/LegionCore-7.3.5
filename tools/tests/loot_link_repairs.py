from pathlib import Path
import argparse,json
from gameobject_name_encoding import fingerprint
ROOT=Path(__file__).resolve().parents[2]
FILE='2026_11_15_84_creature_loot_links.sql'
def load():return json.loads((ROOT/'tools/tests/LootLink20261115Repairs.json').read_text(encoding='utf-8'))
def predicate(r):return f"(g.entry={r['entry']} AND g.lootid IN(0,{r['entry']}) AND {fingerprint([c for c in r['values'] if c!='lootid'])}='{r['other_fingerprint']}')"
def sql(p):
 rows=p['repairs'];out=['-- Restore existing same-entry authored loot through native lootid dispatch.','-- No loot chances, items, quest rules, personal flags or scripts are changed.','START TRANSACTION;SET SESSION group_concat_max_len=10000000;',"SET @loot_link_safe=((SELECT COUNT(*) FROM creature_template g WHERE "+' OR '.join(predicate(r) for r in rows)+')=13);']
 for r in rows:
  out.append(f"SET @loot_link_safe=@loot_link_safe AND ((SELECT COUNT(*)={r['loot_rows']} AND SHA2(GROUP_CONCAT({fingerprint(p['loot_columns'])} ORDER BY g.Item SEPARATOR ','),256)='{r['loot_signature']}' FROM creature_loot_template g WHERE g.Entry={r['entry']}));")
  out.append(f"SET @loot_link_safe=@loot_link_safe AND EXISTS(SELECT 1 FROM creature WHERE id={r['entry']}) AND EXISTS(SELECT 1 FROM creature_template_wdb WHERE Entry={r['entry']} AND Type={r['context']['wdb_type']} AND Classification={r['context']['wdb_classification']}) AND NOT EXISTS(SELECT 1 FROM personal_loot_template WHERE type=1 AND entry={r['entry']}) AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=1 AND SourceGroup={r['entry']});")
 out.append("SET @loot_link_guard=IF(COALESCE(@loot_link_safe,0),'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Creature loot link/source differs; review before updating''');PREPARE loot_link_guard FROM @loot_link_guard;EXECUTE loot_link_guard;DEALLOCATE PREPARE loot_link_guard;")
 for r in rows:out.append(f"UPDATE creature_template g SET lootid={r['entry']} WHERE @loot_link_safe AND g.lootid=0 AND {predicate(r)};")
 return '\n'.join(out+['COMMIT;'])+'\n'
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--sql-file',type=Path,required=True);ap.add_argument('--generate',action='store_true');a=ap.parse_args();p=load();assert len(p['repairs'])==13
 for r in p['repairs']:
  v=r['values'];assert v['lootid']==0 and not v['AIName'] and not v['ScriptName'] and not(v['unit_flags']&0x2020102) and int(r['context']['spawns'])>0 and not int(r['context']['personal']) and not int(r['context']['conditions']) and r['loot_rows']>0
 if a.generate:a.sql_file.parent.mkdir(parents=True,exist_ok=True);a.sql_file.write_text(sql(p),encoding='utf-8',newline='\n')
 else:assert a.sql_file.read_text(encoding='utf-8')==sql(p)
 print('PASS13 missing same-entry loot links; exact template/source guards; native personal flags, authored chances/items/quest rules untouched')
if __name__=='__main__':main()
