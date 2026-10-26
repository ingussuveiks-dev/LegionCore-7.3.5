param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned baseline cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ashbringer-balnazzar-surrender"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.cpp'
$source=Get-Content "$repo/$path" -Raw
if($source.Contains('namespace AshbringerAcquisition')){$source=$source.Replace((Block $source 'namespace AshbringerAcquisition'),'');foreach($header in @('GameObjectAI.h','Scenario.h','ScenarioMgr.h')){$source=$source.Replace("#include `"$header`"",'')}}
$pinned=(& git -C $repo show "803e4734:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned Balnazzar baseline unavailable'}
$source=$source.Replace((Block $source 'class spell_bs_dominate_mind :'),(Block $pinned 'class spell_bs_dominate_mind :'))
$old=Block (Block $pinned 'struct npc_bs_balnazzarAI') 'void DamageTaken('
if($Baseline){$source=$pinned;$handler=$old}else{
 $handler=Block (Block $source 'struct npc_bs_balnazzarAI') 'void DamageTakenAfterScaling('
 $expected=$old.Replace('void DamageTaken(Unit* /*attacker*/,','void DamageTakenAfterScaling(Unit* attacker,').Replace('if (damage >= me->GetHealth())',"bool lethal = damage >= me->GetHealth(attacker);`n            if (lethal)").Replace('if (me->HealthBelowPct(15) && !fightEnd)','if ((lethal || me->HealthBelowPct(15)) && !fightEnd)')
 $restored=$source.Replace($handler,$old).Replace((Block $source 'class go_bs_the_ashbringer :'),(Block $pinned 'class go_bs_the_ashbringer :'))
 if(($handler -replace '\s','') -cne ($expected -replace '\s','') -or ($restored -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Balnazzar changed beyond final scaled lethal surrender and separately tested pickup callback; original side effects must remain'}
}
[IO.File]::WriteAllText("$out/Surrender.inc",$handler+(Block (Block $source 'struct npc_bs_balnazzarAI') 'void Reset('))
$header=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.h" -Raw
[IO.File]::WriteAllText("$out/Constants.inc",(Block $header 'enum Data')+';'+(Block $source 'enum eSpells')+';')
$core=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$first=$core.IndexOf('    damage /= victim->GetHealthMultiplierForTarget(this);')
$bridge=Block $core.Substring($first) '    if (Creature* creature = victim->ToCreature())'
if($first -lt 0 -or $core.IndexOf('    if (health <= damage)',$first) -lt $first+$bridge.Length){throw 'Native final protection/death order changed'}
[IO.File]::WriteAllText("$out/ScaledHook.inc",'    damage /= victim->GetHealthMultiplierForTarget(this);'+$bridge)
[IO.File]::WriteAllText("$out/InstanceCredit.inc",(Block (Get-Content "$repo/src/server/game/Instances/InstanceScript.cpp" -Raw) 'void InstanceScript::DoUpdateAchievementCriteria(').Replace('CriteriaTypes type','uint32 type'))
$eventHeader=Get-Content "$repo/src/server/game/AI/EventMap.h" -Raw
$eventSource=Get-Content "$repo/src/server/game/AI/EventMap.cpp" -Raw
[IO.File]::WriteAllText("$out/EventMapClass.inc",(Block $eventHeader 'enum c_events')+';'+(Block $eventHeader 'class TC_GAME_API EventMap').Replace('TC_GAME_API ','')+';')
[IO.File]::WriteAllText("$out/EventMapMethods.inc",((@('void EventMap::Reset(','void EventMap::ScheduleEvent(uint32 eventId, uint32 time','uint32 EventMap::ExecuteEvent(','void EventMap::CancelEvent(')|ForEach-Object {Block $eventSource $_}) -join "`n"))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=$(if($Baseline){'/DBASELINE '}else{''})+$(if($AddressSanitizer){'/fsanitize=address /Zi'}else{''})
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/AshbringerBalnazzarSurrenderTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Balnazzar surrender regression failed'}
if($Baseline){return}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $rows=@("SET SESSION TRANSACTION READ ONLY;SELECT 'binding',entry,ScriptName,AIName FROM creature_template WHERE ScriptName='npc_bs_balnazzar';SELECT 'spawn',guid,id,map,phaseId FROM creature WHERE id=90981;SELECT 'scaling',entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE entry=90981;SELECT 'scenario',ScenarioID,MapID,Class,Team FROM scenario_data WHERE MapID=1500;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Read-only Balnazzar query failed'}
}finally{$env:MYSQL_PWD=$previous}
if(Compare-Object @("binding`t90981`tnpc_bs_balnazzar`t","spawn`t366331`t90981`t1500`t5942","scaling`t90981`t98`t110","scenario`t775`t1500`t0`t0","scenario`t933`t1500`t0`t0","scenario`t961`t1500`t0`t0") $rows){throw 'Native Balnazzar/shared-map bindings changed'}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile AshbringerBalnazzarCatalog.cpp -ProbeName AshbringerBalnazzarCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"
if(!$? -or $LASTEXITCODE){throw 'Native Balnazzar catalog failed'}
if(Compare-Object @("SPELL`t227853`tKill Credit","EFFECT`t227853`t0`t90`t0`t90981`t0`t18`t8","SPELL`t152821`tInvisible Man Transform","EFFECT`t152821`t0`t6`t56`t17286`t0`t1`t0","KIT`t61036`t1","STEP`t1853`t45269","CRITERION`t45270`t28902`t92`t46149") (Get-Content "$out/native.tsv")){throw 'Native775 finale/credit/morph/kit metadata changed'}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_ashbringer_balnazzar_surrender_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed Balnazzar surrender missing'}}finally{$env:PYTHONPATH=$previous}
}
