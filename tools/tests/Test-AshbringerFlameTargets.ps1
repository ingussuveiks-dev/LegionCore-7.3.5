param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Failing baseline cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ashbringer-flame-targets"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.cpp'
$source=Get-Content "$repo/$path" -Raw
if($source.Contains('namespace AshbringerAcquisition')){$source=$source.Replace((Block $source 'namespace AshbringerAcquisition'),'');foreach($header in @('GameObjectAI.h','Scenario.h','ScenarioMgr.h')){$source=$source.Replace("#include `"$header`"",'')}}
$pinned=(& git -C $repo show "40543732:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned flame baseline unavailable'}
$source=$source.Replace((Block $source 'class spell_bs_dominate_mind :'),(Block $pinned 'class spell_bs_dominate_mind :'))
if($Baseline){$source=$pinned}else{
 $original=Block (Block $pinned 'struct npc_bs_doomguard_incineratorAI') 'void UpdateAI('
 $updated=Block (Block $source 'struct npc_bs_doomguard_incineratorAI') 'void UpdateAI('
 $restored=$source.Replace($updated,$original).Replace('type != POINT_MOTION_TYPE || (id != 1 && id != 2)','type != POINT_MOTION_TYPE')
 if($restored.Contains('bool lethal = damage >= me->GetHealth(attacker);')){$restored=$restored.Replace((Block (Block $restored 'struct npc_bs_balnazzarAI') 'void DamageTakenAfterScaling('),(Block (Block $pinned 'struct npc_bs_balnazzarAI') 'void DamageTaken('))}
 $restored=$restored.Replace((Block $restored 'class go_bs_the_ashbringer :'),(Block $pinned 'class go_bs_the_ashbringer :'))
 if(($restored -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Unrelated Ashbringer behavior changed beyond separately tested Zerus point guard'}
}
[IO.File]::WriteAllText("$out/FlameAI.inc",(Block $source 'struct npc_bs_doomguard_incineratorAI').Replace(' override','')+';')
$header=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.h" -Raw
$pinnedHeader=(& git -C $repo show '40543732:src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.h') -join "`n"
if($LASTEXITCODE -or ($header -replace '\s','') -cne ($pinnedHeader -replace '\s','')){throw 'Unrelated native target/header data changed'}
[IO.File]::WriteAllText("$out/FlameTargets.inc",(Block $header 'Position const flameTarget[36]')+';')
$eventHeader=Get-Content "$repo/src/server/game/AI/EventMap.h" -Raw
$eventSource=Get-Content "$repo/src/server/game/AI/EventMap.cpp" -Raw
[IO.File]::WriteAllText("$out/EventMapClass.inc",(Block $eventHeader 'enum c_events')+';'+(Block $eventHeader 'class TC_GAME_API EventMap').Replace('TC_GAME_API ','')+';')
[IO.File]::WriteAllText("$out/EventMapMethods.inc",((@('void EventMap::Reset(','void EventMap::ScheduleEvent(uint32 eventId, uint32 time','uint32 EventMap::ExecuteEvent(','void EventMap::CancelEvent(')|ForEach-Object {Block $eventSource $_}) -join "`n"))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE /fsanitize=address /Zi'}elseif($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/AshbringerFlameTargetsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
if($Baseline){
 & "$out/run.cmd" *> "$out/baseline-asan.log"
 if(!$LASTEXITCODE -or [IO.File]::ReadAllText("$out/baseline-asan.log") -notmatch 'AddressSanitizer: global-buffer-overflow'){throw 'Pinned13th tick did not reproduce the actual global target-array overread'}
 'PASS pinned40543732: actual95962 AI/native EventMap reads beyond36 targets on13th1000ms tick; ASAN global-buffer-overflow retained'
 $global:LASTEXITCODE=0
 return
}
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native flame-target regression failed'}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $rows=@("SET SESSION TRANSACTION READ ONLY;SELECT 'template',entry,ScriptName,AIName FROM creature_template WHERE entry=95962;SELECT 'spawn-count',COUNT(*) FROM creature WHERE id=95962;SELECT 'addon-count',COUNT(*) FROM creature_template_addon WHERE entry=95962;SELECT 'smart-count',COUNT(*) FROM smart_scripts WHERE entryorguid=95962;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Read-only flame actor query failed'}
}finally{$env:MYSQL_PWD=$previous}
if(Compare-Object @("template`t95962`tnpc_bs_doomguard_incinerator`t","spawn-count`t0","addon-count`t0","smart-count`t0") $rows){throw 'Native summoned flame actor binding changed'}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile AshbringerFlameCatalog.cpp -ProbeName AshbringerFlameCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"
if(!$? -or $LASTEXITCODE){throw 'Native flame catalog failed'}
if(Compare-Object @("SPELL`t190666`tFel Firebomb","EFFECT`t190666`t0`t32`t190644`t18`t107","VISUAL`t190666`t49310`t1","SPELL`t190644`tFel Firebomb","EFFECT`t190644`t0`t179`t0`t87`t0","VISUAL`t190644`t45999`t1") (Get-Content "$out/native.tsv")){throw 'Native flame effects or visual links changed'}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_ashbringer_flame_targets_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed bounded flame emission missing'}}finally{$env:PYTHONPATH=$previous}
}
