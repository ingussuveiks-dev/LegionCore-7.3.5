param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimSunderedData.ps1"
& "$PSScriptRoot/Test-StormheimHelheimDisguise.ps1" *> "$PSScriptRoot/../../.codex/sundered-upstream-test.log"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-sundered"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$previous="$repo/build-extractors/runtime-test/stormheim-helheim-disguise"
Get-ChildItem "$previous/*.inc" | Copy-Item -Destination $output
$fixture=Get-Content "$previous/HelheimDisguiseFixture.inc" -Raw
$fixture=$fixture.Replace('struct Creature:Unit {','struct Creature:Unit {bool ShouldHideFor(ObjectGuid g){return hidden.count(g);}void GetPlayerListInGrid(std::list<Player*>&,float);')
$fixture=$fixture.Replace('void MoveFollow(Unit* p,float,float){','uint32 falls=0;void MoveFall(){++falls;}void MoveFollow(Unit* p,float,float){')
$fixture=$fixture.Replace('struct Creature:Unit {','struct Creature:Unit {bool gravityDisabled=true;void SetDisableGravity(bool v){gravityDisabled=v;}void RemoveAurasDueToSpell(uint32 id){for(auto i=roots.begin();i!=roots.end();)if(i->first.first==id)i=roots.erase(i);else ++i;}')
$fixture=$fixture.Replace('struct Player:Unit {','struct Player:Unit {uint32 visibilityUpdates=0;void UpdateVisibilityOf(WorldObject*){++visibilityUpdates;}')
$fixture="enum class AnimTier {Ground,Fly};`n"+$fixture
$fixture=$fixture.Replace('struct Creature:Unit {','struct Creature:Unit {AnimTier tier=AnimTier::Fly;void SetAnimTier(AnimTier v){tier=v;}')
$fixture=$fixture.Replace('virtual void SetData(uint32,uint32){++delegated;}','virtual void SetGUID(ObjectGuid const&,int32){++delegated;}virtual void SetData(uint32,uint32){++delegated;}')
[IO.File]::WriteAllText("$output/SunderedFixture.inc",$fixture)
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_sundered.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$output/Sundered.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_sundered()')))
$native=Get-Content "$repo/src/server/game/Spells/SpellInfo.cpp" -Raw
$a=$native.IndexOf('SpellCastResult SpellInfo::CheckExplicitTarget(');$b=$native.IndexOf('bool SpellInfo::CheckTargetCreatureType(',$a)
[IO.File]::WriteAllText("$output/NativeExplicitTarget.inc",$native.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimSunderedTest.cpp`" /Fe`"$output/Sundered.exe`" /Fo`"$output/Sundered.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/Sundered.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Sundered production regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 foreach($marker in @('npc_stormheim_sundered','spell_stormheim_sundered','Sundered: ignored generic fragment credit','Sundered: ignored generic fragment talk credit')){if(!$bytes.Contains($marker)){throw "Canonical Release lacks $marker"}}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 foreach($name in @('scripts','game')){
  $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match "$name\\Release\\$name.lib$"} | Select-Object -First 1
  if(!$relative){throw "Missing actual $name link input"}
  $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
  if(!(Test-Path $lib) -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw "Canonical Release predates $name link input"}
 }
 'PASS: canonical Release Sundered bindings/credit guards and actual scripts/game final-link inputs.'
}
