"""Complete changed pet battle methods; DB2/cast/session endpoints controlled."""
import json
from ten_ten_20261111_scope import ROOT,PLAN,require_fixed
from prepare_ten_ten_20261111_fifth import block

def prepare(out,mode):
 rows=[e for e in PLAN['changes']if e['batch']==9];assert len(rows)==10;require_fixed([e['key']for e in rows])
 paths={p for e in rows for p in e['paths']};sources={p:(ROOT/p).read_text()for p in paths}
 if mode=='baseline':
  for e in reversed(rows):
   for ed in reversed(e['edits']):assert sources[ed['path']].count(ed['new'])==1;sources[ed['path']]=sources[ed['path']].replace(ed['new'],ed['old'],1)
 signatures=[]
 for e in rows:
  for ed in e['edits']:
   if ed['path'].endswith('.h'):continue
   sig=ed['new'].split('{')[0].strip()
   if sig not in [a[1]for a in signatures]:signatures.append((ed['path'],sig))
 import re
 text='\n'.join(block(sources[p],re.escape(sig.split('PetBattleAbilityImplicitTarget')[0]+'PetBattleAbilityImplicitTarget')if 'PetBattleAbilityImplicitTarget'in sig else re.escape(sig))for p,sig in signatures)
 pe=next(p for p in paths if p.endswith('PetBattleAbilityEffect.cpp'))
 text+='\n'+block(sources[pe],r'bool PetBattleAbilityEffect::AddTarget\(uint32 target\)')
 pb=next(p for p in paths if p.endswith('/PetBattle.cpp'));text+='\n'+block(sources[pb],r'void PetBattle::ProceedRound\(')
 out.mkdir(parents=True,exist_ok=True);(out/'Selected.inc').write_text(text);(out/'cases.json').write_text(json.dumps([e['key']for e in rows]));print('Extracted complete pet battle changed methods; fixture endpoints explicit')
