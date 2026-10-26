param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned failing arrival cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ashbringer-zerus-arrival"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.cpp'
$source=Get-Content "$repo/$path" -Raw
if($source.Contains('namespace AshbringerAcquisition')){$source=$source.Replace((Block $source 'namespace AshbringerAcquisition'),'');foreach($header in @('GameObjectAI.h','Scenario.h','ScenarioMgr.h')){$source=$source.Replace("#include `"$header`"",'')}}
$pinned=(& git -C $repo show "07bd3831:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned Zerus baseline unavailable'}
$source=$source.Replace((Block $source 'class spell_bs_dominate_mind :'),(Block $pinned 'class spell_bs_dominate_mind :'))
if($Baseline){$source=$pinned}else{
 $restored=$source.Replace('type != POINT_MOTION_TYPE || (id != 1 && id != 2)','type != POINT_MOTION_TYPE')
 if($restored.Contains('bool lethal = damage >= me->GetHealth(attacker);')){$restored=$restored.Replace((Block (Block $restored 'struct npc_bs_balnazzarAI') 'void DamageTakenAfterScaling('),(Block (Block $pinned 'struct npc_bs_balnazzarAI') 'void DamageTaken('))}
 $restored=$restored.Replace((Block $restored 'class go_bs_the_ashbringer :'),(Block $pinned 'class go_bs_the_ashbringer :'))
 if(($restored -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Unrelated change beyond explicit Zerus point1/2 admission'}
}
$boss=Block $source 'struct npc_bs_boss_jailer_zerusAI'
[IO.File]::WriteAllText("$out/ZerusArrival.inc",(Block $boss 'void MovementInform(').Replace(' override',''))
$producer=[regex]::Match((Block $boss 'void UpdateAI('),'(?s)case EVENT_3:(.*?)break;')
if(!$producer.Success){throw 'Missing actual Zerus helper movement request'}
[IO.File]::WriteAllText("$out/ZerusRequest.inc",('void IssueMove(){'+$producer.Groups[1].Value+'}'))
$header=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.h" -Raw
$pinnedHeader=(& git -C $repo show '07bd3831:src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.h') -join "`n"
if($LASTEXITCODE -or ($header -replace '\s','') -cne ($pinnedHeader -replace '\s','')){throw 'Unrelated header/position data changed'}
[IO.File]::WriteAllText("$out/ZerusPositions.inc",(Block $header 'Position const zerusSumPos[4]')+';')
$eventHeader=Get-Content "$repo/src/server/game/AI/EventMap.h" -Raw
$eventSource=Get-Content "$repo/src/server/game/AI/EventMap.cpp" -Raw
[IO.File]::WriteAllText("$out/EventMapClass.inc",(Block $eventHeader 'enum c_events')+';'+(Block $eventHeader 'class TC_GAME_API EventMap').Replace('TC_GAME_API ','')+';')
[IO.File]::WriteAllText("$out/EventMapMethods.inc",((@('void EventMap::Reset(','void EventMap::ScheduleEvent(uint32 eventId, uint32 time','uint32 EventMap::ExecuteEvent(','void EventMap::CancelEvent(')|ForEach-Object {Block $eventSource $_}) -join "`n"))
[IO.File]::WriteAllText("$out/PointArrival.inc",('template<> '+(Block (Get-Content "$repo/src/server/game/Movement/MovementGenerators/PointMovementGenerator.cpp" -Raw) 'void PointMovementGenerator<Creature>::MovementInform')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE /fsanitize=address /Zi'}elseif($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/AshbringerZerusArrivalTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" %*`nexit /b %errorlevel%`n")
if($Baseline){
 & "$out/run.cmd" invalid *> "$out/baseline-invalid.log";if($LASTEXITCODE -or [IO.File]::ReadAllText("$out/baseline-invalid.log") -notmatch 'spuriously beams/summons'){throw 'Old invalid point side effects not reproduced'}
 & "$out/run.cmd" overflow *> "$out/baseline-asan.log";if(!$LASTEXITCODE -or [IO.File]::ReadAllText("$out/baseline-asan.log") -notmatch 'AddressSanitizer: global-buffer-overflow'){throw 'Old point3 overread not reproduced'}
 'PASS pinned07bd3831: invalid POINT8/id0 spurious effects and POINT8/id3 ASAN global-buffer-overflow retained'
 $global:LASTEXITCODE=0
 return
}
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Zerus arrival regression failed'}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $rows=@("SET SESSION TRANSACTION READ ONLY;SELECT 'template',entry,ScriptName,AIName FROM creature_template WHERE entry=91672;SELECT 'spawn',guid,id,map,phaseId FROM creature WHERE id=91672;SELECT 'helper',entry,ScriptName FROM creature_template WHERE entry=91697;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Read-only Zerus query failed'}
}finally{$env:MYSQL_PWD=$previous}
if(Compare-Object @("template`t91672`tnpc_bs_boss_jailer_zerus`t","spawn`t366318`t91672`t1500`t5940","helper`t91697`tnpc_bs_enemy_forces") $rows){throw 'Native Zerus/helper bindings changed'}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile AshbringerZerusCatalog.cpp -ProbeName AshbringerZerusCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"
if(!$? -or $LASTEXITCODE){throw 'Native Zerus catalog failed'}
if(Compare-Object @("SPELL`t192596`tInquisitor Beam","EFFECT`t192596`t0`t6`t4`t0`t18`t8","VISUAL`t192596`t49890`t1","SPELL`t190711`tDrain Soul","EFFECT`t190711`t0`t6`t4`t0`t1`t0","VISUAL`t190711`t49334`t1") (Get-Content "$out/native.tsv")){throw 'Native Zerus effects/visual links changed'}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_ashbringer_zerus_arrival_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed Zerus point admission missing'}}finally{$env:PYTHONPATH=$previous}
}
