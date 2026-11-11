from pathlib import Path
import json,subprocess
from healing_width_scope import retained_source,pinned_sources
r=Path(__file__).resolve().parents[2];plan=json.loads((r/'tools/tests/NativeHealApiChanges.json').read_text())
originals=pinned_sources(plan['baseline'],[row['path']for row in plan['changes']])
for row in plan['changes']:
 text=originals[row['path']].decode().replace('\r\n','\n')
 lines=text.splitlines(True)
 for edit in reversed(row['edits']):
  index=edit['line'];old=edit['old'].splitlines(True)
  assert lines[index:index+len(old)]==old,(row['path'],index)
  lines[index:index+len(old)]=edit['new'].splitlines(True)
 text=''.join(lines)
 assert text==retained_source(row['path']),'Unrelated source change:'+row['path']
print('PASS pinned native32heal amendment:',len(plan['changes']),'historical files plus independently certified unsigned64healing amendment;historical quest consumer changes remain unused gain signatures only')
