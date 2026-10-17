param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-gift"
New-Item -ItemType Directory -Force -Path $out | Out-Null
& "$PSScriptRoot/Test-StormheimGiftData.ps1"
& "$PSScriptRoot/Test-StormheimShields.ps1" *> "$out/upstream.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-shields/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$out/UnworthyFixture.inc" -Raw
$f=$f.Replace('struct Player:Unit {','struct Player:Unit {void SetQuestUpdate(uint32);void GetZoneAndAreaId(uint32& z,uint32& a){z=zone;a=area;}void UpdateAvailableQuestLines(){}bool bagFull=false;std::map<uint32,uint32> bank;')
$f=$f.Replace('uint32 GetItemCount(uint32 id){return bags[id];}','uint32 GetItemCount(uint32 id,bool withBank=false){return bags[id]+(withBank?bank[id]:0);}')
$f=$f.Replace('int CastSpell(Player*,uint32 a,bool){if(!castResult){auras.insert(a);if(castHook)castHook();}return castResult;}','int CastSpell(Player*,uint32,bool);')
$f=$f.Replace('struct Creature:Unit {','struct Creature:Unit {Unit* owner=nullptr;Unit* GetAnyOwner(){return owner;}')
$f=$f.Replace('uint32 GetInstanceId(){return 0;}','uint32 instance=0;uint32 GetInstanceId(){return instance;}')
$f=$f.Replace('uint32 GetSkillValue(int){return 0;}','bool HasItemCount(uint32 id,uint32 count,bool checkBank){return GetItemCount(id,checkBank)>=count;}uint32 GetSkillValue(int){return 0;}')
[IO.File]::WriteAllText("$out/UnworthyFixture.inc",$f)
$f=Get-Content "$out/ShieldsFixture.inc" -Raw
$f=$f.Replace('struct Item {uint32 GetEntry(){return 0;}};','struct Item {uint32 entry=128287;uint32 GetEntry(){return entry;}};')
[IO.File]::WriteAllText("$out/ShieldsFixture.inc",$f)
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_gift.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$out/GiftProduction.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_gift()')))
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$core.IndexOf('void Player::SetQuestUpdate(');$b=$core.IndexOf('void Player::SendMusic(',$a)
[IO.File]::WriteAllText("$out/NativeQuestUpdate.inc",$core.Substring($a,$b-$a))
$core=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$a=$core.IndexOf('        case CONDITION_QUEST_OBJECTIVE_DONE:');$b=$core.IndexOf('        case CONDITION_INSTANCE_INFO:',$a)
[IO.File]::WriteAllText("$out/NativeGiftCondition.inc",$core.Substring($a,$b-$a))
$a=$core.IndexOf('        case CONDITION_ITEM:');$b=$core.IndexOf('        case CONDITION_ITEM_EQUIPPED:',$a)
[IO.File]::WriteAllText("$out/NativeGiftItemCondition.inc",$core.Substring($a,$b-$a))
$core=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$a=$core.IndexOf('void Spell::SelectImplicitNearbyTargets(');$b=$core.IndexOf('void Spell::SelectImplicitTargetsFromThreadList(',$a)
[IO.File]::WriteAllText("$out/NearbyTargets.inc",$core.Substring($a,$b-$a))
$fixture=Get-Content "$PSScriptRoot/StormheimReformationTest.cpp" -Raw
$a=$fixture.IndexOf('namespace Targeting');$b=$fixture.IndexOf('void Test(char const* path)',$a)
$fixture=$fixture.Substring($a,$b-$a).Replace('Id=192069','Id=189134')
[IO.File]::WriteAllText("$out/GiftTargeting.inc",$fixture+"`n}`n")
$core=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$a=$core.IndexOf('        case SMART_ACTION_CALL_KILLEDMONSTER:');$b=$core.IndexOf('        case SMART_ACTION_SET_INST_DATA:',$a)
[IO.File]::WriteAllText("$out/NativeGiftCredit.inc",$core.Substring($a,$b-$a))
$a=$core.IndexOf('        case SMART_TARGET_OWNER_OR_SUMMONER:');$b=$core.IndexOf('        case SMART_TARGET_THREAT_LIST:',$a)
[IO.File]::WriteAllText("$out/NativeGiftOwner.inc",$core.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimGiftTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Gift production/native regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 foreach($marker in @('Stormheim: Gift detonator refresh and native self destination','go_stormheim_gift_planting','spell_stormheim_gift_detonator','player_stormheim_gift_refresh')){if(!$bytes.Contains($marker)){throw "Canonical Release lacks $marker"}}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $r=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 $relative=$r.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'scripts\\Release\\scripts.lib$'} | Select-Object -First 1
 if(!$relative){throw 'Missing actual scripts final-link input'}
 $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
 if((Get-Item "$repo/src/server/scripts/Legion/stormheim_gift.cpp").LastWriteTimeUtc -gt (Get-Item $lib).LastWriteTimeUtc -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical Release predates Gift implementation'}
 'PASS: canonical Release Gift registrations and actual final-link library freshness.'
}
