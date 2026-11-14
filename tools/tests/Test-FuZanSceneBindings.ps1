param([string]$OutputDirectory='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/.codex/fuzan-scene-bindings/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"}
$out=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDirectory)
if(!$out.StartsWith($repo+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Keep native scene evidence inside the workspace'}
[void][IO.Directory]::CreateDirectory($out)
$protected=@{}
foreach($relative in @('build-extractors/bin/Release/worldserver.conf','build-extractors/bin/Release/logs/DBErrors.log','build-extractors/runtime-test/legion-legendary-catalog/SlaghammerHandoffNative.exe')){$protected[$relative]=(Get-FileHash -LiteralPath "$repo/$relative").Hash}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($repo,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $prior=$env:LEGION_DIALOGUE_CONFIG
 try {
  $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile FuZanSceneBindingCatalog.cpp -ProbeName FuZanSceneBindingCatalog -OutputFile "$out/catalog.tsv" *> "$out/compiler-run.log"
  if(!$? -or $LASTEXITCODE){throw 'Native scene reader failed; inspect captured output'}
 } finally {$env:LEGION_DIALOGUE_CONFIG=$prior}
 @'
from pathlib import Path
import hashlib,json,re,sys
out=Path(sys.argv[1]);root=Path.cwd()
source=(out/'catalog.tsv').read_text(encoding='utf-8-sig')
assert source.rstrip().endswith('EOF\tFuZanSceneBindingCatalog')
assert 'COMPLETE\t5969\t20\t5969\t1378\t6919\n' in source
assert 'SELECTION\t38\t222\t26\n' in source
blocks={}
for m in re.finditer(r'^(TEXT|PACKAGE_TEXT)\t(\d+)\t([^\n]+)\nSCRIPT_BEGIN\t(\d+)\t(\d+)\n(.*?)\nSCRIPT_END\t(\d+)',source,re.M|re.S):
    kind,sid,name,begin,length,body,end=m.groups();sid=int(sid)
    assert sid==int(begin)==int(end) and sid not in blocks
    assert len(body.encode('utf8'))==int(length),('Incomplete source payload',sid)
    blocks[sid]={'name':name.split('\t')[0],'bytes':int(length),'body':body}
assert len(blocks)==222
scripts={};members={}
for line in source.splitlines():
    fields=line.split('\t')
    if fields[0]=='SCRIPT_SELECTED':
        sid=int(fields[1]);assert sid not in scripts
        scripts[sid]=tuple(map(int,fields[2:4]))
    elif fields[0]=='MEMBER':members.setdefault(int(fields[2]),[]).append(tuple(map(int,fields[3:6])))
assert len(scripts)==222
packages=set();nodes=set()
def script(sid):
    if not sid or sid in nodes:return
    assert sid in scripts and sid in blocks
    nodes.add(sid)
    for linked in scripts[sid]:script(linked)
def package(pid):
    if pid in packages:return
    packages.add(pid)
    assert pid in members
    for sid,child,order in members[pid]:
        script(sid)
        if child:package(child)
package(1668)
expected={41,154,155,1651,1665,9338,9339,10442,10480,10794,11151,11375,11559,12155,14271,14272}
assert packages=={156,1668} and nodes==expected
init,master=blocks[14271]['body'],blocks[14272]['body']
assert 'clone:EquipWeapon(WeaponSlot.MainHand, 128938)' in init
assert 'SendTrigger("Smooth Phase Complete")' in init
assert 'WaitForTrigger("Smooth Phase Complete")' in master and 'scene:EndScene()' in master
assert not re.search(r'\b(?:109441|109820|217818|217819|217820)\b',init+'\n'+master)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
inputs={p:sha(root/p) for p in ['tools/research/FuZanSceneBindingCatalog.cpp','tools/tests/Test-FuZanSceneBindings.ps1']+['build-extractors/bin/Release/dbc/enUS/'+name+'.db2' for name in ('SceneScriptText','SceneScriptGlobalText','SceneScript','SceneScriptPackage','SceneScriptPackageMember')]}
result={'status':'READ_ONLY_CLIENT_SCENE_EVIDENCE_ROUTE_BINDING_UNRESOLVED','native_counts':{'texts':5969,'globals':20,'scripts':5969,'packages':1378,'members':6919},'keyword_matches':38,'expanded_script_nodes':222,'expanded_packages':26,'fuzan_packages':sorted(packages),'fuzan_complete_source_fragments':[dict(id=sid,name=blocks[sid]['name'],bytes=blocks[sid]['bytes'],first=scripts[sid][0],next=scripts[sid][1]) for sid in sorted(nodes)],'inputs':inputs,'catalog_sha256':sha(out/'catalog.tsv'),'limits':'All selected native payload lengths and complete First/Next/child-package closure checked. No authored NPC-to-TaxiPath binding inferred;generic FakeMount/scene-path declarations and unrelated numeric namespaces do not establish server flight. No persistent DB/character/account write or client traversal.'}
(out/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf8')
print('PASS5969 text/20 global rows;222 complete selected payloads;Fu Zan1668/156 all16 linked fragments,including First/Next continuations;no server path binding inferred')
'@ | & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe -X utf8 - $out *> "$out/validation.log"
 if($LASTEXITCODE){throw 'Native scene catalog validation failed'}
 foreach($relative in $protected.Keys){if($protected[$relative] -ne (Get-FileHash -LiteralPath "$repo/$relative").Hash){throw 'Protected configuration/original evidence changed'}}
 Get-Content -LiteralPath "$out/validation.log"
}
$global:LASTEXITCODE=0
