param([switch]$Baseline,[switch]$CreditBaseline,[switch]$StartBaseline,[switch]$ResetBaseline,[switch]$RetryBaseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if(($Baseline -or $CreditBaseline -or $StartBaseline -or $ResetBaseline -or $RetryBaseline) -and $VerifyReleaseBinary){throw 'Baseline cannot certify deployed waves'}
if(@($Baseline,$CreditBaseline,$StartBaseline,$ResetBaseline,$RetryBaseline | Where-Object {$_}).Count -gt 1){throw 'Choose only one pinned baseline'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/magnar-waves"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/EarthWarder.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "6e6d1715:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned wave source unavailable'}}
if($CreditBaseline){$source=(& git -C $repo show "c0f8efce:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned credit source unavailable'}}
if($StartBaseline){$source=(& git -C $repo show "5d5a1f24:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned start source unavailable'}}
if($ResetBaseline){$source=(& git -C $repo show "6eb208cf:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned reset source unavailable'}}
if($RetryBaseline){$source=(& git -C $repo show "f91fbfc7:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned final-duel retry source unavailable'}}
[IO.File]::WriteAllText("$out/Positions.inc",(Block $source 'Position const trashPos[12]')+';')
$source=$source.Substring($source.IndexOf('struct npc_magnar_icebreakerAI :'))
$first=$source.IndexOf('        SummonList summons;');$last=$source.IndexOf('        void Reset()',$first)
[IO.File]::WriteAllText("$out/Fields.inc",$source.Substring($first,$last-$first))
[IO.File]::WriteAllText("$out/Handoff.inc",$(if($source.Contains('void UpdateFinale(')){Block $source 'void UpdateFinale('}else{''}))
foreach($entry in @(@('Reset','void Reset()'),@('EnterCombat','void EnterCombat('),@('JustSummoned','void JustSummoned('),@('Death','void SummonedCreatureDies('),@('Start','void DoAction('),@('Update','void UpdateAI('),@('Launch','void LaunchEventWave('),@('Summon','void SummonTrash('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Block $source $entry[1]))}
[IO.File]::WriteAllText("$out/Advance.inc",$(if($Baseline){''}else{Block $source 'void AdvanceWaveIfReady('}))
[IO.File]::WriteAllText("$out/Eligible.inc",$(if($Baseline -or $CreditBaseline){''}else{(Block $source 'bool IsEncounterPlayer(')+(Block $source 'Player* GetEncounterPlayer(')}))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
[IO.File]::WriteAllText("$out/OwnerPlayer.inc",(Block $source 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'))
$first=$source.IndexOf('        if (TempSummon* summon = creature->ToTempSummon())',$source.IndexOf('void Unit::Kill('));$last=$source.IndexOf('        // Call instance script.',$first)
if($first -lt 0 -or $last -le $first){throw 'Missing native kill/summoner dispatch'}
[IO.File]::WriteAllText("$out/NativeDeath.inc",$source.Substring($first,$last-$first))
$source=Get-Content "$repo/src/server/game/AI/ScriptedAI/ScriptedCreature.cpp" -Raw
[IO.File]::WriteAllText("$out/DespawnAll.inc",(Block $source 'void SummonList::DespawnAll()').Replace('SummonList::',''))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw
[IO.File]::WriteAllText("$out/UnitTypeMask.inc",(Block $source 'enum UnitTypeMask')+';')
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}elseif($CreditBaseline){'/DCREDIT_BASELINE'}elseif($StartBaseline){'/DSTART_BASELINE'}elseif($ResetBaseline){'/DRESET_BASELINE'}elseif($RetryBaseline){'/DRETRY_BASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/MagnarWavesTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Magnar wave regression failed'}
if($VerifyReleaseBinary){
 & "$PSScriptRoot/Test-MagnarFinale.ps1" -VerifyReleaseBinary
 if($LASTEXITCODE){throw 'Scaled finale or live metadata regression failed'}
 $native=Get-Content "$repo/build-extractors/runtime-test/magnar-finale/native.tsv"
 foreach($expected in @("EARTH_STEP`t3`t1830`t0`t0`tThe Confrontation","EARTH_STEP`t4`t1869`t0`t0`tThe Greatest Warrior")){if($native -notcontains $expected){throw "Native start/finale stage differs: $expected"}}
 foreach($expected in @("EFFECT`t191032`t0`t6`t106`t0`t0`t1`t0","DURATION`t191821`t21`t-1`t-1`t0","EFFECT`t191821`t0`t6`t4`t0`t0`t1`t0","CAST`t191038`t19`t2500`t2500`t0`t0")){if($native -notcontains $expected){throw "Native wave spell metadata differs: $expected"}}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$connection[3]
  $rows=@(@'
SELECT 'actor',t.entry,w.Name1,t.AIName,t.ScriptName FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry IN(97123,94707,94813);
SELECT 'sai',entryorguid,id,event_type,action_type,action_param1,target_type FROM smart_scripts WHERE source_type=0 AND entryorguid IN(97123,94707,94813);
SELECT 'addon',entry,auras FROM creature_template_addon WHERE entry IN(97123,94707,94813);
SELECT 'dialogue-line',id,idx,textId,unk2 FROM conversation_data WHERE entry=498 ORDER BY idx;
SELECT 'dialogue-actor',id,creatureId,duration FROM conversation_creature WHERE entry=498 ORDER BY id;
SELECT 'dialogue-models',COUNT(*) FROM conversation_actor WHERE entry=498;
SELECT 'start-binding',entry,AIName,ScriptName FROM creature_template WHERE entry=96468;
SELECT 'start-spawn',guid,id,map,phaseId FROM creature WHERE id=96468 AND map=1495;
SELECT 'start-smart',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid IN(96468,-361306);
'@|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Live wave actor metadata failed'}
 }finally{$env:MYSQL_PWD=$previous}
 foreach($expected in @("actor`t94707`tIcebreaker Tombguard`t`t","actor`t94813`tSpectral Windshaper`tSmartAI`t","actor`t97123`tIcebreaker Champion`t`t","sai`t94813`t0`t0`t11`t191038`t2","addon`t94707`t165114 191821","addon`t97123`t165114 191821")){if($rows -notcontains $expected){throw "Live wave actor differs: $expected"}}
 foreach($expected in @("dialogue-line`t1335`t0`t0`t0","dialogue-line`t1336`t1`t5646`t1","dialogue-line`t1337`t2`t19930`t0","dialogue-actor`t0`t96034`t26000","dialogue-actor`t1`t96468`t4300","dialogue-models`t0")){if($rows -notcontains $expected){throw "Live498 dialogue differs: $expected"}}
 foreach($expected in @("start-binding`t96468`t`tnpc_hrutnir","start-spawn`t361306`t96468`t1495`t6999","start-smart`t0")){if($rows -notcontains $expected){throw "Native start companion differs: $expected"}}
 if($rows.Count -ne 15){throw 'Unexpected wave actor/script/addon/dialogue/start rows'}
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsCommunionCatalog.cpp -ProbeName WarswordsCommunionCatalog -OutputFile "$out/dialogue.tsv" *> "$out/dialogue.log";if(!$? -or $LASTEXITCODE){throw 'Native498 dialogue catalogue failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $dialogue=Get-Content "$out/dialogue.tsv"
 foreach($expected in @("LINE`t1335`t98333`t0`t500`t1336`t0`t1`t60`t60","LINE`t1336`t98337`t0`t500`t1337`t0`t0`t60`t60","LINE`t1337`t98334`t0`t500`t0`t0`t1`t60`t60","SOUND`t98333`t60415`t1370070","SOUND`t98337`t60416`t1370071","SOUND`t98334`t60417`t1370072")){if($dialogue -notcontains $expected){throw "Native498 line/voice reference differs: $expected"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_magnar_waves_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical wave implementation missing'}}finally{$env:PYTHONPATH=$previous}
}
