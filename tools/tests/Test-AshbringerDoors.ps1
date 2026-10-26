param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned failing baseline cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ashbringer-doors"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/instance_broken_shore.cpp'
$source=Get-Content "$repo/$path" -Raw
$pinned=(& git -C $repo show "01898636:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned door baseline unavailable'}
if($Baseline){$source=$pinned}else{
 $restored=$source.Replace('GuidSet DoorGUIDconteiner;','std::list<GameObject*> DoorGUIDconteiner;').Replace('DoorGUIDconteiner.insert(go->GetGUID());','DoorGUIDconteiner.push_back(go);').Replace((Block $source 'void OnGameObjectRemove('),'')
 foreach($method in @('void onScenarionNextStep(','void CreatureDies(')){$restored=$restored.Replace((Block $source $method),(Block $pinned $method))}
 if(($restored -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Instance changed beyond door GUID ownership/callbacks'}
 $death=Block $source 'void CreatureDies('
 $oldDeath=Block $pinned 'void CreatureDies('
 $loop=Block $death 'for (ObjectGuid const& guid : doors)'
 $oldLoop=Block $oldDeath 'for (auto go : DoorGUIDconteiner)'
 $restoredDeath=$death.Replace($loop,$oldLoop).Replace('GuidSet doors = DoorGUIDconteiner;','') -replace 'case NPC_JAILER_ZERUS_BOSS:\s*\{','case NPC_JAILER_ZERUS_BOSS:' -replace 'break;\s*\}\s*\}\s*\}$',"break; } }"
 if(($restoredDeath -replace '\s','') -cne ($oldDeath -replace '\s','')){throw 'Death progression changed beyond door loop/snapshot scope'}
}
$methods=@('void Initialize(','void OnGameObjectCreate(','void onScenarionNextStep(','void CreatureDies(')
if(!$Baseline){$methods+='void OnGameObjectRemove('}
[IO.File]::WriteAllText("$out/DoorMethods.inc",(($methods|ForEach-Object{(Block $source $_).Replace(' override','')}) -join "`n"))
$header=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.h" -Raw
[IO.File]::WriteAllText("$out/DoorConstants.inc",((@('enum Data','enum eCreatures','enum eGameObects')|ForEach-Object{(Block $header $_)+';'}) -join "`n"))
$accessor=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw
[IO.File]::WriteAllText("$out/NativeFind.inc",(Block $accessor 'static T* Find(ObjectGuid guid)').Replace('static T*','static Object*'))
[IO.File]::WriteAllText("$out/LookupWorld.inc",(Block $accessor 'template<class T> static T* GetObjectInWorld(ObjectGuid guid, T*'))
[IO.File]::WriteAllText("$out/LookupMap.inc",(Block $accessor 'template<class T> static T* GetObjectInMap(ObjectGuid guid, Map* map, T*'))
$core=Get-Content "$repo/src/server/game/Instances/InstanceScript.cpp" -Raw
$map=Get-Content "$repo/src/server/game/Maps/Map.cpp" -Raw
$go=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
[IO.File]::WriteAllText("$out/NativeDoorMethods.inc",((@((Block $core 'void InstanceScript::HandleGameObject('),(Block $core 'void InstanceScript::OnGameObjectRemove('),(Block $map 'GameObject* Map::GetGameObject('),(Block $go 'void GameObject::RemoveFromWorld('))) -join "`n"))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE /fsanitize=address /Zi'}elseif($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
[IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/AshbringerDoorsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nexit /b %errorlevel%`n")
& "$out/build.cmd";if($LASTEXITCODE){throw 'Native door fixture build failed'}
if(!(Test-Path "$out/test.exe")){throw 'Compiled door fixture is missing; inspect local security/launch diagnostics, do not bypass protection'}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`n`"$out/test.exe`" %*`nexit /b %errorlevel%`n")
if($Baseline){
 foreach($path in @('stage','death')){
  & "$out/run.cmd" $path *> "$out/baseline-$path-asan.log"
  if(!$LASTEXITCODE -or [IO.File]::ReadAllText("$out/baseline-$path-asan.log") -notmatch 'AddressSanitizer: heap-use-after-free'){throw "Pinned $path baseline did not reproduce expected native stale-pointer access"}
 }
 'PASS pinned01898636: actual remove/destruction followed by stage3 and Zerus death both hit AddressSanitizer heap-use-after-free; diagnostics retained'
 $global:LASTEXITCODE=0
 return
}
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native door lifetime regression failed'}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@("SET SESSION TRANSACTION READ ONLY;SELECT map,script FROM instance_template WHERE map=1500;SELECT guid,id,map,phaseId FROM gameobject WHERE guid IN (128095,128111,128116) ORDER BY guid;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Read-only door binding query failed'}}finally{$env:MYSQL_PWD=$previous}
if(Compare-Object @("1500`tinstance_broken_shore","128095`t243715`t1500`t5171 5940 5942 6181","128111`t243816`t1500`t5171 5940 5942 6181","128116`t245962`t1500`t5171 5940 5942 6181") $rows){throw 'Native door bindings changed'}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile AshbringerDoorCatalog.cpp -ProbeName AshbringerDoorCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"
if(!$? -or $LASTEXITCODE){throw 'Native775 catalog failed'}
$expectedNative=@(
 "STEP`t1`t1485`t42486","STEP`t2`t1486`t45098","STEP`t3`t1487`t45145","STEP`t0`t1488`t42545",
 "STEP`t5`t1853`t45269","STEP`t6`t1854`t45277","STEP`t4`t2632`t49026",
 "CRITERION`t42487`t42486`t80`t30968`t92`t52845","CRITERION`t42546`t42545`t1`t27508`t92`t46018",
 "CRITERION`t45099`t45098`t1`t28782`t92`t46064","CRITERION`t45146`t45145`t1`t28823`t92`t46130",
 "CRITERION`t45270`t45269`t1`t28902`t92`t46149","CRITERION`t45278`t45277`t1`t28906`t92`t46162",
 "CRITERION`t45792`t45145`t1`t29022`t92`t46506","CRITERION`t49027`t49026`t1`t30652`t92`t52185",
 "CRITERION`t49028`t45145`t1`t30653`t92`t52186","CRITERION`t49480`t42486`t50`t30969`t92`t52846",
 "CRITERION`t49481`t42486`t35`t30970`t92`t52847","CRITERION`t49482`t42486`t125`t30971`t92`t52858"
)
if(Compare-Object $expectedNative (Get-Content "$out/native.tsv")){throw 'Native775 steps/criteria changed'}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_ashbringer_doors_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed door lifetime guard missing'}}finally{$env:PYTHONPATH=$previous}
}
