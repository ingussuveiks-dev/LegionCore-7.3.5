from pathlib import Path
import json,runpy,re
p=runpy.run_path(str(Path(__file__).with_name('verify_fortyfifth_ten_release.py')))
loader=p
while 'callbacks'not in loader:loader=loader['p']
p=loader|p;n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values']
plan=json.loads(Path(__file__).with_name('FortySixthTenRepairs.json').read_text())
def is_null_test(x):
 return x.mnemonic=='test'and re.fullmatch(r'(r\w+), \1',x.op_str)is not None
def nullable_argument_guard(rows,call):
 for j in range(max(0,call-16),call-1):
  if not is_null_test(rows[j])or rows[j+1].mnemonic not in ['je','jz']:continue
  reg=rows[j].op_str.split(',')[0]
  if reg=='rcx'and not any(x.mnemonic=='mov'and x.op_str.startswith('rcx,')for x in rows[j+2:call]):return True
  if any(x.mnemonic=='mov'and x.op_str=='rcx, '+reg for x in rows[j+2:call]):return True
 return False
def result_guard(rows,call):
 aliases={'rax'}
 for x,y in zip(rows[call+1:call+7],rows[call+2:call+8]):
  m=re.fullmatch(r'(r\w+), (r\w+)',x.op_str)if x.mnemonic=='mov'else None
  if m and m[2]in aliases:aliases.add(m[1])
  if is_null_test(x)and x.op_str.split(',')[0]in aliases and y.mnemonic in ['je','jz']:return True
 return False
for e in plan:
 rows=ins('?JustDied@'+e['cls']+'@')
 if e['kind']=='victim_instance':
  call,=sites(rows,'?GetInstanceScript@WorldObject@@')
  assert any(x.mnemonic=='mov'and x.op_str=='rcx, qword ptr [rcx + 0x20]'for x in rows[max(0,call-5):call]),'Magmus instance getter must receive the victim from this->me'
  assert result_guard(rows,call),'missing victim instance remains guarded'
 else:
  call,=sites(rows,'?GetAffectingPlayer@Unit@@')
  assert nullable_argument_guard(rows,call),(e['key'],'nullable killer admission before native getter')
  assert result_guard(rows,call),(e['key'],'unowned/missing player result rejected before progress')
  if e['index']<3:assert [101167,53085,100992][e['index']]in values(rows),'original achievement ID retained'
  if e['index']in[6,7,8]:assert sites(rows,'?PermBindAllPlayers@InstanceMap@@'),'original native all-player binding endpoint retained'
print('PASS linked final ten:nine real death hooks resolve nullable killers through native GetAffectingPlayer with result admission;Magmus uses victim instance and null-result guard. Original achievement IDs/native bind endpoints and the complete prior corrected ownership/45-group chain retained. Full source/model/native/client scopes remain distinct.')
