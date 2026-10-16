param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimHelheimBargainData.ps1"
& "$PSScriptRoot/Test-StormheimHelheimDisguise.ps1" *> "$PSScriptRoot/../../.codex/bargain-upstream-test.log"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-helheim-bargain"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$previous="$repo/build-extractors/runtime-test/stormheim-helheim-disguise"
Get-ChildItem "$previous/*.inc" | Copy-Item -Destination $output
$fixture=Get-Content "$previous/HelheimDisguiseFixture.inc" -Raw
$fixture=$fixture.Replace('struct Creature:Unit {','struct Creature:Unit {void GetPlayerListInGrid(std::list<Player*>&,float);')
$fixture=$fixture.Replace('struct {uint32 open=1691,questID=38811;} goober;', 'struct {uint32 open=1691,questID=38811;} goober;struct {uint32 gossipID=18790;} questgiver;')
$fixture=$fixture.Replace('virtual bool GossipUse(Player*){return false;}', 'virtual bool GossipSelect(Player*,uint32,uint32){return false;}virtual bool GossipUse(Player*){return false;}')
$fixture=$fixture.Replace('bool GossipHello(Player*,bool)override{++delegated;', 'bool GossipSelect(Player*,uint32,uint32)override{++delegated;return false;}bool GossipHello(Player*,bool)override{++delegated;')
$fixture=$fixture.Replace('bool GossipUse(Player*)override{++delegated;return false;}', 'bool GossipUse(Player*)override;')
[IO.File]::WriteAllText("$output/BargainFixture.inc",$fixture)
$smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
$a=$smart.IndexOf('bool SmartGameObjectAI::GossipUse(');$b=$smart.IndexOf('// Called when a player selects',$a)
[IO.File]::WriteAllText("$output/NativeGossipUse.inc",$smart.Substring($a,$b-$a))
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$core.IndexOf('        else if (GameObject* go = source->ToGameObject())',$core.IndexOf('void Player::PrepareGossipMenu('))
$a=$core.IndexOf('            switch',$a);$b=$core.IndexOf("`n        }",$a)
[IO.File]::WriteAllText("$output/NativeGoMenu.inc",$core.Substring($a,$b-$a))
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_helheim_bargain.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
$source='namespace StormheimLifeblood { void Offer(Player*,GameObject*){} bool Select(Player*,GameObject*,unsigned,unsigned){return false;} }' + "`n" + $source
[IO.File]::WriteAllText("$output/Bargain.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_helheim_bargain()')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimHelheimBargainTest.cpp`" /Fe`"$output/HelheimBargain.exe`" /Fo`"$output/HelheimBargain.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/HelheimBargain.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Helheim bargain native regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 foreach($marker in @('npc_stormheim_helheim_throne','go_stormheim_helheim_bargain','A Desperate Bargain: ignored generic throne credit','A Desperate Bargain: ignored generic throne talk credit')){if(!$bytes.Contains($marker)){throw "Canonical Release lacks $marker"}}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 foreach($name in @('scripts','game')){
  $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match "$name\\Release\\$name.lib$"} | Select-Object -First 1
  if(!$relative){throw "Missing actual $name link input"}
  $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
  if(!(Test-Path $lib) -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw "Canonical Release predates $name link input"}
 }
 'PASS: canonical Release bargain bindings/credit guards and actual scripts/game final-link inputs.'
}
