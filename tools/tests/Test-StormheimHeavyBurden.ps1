param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimHeavyBurdenData.ps1"
& "$PSScriptRoot/Test-StormheimHelheimBargain.ps1" *> "$PSScriptRoot/../../.codex/burden-upstream.log"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-heavy-burden"
New-Item -ItemType Directory -Force -Path $out | Out-Null
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-helheim-bargain/*.inc" | Copy-Item -Destination $out
$fixture=Get-Content "$out/BargainFixture.inc" -Raw
$fixture=$fixture.Replace('struct Player:Unit {','struct Player:Unit {bool casting=false;bool HasUnitState(int){return casting;}uint32 cooking=0;bool SatisfyQuestSkill(Quest const*,bool)const;')
$fixture=$fixture.Replace('struct Unit:WorldObject {','struct Unit:WorldObject {bool IsPlayer(){return ToPlayer()!=nullptr;}')
$fixture=$fixture.Replace('struct WorldObject:Position {','struct WorldObject:Position {bool IsUnit(){return ToUnit()!=nullptr;}GameObject* ToGameObject(){return nullptr;}void* ToCorpse(){return nullptr;}')
$fixture=$fixture.Replace('struct Quest {','struct Quest {uint32 RequiredSkillId=0,RequiredSkillPoints=0;')
$fixture=$fixture.Replace('uint32 GetSkillValue(int){return 0;}','uint32 GetSkillValue(uint32 skill){return skill==185?cooking:0;}')
$fixture=$fixture.Replace('return SatisfyQuestRace(q,b)&&SatisfyQuestPreviousQuest(q,b);','return SatisfyQuestRace(q,b)&&SatisfyQuestPreviousQuest(q,b)&&SatisfyQuestSkill(q,b);')
$fixture=$fixture.Replace('void SendCanTakeQuestResponse(int,Quest const*,char const* = nullptr){}','void SendCanTakeQuestResponse(int,Quest const*,char const* = nullptr)const{}')
$fixture=$fixture.Replace('struct {uint32 open=1691,questID=38811;} goober;','struct {uint32 open=1691,questID=40078,spell=193563;} goober;')
$fixture=$fixture.Replace('struct SpellInfo {','struct SpellInfo {uint32 MiscValue=98239;')
$fixture=$fixture.Replace('struct GameObject:WorldObject {','struct GameObject:WorldObject {uint32 nativeFallback=0;void Use(Unit*);')
[IO.File]::WriteAllText("$out/BurdenFixture.inc",$fixture)
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$core.IndexOf('bool Player::SatisfyQuestSkill(');$b=$core.IndexOf('bool Player::SatisfyQuestLevel(',$a)
[IO.File]::WriteAllText("$out/NativeQuestSkill.inc",$core.Substring($a,$b-$a))
$effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$a=$effects.IndexOf('void Spell::EffectKillCredit(');$b=$effects.IndexOf('void Spell::EffectQuestFail(',$a)
[IO.File]::WriteAllText("$out/NativeSpellGroupCredit.inc",$effects.Substring($a,$b-$a))
$objects=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$a=$objects.IndexOf('void GameObject::Use(Unit* user)');$b=$objects.IndexOf('    // If cooldown data present',$a)
[IO.File]::WriteAllText("$out/NativeObjectUse.inc",$objects.Substring($a,$b-$a)+"`n    ++nativeFallback;`n}`n")
$spell=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$a=$spell.IndexOf('    if (WorldObject* target = m_targets.GetObjectTarget())',$spell.IndexOf('void Spell::InitExplicitTargets('));$b=$spell.IndexOf("`n    else",$a)
[IO.File]::WriteAllText("$out/NativeExplicitAdmission.inc",$spell.Substring($a,$b-$a))
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_heavy_burden.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$out/HeavyBurden.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_heavy_burden()')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$out`" `"$PSScriptRoot/StormheimHeavyBurdenTest.cpp`" /Fe`"$out/HeavyBurden.exe`" /Fo`"$out/HeavyBurden.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/HeavyBurden.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd)
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Heavy Burden production regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 foreach($marker in @('go_stormheim_heavy_burden','spell_stormheim_heavy_burden','player_stormheim_heavy_burden','A Heavy Burden: ignored generic fish credit','A Heavy Burden: ignored generic fish talk credit')){if(!$bytes.Contains($marker)){throw "Canonical Release lacks $marker"}}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 foreach($name in @('scripts','game')){
  $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match "$name\\Release\\$name.lib$"} | Select-Object -First 1
  if(!$relative){throw "Missing actual $name input"}
  $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
  if(!(Test-Path $lib) -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw "Canonical Release predates $name input"}
 }
 'PASS: canonical Release Heavy Burden registrations/guards and actual scripts/game link inputs.'
}
