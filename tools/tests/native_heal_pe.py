"""Selected unsigned64 healing ABI;actual native matrices are separate."""
def verify(ins,sites,values,decoder,n):
 def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
 rows=ins('?DealHeal@Unit@@')
 assert op(rows,'mov','qword ptr [rbp - 0x59], r8'),'Native amount must enter as unsigned64'
 assert not sites(rows,'?ModifyHealth@Unit@@'),'Positive healing uses bounded unsigned AddHealth'
 health,=sites(rows,'?GetHealth@Unit@@QEBA_KPEAV1@@Z');maximum,=sites(rows,'?GetMaxHealth@Unit@@QEBA_KPEAV1@@Z');applied,=sites(rows,'?SetHealthScal@Unit@@');notified,=sites(rows,'?NotifyHealApplied64@Creature@@')
 assert health<maximum<applied<notified
 for m,s in [('sub','rcx, rbx'),('cmp','rax, rbx'),('cmovb','r14, rcx'),('lea','rdx, [r14 + rbx]'),('mov','r9, r14')]:assert op(rows,m,s),(m,s)
 assert op(rows,'mov','rax, r14'),'Actual64gain return must not truncate'
 assert sites(rows,'?UpdateDamageManager@InstanceScript@@')and op(rows,'mov','r8, rbx'),'Shared heal retains full positive amount'
 assert len(sites(rows,'?UpdateAchievementCriteria@Player@@'))==4
 assert sum(x.mnemonic=='cmova'and x.op_str=='r8d, r15d'for x in rows)==4 and 0xffffffff in values(rows),'Native32criterion input must saturate,not wrap'
 rows=ins('?HealBySpell@Unit@@');assert sites(rows,'?CalcHealAbsorb@Unit@@')and sites(rows,'?DealHeal@Unit@@')and op(rows,'mov','rax, rdi'),'Native heal flow/full gain retained'
 rows=ins('?CalcHealAbsorb@Unit@@')
 for m,s in [('mov','r12, qword ptr [r9]'),('cvttss2si','rax, xmm0'),('cmovb','r9, rax'),('sub','r12, r9'),('mov','qword ptr [r14], r12')]:assert op(rows,m,s),(m,s)
 rows=ins('?AbsorbDamage@DamageInfo@@')
 for m,s in [('test','rdx, rdx'),('neg','r9'),('sub','rdx, r9'),('mov','qword ptr [r10], rdx'),('mov','qword ptr [r11], rax')]:assert op(rows,m,s),(m,s)
 rows=ins('?AfterAbsorb@spell_endurance_of_niuzao_AuraScript@');at,=sites(rows,'?AbsorbDamage@DamageInfo@@');window=rows[at-4:at]
 assert any(x.mnemonic=='cvttss2si'and x.op_str.startswith('rdx,')for x in window)and op(window,'neg','rdx'),'Niuzao rejection must retain64signed negation'
 rows=ins('?UpdateDamageManager@InstanceScript@@');assert sites(rows,'?SetHealth@Unit@@')and sites(rows,'?Kill@Unit@@')and not op(rows,'neg','r8'),'Native shared health/kill endpoints retained with positive unsigned input'
 rows=ins('?SetHealingValue@Spell@@')
 assert {0x20,0xa,0x20000000}<=set(values(rows))and sites(rows,'?GetEffect@SpellInfo@@'),'Exact channel validates index,real profile HEAL and native fixed-heal attributes'
 assert any(x.mnemonic=='mov'and x.op_str.startswith('qword ptr [')and x.op_str.endswith(', r15')for x in rows),'Exact amount stored at full width'
 casts=[]
 for name in n['symbols']:
  if name.startswith('?CastSpell@Unit@@'):
   body=ins(name)
   if sites(body,'?SetHealingValue@Spell@@'):casts.append(body)
 assert len(casts)==2 and all(sites(body,'?prepare@Spell@@')for body in casts),'Both custom-value cast routes validate64payload before native prepare'
 print('PASS linked unsigned64 heal:bounded native health application,full AI amount/gain,integer heal absorb,physical undo,Niuzao,shared manager,guarded exact cast channel and native32criterion saturation. Packet projections and full native matrices are independent checks.')
