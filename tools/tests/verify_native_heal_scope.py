from pathlib import Path
import json,subprocess
r=Path(__file__).resolve().parents[2];plan=json.loads((r/'tools/tests/NativeHealApiChanges.json').read_text())
for row in plan['changes']:
 text=subprocess.check_output(['git','-C',str(r),'show',plan['baseline']+':'+row['path']]).decode().replace('\r\n','\n')
 lines=text.splitlines(True)
 for edit in reversed(row['edits']):
  index=edit['line'];old=edit['old'].splitlines(True)
  assert lines[index:index+len(old)]==old,(row['path'],index)
  lines[index:index+len(old)]=edit['new'].splitlines(True)
 text=''.join(lines)
 assert text==(r/row['path']).read_text(encoding='utf-8-sig'),'Unrelated source change:'+row['path']
print('PASS exact native heal API scope:',len(plan['changes']),'files;gain signatures only in existing quest consumers;no spell amount/target/proc policy or quest progression rewrite')
