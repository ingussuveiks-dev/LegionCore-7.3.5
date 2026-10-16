param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimDrowningValkyraData.ps1"
& "$PSScriptRoot/Test-StormheimHelheimDisguise.ps1" *> "$PSScriptRoot/../../.codex/drowning-valkyra-upstream-test.log"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-drowning-valkyra"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$previous="$repo/build-extractors/runtime-test/stormheim-helheim-disguise"
Get-ChildItem "$previous/*.inc" | Copy-Item -Destination $output
$fixture=Get-Content "$previous/HelheimDisguiseFixture.inc" -Raw
$fixture=$fixture.Replace('struct Creature:Unit {','struct Creature:Unit {bool ShouldHideFor(ObjectGuid g){return hidden.count(g);}void GetPlayerListInGrid(std::list<Player*>&,float);')
$fixture=$fixture.Replace('struct Player:Unit {','struct Player:Unit {uint32 visibilityUpdates=0;void UpdateVisibilityOf(WorldObject*){++visibilityUpdates;}')
$fixture=$fixture.Replace('uint32 GetEntry(){return entry;}','uint32 GetEntry()const{return entry;}')
$fixture=$fixture.Replace('bool HasFlag(int f,uint32 v){return fields[f]&v;}','bool HasFlag(int f,uint32 v)const{auto i=fields.find(f);return i!=fields.end()&&(i->second&v);}')
$fixture=$fixture.Replace('struct Player:Unit {','struct Player:Unit {bool canSeeSpellClickOn(Creature const*)const;')
$fixture=$fixture.Replace('struct Creature:Unit {','struct Creature:Unit {std::string aiName="SmartAI";std::string GetAIName(){return aiName;}')
[IO.File]::WriteAllText("$output/DrowningValkyraFixture.inc",$fixture)
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_drowning_valkyra.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$output/DrowningValkyra.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_drowning_valkyra()')))
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$core.IndexOf('bool Player::canSeeSpellClickOn(');$b=$core.IndexOf('void Player::SendTalentsInfoData(',$a)
[IO.File]::WriteAllText("$output/NativeClickVisibility.inc",$core.Substring($a,$b-$a))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$a=$unit.IndexOf('                if (creature)',$unit.IndexOf('if (index == UNIT_FIELD_NPC_FLAGS)'))
$b=$unit.IndexOf('                if (appendValue & UNIT_NPC_FLAG_TRAINER)',$a)
[IO.File]::WriteAllText("$output/NativeClickField.inc",$unit.Substring($a,$b-$a))
$smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$a=$smart.IndexOf('bool SmartScript::IsSmart(Creature*');$b=$smart.IndexOf('bool SmartScript::IsSmartGO(',$a)
[IO.File]::WriteAllText("$output/NativeIsSmart.inc",$smart.Substring($a,$b-$a))
$condition=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$a=$condition.IndexOf('                QuestStatus status = player->GetQuestStatus(ConditionValue1);',$condition.IndexOf('case CONDITION_QUESTTAKEN:'))
$b=$condition.IndexOf("`n            }",$a)
[IO.File]::WriteAllText("$output/NativeQuestTaken.inc",$condition.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimDrowningValkyraTest.cpp`" /Fe`"$output/DrowningValkyra.exe`" /Fo`"$output/DrowningValkyra.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/DrowningValkyra.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Drowning Valkyra production regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 foreach($marker in @('npc_stormheim_drowning_valkyra','spell_stormheim_drowning_valkyra','Drowning Valkyra: ignored generic rescue credit','Drowning Valkyra: ignored generic rescue talk credit')){if(!$bytes.Contains($marker)){throw "Canonical Release lacks $marker"}}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 foreach($name in @('scripts','game')){
  $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match "$name\\Release\\$name.lib$"} | Select-Object -First 1
  if(!$relative){throw "Missing actual $name link input"}
  $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
  if(!(Test-Path $lib) -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw "Canonical Release predates $name link input"}
 }
 'PASS: canonical Release DrowningValkyra bindings/credit guards and actual scripts/game final-link inputs.'
}
