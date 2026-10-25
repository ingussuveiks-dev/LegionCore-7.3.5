param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot certify installed guards'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hruthnir-guards"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "5dd9a8ab:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned guard source unavailable'}}
$first=$source.IndexOf('        std::list<GameObject*> goDoorList;');$last=$source.IndexOf('        void Initialize()',$first)
[IO.File]::WriteAllText("$out/Fields.inc",$source.Substring($first,$last-$first))
foreach($entry in @(@('Initialize','void Initialize()'),@('Create','void OnCreatureCreate('),@('Object','void OnGameObjectCreate('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Block $source $entry[1]).Replace(' override',''))}
$death=Block $source 'void OnUnitDeath('
$first=$death.IndexOf('                case 96455:');$last=$death.IndexOf('                    break;',$first)
if($first -lt 0 -or $last -lt 0){throw 'Native guard death branch missing'}
[IO.File]::WriteAllText("$out/Death.inc",'void OnUnitDeath(Unit* unit){switch(unit->GetEntry()){'+$death.Substring($first,$last-$first)+'break;}}')
[IO.File]::WriteAllText("$out/Advance.inc",$(if($Baseline){'void TryAdvanceMagnarGuards(){}'}else{(Block $source 'void RegisterMagnarGuardDeath(')+(Block $source 'void TryAdvanceMagnarGuards()')}))
if(!$Baseline){
 $loader=Block (Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw) 'bool Creature::LoadCreatureFromDB('
 $generate=$loader.IndexOf('guid = sObjectMgr->GetGenerator<HighGuid::Creature>()->Generate();')
 $restore=$loader.IndexOf('m_respawnTime = GetMap()->GetCreatureRespawnTime(m_DBTableGuid);')
 $dead=$loader.IndexOf('m_deathState = DEAD;',$restore)
 $publish=$loader.IndexOf('GetMap()->AddToMap(this)',$dead)
 if($generate -lt 0 -or $restore -le $generate -or $dead -le $restore -or $publish -le $dead){throw 'Native instance reload/dead-state publication differs'}
 $update=Block $source 'void Update(uint32 diff)'
 if(!$update.Contains('TryAdvanceMagnarGuards();') -or $update.IndexOf('TryAdvanceMagnarGuards();') -gt $update.IndexOf('scenario->GetScenarioId() != 1068') -or !$update.Contains('hunterWindTimer = 500;')){throw 'Guard retry must run on the shared500ms timer before Hunter-only admission'}
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/HruthnirGuardsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Hruthnir guard regression failed'}
if($VerifyReleaseBinary){
 & "$PSScriptRoot/Test-MagnarWaves.ps1" -VerifyReleaseBinary
 if($LASTEXITCODE){throw 'Native stage metadata or downstream Magnar regression failed'}
 $native=Get-Content "$repo/build-extractors/runtime-test/magnar-finale/native.tsv"
 foreach($row in @("EARTH_STEP`t2`t1829`t0`t0`tCrypt Crawl","EARTH_CRITERION`t2`t28984`t92`t46358`t1")){if($native -notcontains $row){throw "Native guard stage differs: $row"}}
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile HruthnirGuardsCatalog.cpp -ProbeName HruthnirGuardsCatalog -OutputFile "$out/native.tsv";if(!$? -or $LASTEXITCODE){throw 'Native guard catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $catalog=Get-Content "$out/native.tsv"
 foreach($row in @("MAP`t1495`t5`t0","DUNGEON`t1028`t1495`t1`t1","SPELL`t57755`tHeroic Throw","SPELL`t191162`tSwirling Storms","SPELL`t191187`tStorm Nova","SPELL`t191149`tSwirling Storms","EFFECT`t57755`t0`t121`t0`t0`t6`t0","EFFECT`t57755`t1`t31`t0`t0`t6`t0")){if($catalog -notcontains $row){throw "Native guard mechanics changed: $row"}}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$connection[3]
  $sql="SELECT 'guard',guid,id,map,PhaseId,position_x,position_y,position_z FROM creature WHERE guid IN(361412,361413,361306);SELECT 'door',guid,id,map,PhaseId FROM gameobject WHERE guid IN(134170,140833);SELECT 'self-death',entryorguid,event_type,event_param1,action_type,target_type FROM smart_scripts WHERE source_type=0 AND entryorguid IN(-361412,-361413) AND event_type=8;"
  $rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Read-only guard metadata failed'}
 }finally{$env:MYSQL_PWD=$previous}
 [IO.File]::WriteAllLines("$out/live.tsv",[string[]]$rows)
 foreach($prefix in @("guard`t361412`t96455`t1495`t6999`t4793.14`t327.93`t-22.51","guard`t361413`t96455`t1495`t6999`t4795`t339.71`t-21.71","guard`t361306`t96468`t1495`t6999`t4884.71`t-77.9149`t60.9946","door`t134170`t243897`t1495`t6999","door`t140833`t248777`t1495`t6985","self-death`t-361412`t8`t57755`t37`t1","self-death`t-361413`t8`t57755`t37`t1")){if($rows -notcontains $prefix){throw "Live guard/door identity changed: $prefix"}}
 $python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& $python "$PSScriptRoot/verify_hruthnir_guards_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed guard policy differs'}}finally{$env:PYTHONPATH=$previous}
}
