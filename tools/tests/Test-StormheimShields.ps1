param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimShieldsData.ps1"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-shields"
& "$PSScriptRoot/Test-StormheimRunesBind.ps1" *> "$out/upstream.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-runes-bind/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$out/UnworthyFixture.inc" -Raw
$f=$f.Replace('struct FakeMap;','struct FakeMap;FakeMap* ShieldsMap();struct GameObjectAI;')
$f=$f.Replace('struct WorldObject {','struct WorldObject {virtual uint32 GetTypeId(){return 5;}uint32 GetInstanceId(){return 0;}')
$f=$f.Replace('uint32 GetDBTableGUIDLow(){return 25164548;}','uint64 dbGuid=0;uint64 GetDBTableGUIDLow(){return dbGuid;}float GetInteractionDistance(){return 5;}GameObjectAI* scripted=nullptr;GameObjectAI* AI(){return scripted;}')
$f=$f.Replace('FakeMap* GetMap()const{static FakeMap m;return &m;}','FakeMap* GetMap()const{return ShieldsMap();}')
$f=$f.Replace('struct Player:Unit {','struct Player:Unit {uint32 GetGossipTextId(uint32,WorldObject*);')
$f=$f.Replace('struct GameObjectAI {','struct GameObjectAI {GameObject* go;explicit GameObjectAI(GameObject* g=nullptr):go(g){}')
[IO.File]::WriteAllText("$out/UnworthyFixture.inc",$f)
$prefix=Get-Content "$out/RunesBindFixture.inc" -Raw
$prefix=[regex]::Replace($prefix,'\bCondition\(','CageCondition(')
$prefix=$prefix.Replace('FakeMap* GameObject::GetMap(){static FakeMap map;return &map;}','FakeMap* ShieldsMap(){static FakeMap map;return &map;}FakeMap* GameObject::GetMap(){return ShieldsMap();}')
$prefix=$prefix.Replace('uint32 spellId=0;Unit* spellCaster=user;','if(auto p=user->ToPlayer())if(AI()&&AI()->GossipUse(p))return;uint32 spellId=0;Unit* spellCaster=user;')
[IO.File]::WriteAllText("$out/ShieldsFixture.inc",$prefix)
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_skold_ashil.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$out/ShieldsProduction.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_skold_ashil()')))
$native=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$native.IndexOf('uint32 Player::GetGossipTextId(uint32');$b=$native.IndexOf('uint32 Player::GetDefaultGossipMenuForSource(',$a)
[IO.File]::WriteAllText("$out/ShieldsText.inc",$native.Substring($a,$b-$a))
$native=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$a=$native.IndexOf('bool ConditionMgr::IsObjectMeetToConditionList(');$b=$native.IndexOf('bool ConditionMgr::CanHaveSourceGroupSet(',$a)
[IO.File]::WriteAllText("$out/ShieldsConditionGroups.inc",$native.Substring($a,$b-$a))
$a=$native.IndexOf('        case CONDITION_OBJECT_ENTRY:');$b=$native.IndexOf('        case CONDITION_TYPE_MASK:',$a)
[IO.File]::WriteAllText("$out/ShieldsObjectCondition.inc",$native.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimShieldsTest.cpp`" /Fe`"$out/Shields.exe`" /Fo`"$out/Shields.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Shields.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Shield reading production regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$text=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 foreach($marker in @('go_stormheim_engraved_shield','Skold-Ashil: distinct shield readings')){if(!$text.Contains($marker)){throw "Canonical Release lacks $marker"}}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj";$r=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 $relative=$r.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'scripts\\Release\\scripts.lib$'} | Select-Object -First 1
 if(!$relative){throw 'Missing actual scripts final-link input'}
 $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
 if((Get-Item "$repo/src/server/scripts/Legion/stormheim_skold_ashil.cpp").LastWriteTimeUtc -gt (Get-Item $lib).LastWriteTimeUtc -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical Release predates shield implementation'}
 'PASS: canonical Release shield implementation diagnostic and actual final-link library freshness.'
}
