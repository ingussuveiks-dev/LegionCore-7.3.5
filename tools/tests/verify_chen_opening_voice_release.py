"""Chen's ordinary first reply must not precede its decoded native voice end."""
from pathlib import Path
import hashlib,json,runpy
from sharasdal_erunak_entry_pe import sites
p=runpy.run_path(str(Path(__file__).with_name('verify_monk_chen_early_presentation_release.py')))
n=p['n'];body=p['setdata'];queues=sites(n,body,'?AddDelayedEvent@FunctionProcessor@@')
definitions=p['p']['p'];table=definitions['table'];refs=definitions['refs'];resolved=definitions['resolved'];ins=n['instructions']
actor_table,actor_slots=table('??_7npc_monk_chen_porterAI@npc_monk_chen_porter@@6B@',82)
script_table,script_slots=table('??_7npc_monk_chen_porter@@6BScriptObject@@@',16)
factory=ins('?GetAI@npc_monk_chen_porter@@')
assert actor_table in refs(factory)
assert n['symbol']('?GetAI@npc_monk_chen_porter@@') in resolved(script_slots)
assert script_table in refs(ins('?AddSC_monk_intro_scenario@@'))
assert n['symbol']('?SetData@npc_monk_chen_porterAI@npc_monk_chen_porter@@') in resolved(actor_slots)
assert len(queues)==3
for at,delay in zip(queues,(4443,5000,8000)):
 assert [(o.mnemonic,o.op_str) for o in body[at-2:at]]==[
  ('mov',f'edx, {delay:#x}'),('mov','rcx, rsi')]
assert ('cmp','edi, 0x115b') in [(o.mnemonic,o.op_str) for o in p['early']]
print('PASS actual registered scoped Chen presentation and shared/unscoped actor both use4443ms first-reply bound, retaining5000ms student/8000ms route queues and all original receipt/owner/AI-lifetime/native registration predicates. Bound independently verified by full hashed native Vorbis decode; client playback/late coalesced delivery remain separate.')
print(json.dumps({'world_sha256':hashlib.sha256(n['binary']).hexdigest(),'broadcast':99965,'soundkit':62031,'filedata':1384167,'first_voice_ms':4443,'student_ms':5000,'route_ms':8000}))
