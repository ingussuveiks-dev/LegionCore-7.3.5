"""Strict, byte-preserving repair plan for misdecoded GameObject names.

This restores authored UTF-8 text. It does not translate names or reconstruct
missing gameplay. Undefined Windows-1252 bytes retain their C1 identity.
"""
from pathlib import Path
import argparse,json
UNDEFINED={0x81,0x8d,0x8f,0x90,0x9d}
def recover(text):
    raw=bytearray()
    for char in text:
        try:raw.extend(char.encode('cp1252'))
        except UnicodeEncodeError:
            if ord(char) not in UNDEFINED:raise
            raw.append(ord(char))
    fixed=bytes(raw).decode('utf-8','strict')
    restored=''.join(chr(b) if b in UNDEFINED else bytes([b]).decode('cp1252') for b in raw)
    if restored!=text or fixed==text:raise ValueError('Not an exact one-pass roundtrip')
    if any(ord(c)<32 or 127<=ord(c)<=159 for c in fixed):raise ValueError('Decoded control character')
    if not any('\u0400'<=c<='\u04ff' for c in fixed) or any(c in fixed for c in ('Ð','Ñ','�')):raise ValueError('Unsupported recovery')
    return fixed
META={'before_hex','after_hex','after_name','other_fingerprint','spawn_count','maps','batch','fix'}
def fingerprint(fields,alias='g'):
    return "SHA2(CONCAT_WS('|',"+','.join(f"COALESCE(HEX(CAST({alias}.`{f}` AS CHAR CHARACTER SET utf8mb4)),'NULL')" for f in fields)+'),256)'
def sql_for(rows,fields,batch):
    ids=','.join(str(x['entry']) for x in rows)
    def case(key):return 'CASE g.entry '+ ' '.join(f"WHEN {x['entry']} THEN '{x[key]}'" for x in rows)+' END'
    fp=fingerprint([f for f in fields if f!='name'])
    old,new,hashes=case('before_hex'),case('after_hex'),case('other_fingerprint')
    return f"""-- Batch{batch}: restore ten exact authored UTF-8 names, not a translation.
-- Strict Windows-1252 byte reversal; only known old/new text and metadata admitted.
START TRANSACTION;
SET @go_name_safe=((SELECT COUNT(*) FROM gameobject_template g WHERE g.entry IN({ids}) AND HEX(g.name) IN({old},{new}) AND {fp}={hashes})=10);
SET @go_name_guard=IF(COALESCE(@go_name_safe,0),'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''GameObject name batch{batch} differs; review before updating''');
PREPARE go_name_guard FROM @go_name_guard;EXECUTE go_name_guard;DEALLOCATE PREPARE go_name_guard;
UPDATE gameobject_template g SET name=CONVERT(UNHEX({new}) USING utf8mb4) WHERE @go_name_safe AND g.entry IN({ids}) AND HEX(g.name)={old} AND {fp}={hashes};
COMMIT;
"""
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--sql-directory',required=True);ap.add_argument('--generate',action='store_true');a=ap.parse_args()
    root=Path(__file__).resolve().parents[2];plan=json.loads((root/'tools/tests/GameObjectName20261115Repairs.json').read_text(encoding='utf-8'));rows=plan['repairs']
    assert len(rows)==len({x['entry'] for x in rows})==100
    fields=[f for f in rows[0] if f not in META]
    for x in rows:
        assert x['after_name']==recover(x['name'])
        assert x['name'].encode('utf-8').hex().upper()==x['before_hex']
        assert x['after_name'].encode('utf-8').hex().upper()==x['after_hex']
        assert int(x['spawn_count'])>0 and 1<=x['batch']<=10
        assert len(x['other_fingerprint'])==64
    dest=Path(a.sql_directory);dest.mkdir(parents=True,exist_ok=True)
    for batch in range(1,11):
        cohort=[x for x in rows if x['batch']==batch];assert len(cohort)==10
        p=dest/f'2026_11_15_{batch+69:02}_gameobject_name_batch{batch}.sql';expected=sql_for(cohort,fields,batch)
        if a.generate:p.write_text(expected,encoding='utf-8',newline='\n')
        else:assert p.read_text(encoding='utf-8')==expected,p
    # Invalid/incomplete encodings, already-correct text and unsafe controls refuse.
    for invalid in ('Prison Runestone','Тюремный камень','Ð','Ð¢\x00','Ð¢\u0081'):
        try:recover(invalid)
        except (ValueError,UnicodeError):pass
        else:raise AssertionError('Unsupported text admitted')
    print('PASS100 exact lossless name recoveries/ten10 cohorts; byte guards and generated SQL scope verified; malformed/already-correct strings refused')
if __name__=='__main__':main()
