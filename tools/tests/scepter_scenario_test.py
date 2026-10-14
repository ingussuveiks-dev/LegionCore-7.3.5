"""SQL-driven interaction replay; native condition execution is tested in C++.

This does not emulate client rendering, navigation, spell validation or network
teleport acknowledgements. Those remain explicit client checks in the audit.
"""
import collections
import json
import math
from pathlib import Path
import sys

parts = Path(sys.argv[1]).read_text(encoding='utf-8-sig').split('PASS2\n')
assert len(parts) == 2
first = parts[0].split('PASS1\n')[1].strip().splitlines()
second = parts[1].strip().splitlines()
assert first == second, 'migration is not idempotent'
data = collections.defaultdict(list)
for line in second:
    kind, record = line.split('\t', 1)
    data[kind].append(json.loads(record))
scripts = collections.defaultdict(dict)
for r in data['smart']:
    key = (r['entry'], r['source'])
    assert r['id'] not in scripts[key], ('duplicate event id', key, r['id'])
    scripts[key][r['id']] = r
for key, rows in scripts.items():
    for r in rows.values():
        if r['link']:
            assert r['link'] in rows and rows[r['link']]['event'] == 61, ('dangling link', key, r)
conditions = collections.defaultdict(list)
for c in data['condition']:
    conditions[c['entry'], c['source'], c['group'] - 1].append(c)
    if c['type'] == 44 and c['v1'] == 1155:
        assert c['target'] == 1, ('null-invoker stage gate', c)
native = json.loads(Path('docs/audits/scepter-native-2026-10-14.json').read_text())
expected = [(c['Type'], c['Asset']) for c in native['criteria']]
assert expected == [(92,52741),(73,52859),(92,52860),(92,52861),(92,52869),
                    (92,52870),(92,52898),(92,52899),(92,52900),(92,52901)]


class Actor:
    def __init__(self, entry, pos, source=0):
        self.entry, self.pos, self.source = entry, pos, source
        self.phase, self.path, self.last = 0, 0, None
        self.once, self.text = set(), None
        self.alive, self.present, self.flags = True, True, 0


class Run:
    def __init__(self, team):
        self.team, self.stage, self.scenario = team, 0, 1155
        self.credits, self.objectives = [], set()
        self.failed_casts, self.teleports, self.actors = {218995,219706}, [], []
        self.player = Actor(1,(-1040,1149,99.5))
        self.pet = Actor(2,self.player.pos)
        self.initial = self.add(109956,(-1038.55,1151.64,99.6015))
        self.nagaz = self.add(109955,(-1038.59,1147.91,99.6015))
        self.tyranis = self.add(109964,(-981.262,1027.91,84.9762))
        self.hook = self.add(110030,(-979.62,1049.37,86.426))
        self.armond = self.add(110323,(-1001.7,1223.6,84.6656))
        self.boss = self.add(109981,(-72.5764,-4530.17,170.275))
        self.final = self.add(109956,(-356.042,-4525.17,170.275))
        self.manifest = self.add(251879,(-1002.84,1225.4,85.9329),1)
        self.entrance = self.add(-370846,(-1272.82,1050,106.995))
        for a in [self.boss,self.armond]:
            a.flags = next(r['flags'] for r in data['creature'] if r['entry'] == a.entry)
        self.manifest.flags = next(r['flags'] for r in data['go'] if r['entry']==251879)

    def add(self, entry, pos, source=0):
        a=Actor(entry,pos,source); self.actors.append(a); return a

    @staticmethod
    def distance(a,b):
        return math.dist(a.pos,b.pos)

    def near(self,a,entry,dist):
        candidates=[b for b in self.actors if b.entry==entry and b.present and b.alive
                    and self.distance(a,b)<=dist]
        return sorted(candidates,key=lambda b:self.distance(a,b))[:1]

    def allowed(self,a,r,invoker):
        groups=collections.defaultdict(list)
        for c in conditions[a.entry,a.source,r['id']]: groups[c['else']].append(c)
        def check(c):
            obj = a if c['target']==1 else invoker
            if not obj: return False
            if c['type']==44: result=self.scenario==c['v1'] and self.stage==c['v2']
            elif c['type']==29: result=bool(self.near(obj,c['v1'],c['v2']))
            else: raise AssertionError(('unmodeled condition',c))
            return not result if c['negative'] else result
        return not groups or any(all(check(c) for c in g) for g in groups.values())

    def emit(self,a,event,p1=0,p2=0,invoker=None):
        if not a.present: return
        for r in list(scripts[a.entry,a.source].values()):
            if r['event']!=event: continue
            if event in (38,40,52) and (r['e1']!=p1 or r['e2']!=p2): continue
            if event==8 and r['e1']!=p1: continue
            if event==10 and (not invoker or self.distance(a,invoker)>r['e2']): continue
            if r['phase'] and not r['phase'] & (1 << (a.phase-1) if a.phase else 0): continue
            if r['flags'] & 1 and r['id'] in a.once: continue
            if self.allowed(a,r,invoker): self.action(a,r,invoker)

    def targets(self,a,r,invoker):
        t=r['target']
        if t==1: return [a]
        if t==7: return [invoker or a.last] if invoker or a.last else []
        if t==21:
            # Native CLOSEST_PLAYER is creature-only, not a gameobject selector.
            return [self.player] if a.source==0 and self.distance(a,self.player)<=r['t1'] else []
        if t in (19,20): return self.near(a,r['t1'],r['t2'] or 100)
        if t in (8,15): return []  # position / door activation is handled below
        raise AssertionError(('unmodeled target',t,r))

    def action(self,a,r,invoker=None):
        a.once.add(r['id'])
        if invoker: a.last=invoker
        targets=self.targets(a,r,invoker)
        act=r['action']
        if act==45:
            for b in targets: self.emit(b,38,r['a1'],r['a2']) # native SetData(nullptr)
        elif act==205:
            if self.player in targets and self.stage<10 and (r['a1'],r['a2'])==expected[self.stage]:
                self.credits.append((r['a1'],r['a2'])); self.stage+=1
        elif act==1: a.text=r['a1']
        elif act==53: a.path=r['a2']
        elif act==41: a.present=False
        elif act==2: pass  # faction switch; combat is initiated explicitly in the replay
        elif act==11:
            if r['a1'] in self.failed_casts: return # native failed CAST skips linked actions
        elif act==85:
            if r['a1']==219338 and a.last: self.teleports.append(219338)
        elif act==12: self.add(r['a1'],(r['x'],r['y'],r['z']))
        elif act==33:
            if self.player in targets: self.objectives.add(r['a1'])
        elif act==19:
            for b in targets: b.flags &= ~r['a1']
        elif act==22: a.phase=r['a1']
        elif act==106:
            for b in targets: b.flags &= ~r['a1']
        elif act in (9,28,81,97,49,206,204): pass
        else: raise AssertionError(('unmodeled action',act,r))
        if r['link']: self.action(a,scripts[a.entry,a.source][r['link']],invoker)

    def text(self,a):
        assert a.text is not None
        text=a.text; a.text=None; self.emit(a,52,text,a.entry)

    def waypoint(self,a,path,point,pos):
        assert a.path==path, ('wrong/missing path',a.entry,a.path,path)
        a.pos=pos; self.player.pos=pos; self.pet.pos=pos
        self.emit(a,40,point,path)

    def kill(self,a,killer):
        assert not a.flags & 770, ('enemy attackable before its stage',a.entry)
        a.alive=False; self.emit(a,6,invoker=killer)


for team in ('Horde','Alliance'):
    w=Run(team)
    # Re-entry at D-Block, after event 601 has already been passed.
    w.emit(w.final,10,invoker=w.player); assert w.stage==0
    w.emit(w.initial,10,invoker=w.player); assert w.stage==1
    w.emit(w.initial,10,invoker=w.player) # Must not restart timed speech.
    w.text(w.initial); w.text(w.nagaz)
    assert w.initial.path==109956 and w.nagaz.path==109955
    w.nagaz.pos=(-1021.9,1126.01,91.3228)
    w.waypoint(w.initial,109956,4,(-1021.67,1132.44,91.3225))
    w.text(w.initial); assert w.stage==2
    # The third stage's event object credits arrival, not a prior NPC spawn.
    eo=Actor(60,(-973.112,1047.51,84.5811),13)
    w.player.pos=eo.pos; w.emit(eo,89,invoker=w.player); assert w.stage==3
    w.emit(w.hook,8,219203,invoker=w.player); assert w.stage==4
    w.emit(w.tyranis,8,219203,invoker=w.player); assert w.stage==4 # exact once
    nagaz=w.near(w.hook,109955,100)[0]
    w.player.pos=nagaz.pos
    w.emit(nagaz,10,invoker=w.player); w.emit(nagaz,10,invoker=w.player)
    w.text(nagaz); w.text(nagaz); assert w.stage==5
    w.waypoint(nagaz,10995500,7,(-913.879,1142.11,84.5403)); w.text(nagaz)
    w.kill(nagaz,w.pet); assert w.stage==6
    guide=w.near(w.manifest,109956,100)[0]
    w.player.pos=guide.pos; w.emit(guide,10,invoker=w.player); w.text(guide)
    assert w.stage==7
    # The page cannot be used early; killing Armond unlocks it but gives no credit.
    w.emit(w.manifest,64,invoker=w.player); assert w.stage==7
    assert w.armond.flags & 770
    w.waypoint(guide,10995600,2,(-969.075,1211.54,84.5822))
    assert not w.armond.flags & 770
    w.text(guide)
    w.player.pos=w.armond.pos; w.pet.pos=w.player.pos
    w.kill(w.armond,w.pet); assert w.stage==7 and w.manifest.phase==1
    assert not w.manifest.flags & 16
    w.waypoint(guide,10995601,1,(-1001.03,1224.03,84.5822))
    guide.present=False
    w.emit(w.manifest,64,invoker=w.player); assert w.stage==7
    guide.present=True # A missing guide must not consume a one-shot click.
    w.emit(w.manifest,64,invoker=w.player); w.emit(w.manifest,64,invoker=w.player)
    w.text(guide); assert w.stage==8 and guide.path==10995602
    w.player.pos=w.entrance.pos
    w.emit(w.entrance,10,invoker=w.player); assert w.stage==9
    w.teleports.clear() # Model a lost/delayed first cast; stage-9 entry retries it.
    w.emit(w.entrance,10,invoker=w.player); assert w.teleports==[219338]
    w.player.pos=w.final.pos; w.emit(w.final,10,invoker=w.player); w.text(w.final)
    assert w.boss.flags & 770
    w.waypoint(w.final,10995603,19,(-139.254,-4529.96,170.192))
    assert w.boss.flags & 770 # Guide still outside the old callback range.
    w.text(w.final)
    w.waypoint(w.final,10995603,20,(-109.606,-4529.61,170.192))
    assert not w.boss.flags & 770
    w.kill(w.boss,w.pet); w.text(w.final); w.text(w.final)
    assert w.stage==10 and w.credits==expected and 109836 in w.objectives
    assert len([a for a in w.actors if a.entry==110800])==1
    print('PASS:',team,'all ten stage transitions, pet kills, locked manifest, portal creation')

# Shared helper isolation, actual return spell and Calydus objective routing.
assert set(scripts[110021,0])=={0}
assert scripts[-370846,0][1]['a2']==52900
assert next(r for r in data['go'] if r['entry']==251699)['spell']==218595
assert scripts[251699,1][0]['action']==204 and scripts[251699,1][0]['a1']==1155
assert not any(c['type']==6 for c in data['condition'])
assert {(r['entry'],r['spell'],r['flags']) for r in data['spellclick']}=={
    (109964,219203,1),(110030,219203,1),(110800,220374,1)}
destinations={r['spell']:r for r in data['destination']}
assert [destinations[s]['map'] for s in (218595,219338,220374)]==[1630,1630,1220]
assert scripts[109838,0][0]['action']==33 and scripts[109838,0][0]['a1']==109837
assert data['questender']==[{'entry':109838,'quest':43153}]
assert any(r['SpellID']==220374 and r['Effect']==252 for r in native['spell_effects'])
assert any(r['SpellID']==219203 and r['ImplicitTarget1']==25 for r in native['spell_effects'])
faction=next(r for r in native['factions'] if r['ID']==1665)
assert faction['FriendGroup'] & 2 and faction['FriendGroup'] & 4 and not faction['EnemyGroup'] & 6
for pos,stage in (((-974,1042,84.58),3),((-973,1053,84.58),4)):
    w=Run('Horde'); w.stage=3; w.player.pos=pos
    eo=Actor(60,(-973.112,1047.51,84.5811),13)
    w.emit(eo,91,invoker=w.player)
    assert w.stage==stage, 'entering the alcove must not resolve the leave choice'
    if stage==4:
        w.emit(eo,91,invoker=w.player); assert w.stage==4
print('PASS: migration twice, unique IDs, all link destinations, shared-helper isolation, entry selector')
