param([switch]$Baseline,[switch]$VerifyReleaseBinary,[switch]$FirstBoss)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot certify repaired Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-spectre-rush"
if($FirstBoss){$out+='-first'}
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Warrior/Warswords/Warswords.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$revision=if($FirstBoss){'07d1d27d'}else{'924618e7'};$source=(& git -C $repo show "${revision}:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned Spectre Rush baseline'}}
if($FirstBoss){$first=$source.IndexOf('class npc_vigfus_bladewind :');$last=$source.IndexOf('class npc_vigfus_bladewind_last :',$first);$source=$source.Substring($first,$last-$first)}else{$source=$source.Substring($source.IndexOf('class npc_vigfus_bladewind_last :'))}
$first=$source.IndexOf('        EventMap events;');$last=$source.IndexOf('        void Reset()',$first)
[IO.File]::WriteAllText("$out/Fields.inc",$source.Substring($first,$last-$first))
foreach($name in @('Reset','JustSummoned','SummonedCreatureDies','JustDied','UpdateAI')){
 $body=if($FirstBoss -and $Baseline -and $name -eq 'JustDied'){'void JustDied(Unit*) override { }'}else{Block $source "void $name("}
 [IO.File]::WriteAllText("$out/$name.inc",$body)
}
$handoff=''
if($FirstBoss -and !$Baseline){$first=$source.IndexOf('            phase = true;',$source.IndexOf('void DamageTakenAfterScaling('));$last=$source.IndexOf('            Talk(5);',$first);if($first -lt 0 -or $last -le $first){throw 'First retreat handoff missing'};$handoff=$source.Substring($first,$last-$first)}
[IO.File]::WriteAllText("$out/RetreatHandoff.inc",$handoff)
foreach($name in @('SummonedCreatureDespawn','UpdateSpectreRush','BeginSpectreRush')){
 $body=if(!$Baseline){Block $source "void $name("}elseif($name -eq 'BeginSpectreRush'){
  $first=$source.IndexOf('case EVENT_3:',$source.IndexOf('void UpdateAI('))+'case EVENT_3:'.Length
  $last=$source.IndexOf('break;',$first)
  "void BeginSpectreRush() {"+$source.Substring($first,$last-$first)+'}'
 }elseif($name -eq 'UpdateSpectreRush'){'void UpdateSpectreRush(uint32) { }'}else{'void SummonedCreatureDespawn(Creature*) override { }'}
 [IO.File]::WriteAllText("$out/$name.inc",$body)
}
$source=Get-Content "$repo/src/server/game/Entities/Creature/TemporarySummon.cpp" -Raw
[IO.File]::WriteAllText("$out/InitSummon.inc",(Block $source 'void TempSummon::InitSummon()'))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$first=$source.IndexOf('if (TempSummon* summon = creature->ToTempSummon())',$source.IndexOf('void Unit::Kill('))
$last=$source.IndexOf('// Call instance script.',$first)
if($first -lt 0 -or $last -le $first){throw 'Native owner death dispatch missing'}
[IO.File]::WriteAllText("$out/DeathDispatch.inc",$source.Substring($first,$last-$first))
$source=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.cpp" -Raw
[IO.File]::WriteAllText("$out/GetCreature.inc",(Block $source 'Creature* ObjectAccessor::GetCreature('))
$source=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw
[IO.File]::WriteAllText("$out/ObjectInMap.inc",(Block $source 'template<class T> static T* GetObjectInMap('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
if($FirstBoss){$definition+=' /DFIRST'}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/WarswordsSpectreRushTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Spectre Rush native regression failed'}
if($VerifyReleaseBinary){
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsCreditCatalog.cpp -ProbeName WarswordsCreditCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native Spectre Rush catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $native=Get-Content "$out/native.tsv"
 foreach($expected in @("CAST`t195996`t1`t0`t0`t0`t0","CAST`t197852`t1`t0`t0`t0`t0","CAST`t195986`t1`t0`t0`t0`t0","CAST`t197853`t1`t0`t0`t0`t0","EFFECT`t197852`t0`t6`t23`t0`t195986`t1`t0","EFFECT`t195986`t0`t6`t23`t0`t197853`t1`t0","EFFECT`t197853`t0`t31`t0`t0`t0`t22`t15")){if($native -notcontains $expected){throw "Native Spectre Rush spell contract changed: $expected"}}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 $query=if($FirstBoss){"SELECT entry,ScriptName FROM creature_template WHERE ScriptName='npc_vigfus_bladewind';SELECT id,map FROM creature WHERE id=98007;"}else{"SELECT entry,ScriptName FROM creature_template WHERE ScriptName='npc_vigfus_bladewind_last';SELECT guid,id,map,PhaseId FROM creature WHERE id=98602;"}
 try{$env:MYSQL_PWD=$connection[3];$rows=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Spectre Rush binding read failed'}}finally{$env:MYSQL_PWD=$previous}
 $expectedRows=if($FirstBoss){@("98007`tnpc_vigfus_bladewind","98007`t1511")}else{@("98602`tnpc_vigfus_bladewind_last","361283`t98602`t1511`t7003")}
 if($rows.Count -ne 2 -or $rows -notcontains $expectedRows[0] -or $rows -notcontains $expectedRows[1]){throw 'Boss binding/spawn changed; review shared uses'}
 $previous=$env:PYTHONPATH
 $verifier=if($FirstBoss){'verify_warswords_first_spectre_rush_release.py'}else{'verify_warswords_spectre_rush_release.py'}
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/$verifier" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Spectre Rush implementation missing'}}finally{$env:PYTHONPATH=$previous}
}
