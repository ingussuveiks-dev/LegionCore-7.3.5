"""Selected native healing/absorption widths;actual matrices are separate."""
def verify(ins,sites,values,decoder,n):
 def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
 rows=ins('?DealHeal@Unit@@');at,=sites(rows,'?ModifyHealth@Unit@@');window=rows[at-6:at]
 assert op(window,'mov','edx, r8d') and not any(x.mnemonic=='movsxd'for x in window),'Positive uint32 heal must zero-extend to native64ModifyHealth'
 notified,=sites(rows,'?NotifyHealApplied@Creature@@');assert op(rows[notified-4:notified],'mov','r9, rax'),'Actual full gain must reach final AI notification'
 assert op(rows,'mov','rax, r14'),'Actual64gain return must not truncate'
 assert sites(rows,'?UpdateDamageManager@InstanceScript@@')and op(rows,'neg','rbx'),'Shared heal must negate after zero extension'
 rows=ins('?HealBySpell@Unit@@');assert sites(rows,'?CalcHealAbsorb@Unit@@')and sites(rows,'?DealHeal@Unit@@')and op(rows,'mov','rax, rdi'),'Native heal flow/full gain retained'
 rows=ins('?CalcHealAbsorb@Unit@@');assert op(rows,'cvttsd2si','r9, xmm1')and op(rows,'sub','r12d, r9d'),'Remaining heal must subtract a bounded integer amount'
 start=n['symbol']('?AbsorbDamage@DamageInfo@@');end=min(n['resolve'](v)for k,v in n['symbols'].items()if k.startswith('?')and n['resolve'](v)>start);assert 0<end-start<512
 rows=list(decoder.disasm(n['binary'][n['offset'](start):n['offset'](end)],start))
 assert op(rows,'test','rdx, rdx')and op(rows,'cmp','r9, rdx')and op(rows,'sub','r10d, edx')and op(rows,'add','eax, r10d'),'Native leaf absorption must handle full32positive and bounded64negative undo'
 rows=ins('?AfterAbsorb@spell_endurance_of_niuzao_AuraScript@');at,=sites(rows,'?AbsorbDamage@DamageInfo@@');window=rows[at-4:at]
 assert any(x.mnemonic=='cvttss2si'and x.op_str.startswith('rdx,')for x in window)and op(window,'neg','rdx'),'Niuzao rejection must retain64signed negation'
 rows=ins('?UpdateDamageManager@InstanceScript@@');assert op(rows,'test','r8, r8')and sites(rows,'?SetHealth@Unit@@')and sites(rows,'?Kill@Unit@@'),'Native shared manager/full signed amount,health and original kill endpoint retained'
 print('PASS linked native heal:zero-extended32amount,64gain/notification/return,integer heal-absorb remainder,full32absorption/bounded negative undo,Niuzao64negation and native shared-manager endpoints. Does not certify all signed32SpellEffect/target/proc records.')
