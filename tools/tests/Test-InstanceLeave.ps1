param([switch]$Baseline,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$token){$a=$s.IndexOf($token);if($a -lt 0){throw "Missing $token"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $token"};$s.Substring($a,$b-$a)}
function Read-Selected([string]$path){if($Baseline){$s=(& git -C $repo show "ba856f49b:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned leave source missing'};$s}else{Get-Content "$repo/$path" -Raw}}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/instance-leave$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[IO.File]::WriteAllText("$out/MapRemove.inc",(Block (Read-Selected 'src/server/game/Maps/Map.cpp') 'void Map::RemovePlayerFromMap('))
[IO.File]::WriteAllText("$out/GroupAura.inc",(Block (Get-Content "$repo/src/server/game/Instances/InstanceScript.cpp" -Raw) 'void InstanceScript::DoRemoveAurasDueToSpellOnPlayers('))
$specs=@(
 @('Ashran','OutdoorPvP/Ashran/AshranInstance.cpp',@('AshranMapID')),
 @('Highmaul','Draenor/Highmaul/instance_highmaul.cpp',@('PlayChogallScene','ChogallNight')),
 @('Ragefire','Scenario/TheSecretsOfRagefire/instance_the_secrets_of_ragefire.cpp',@('SPELL_GOBLIN_ILLUSION')),
 @('DarkRiders','Legion/ClassHalls/dark_riders_artifact.cpp',@()),
 @('Icecrown','Northrend/IcecrownCitadel/instance_icecrown_citadel.cpp',@()),
 @('DarkHeart','Scenario/DarkHeartOfPandaria/instance_dark_heart_of_pandaria.cpp',@('SPELL_GOBLIN_ILLUSION')),
 @('Army','Scenario/ArmyTraining/instance_army_training.cpp',@('SPELL_STARTED_OVERRIDE','SPELL_COUNT_MINIONS','SPELL_TELE_END')),
 @('Tournament','Scenario/CelestialTournament/instance_celestial_tournament.cpp',@('SPELL_BATTLE_PET_CHALLENGE')),
 @('BrokenShore','Scenario/BrokenIslands/instance_broken_islands.cpp',@()),
 @('Snow','Scenario/BloodInTheSnow/instance_blood_in_the_snow.cpp',@('SPELL_BLIZZARD_WEATHER')),
 @('Heart','Pandaria/HeartFear/instance_heart_of_fear.cpp',@()),
 @('Gate','Pandaria/GateSettingSun/instance_gate_setting_sun.cpp',@()),
 @('Siege','Pandaria/SiegeofOrgrimmar/instance_siege_of_orgrimmar.cpp',@())
)
$chunks=@()
foreach($spec in $specs){
 $path="src/server/scripts/$($spec[1])";$source=Read-Selected $path;$body=Block $source 'void OnPlayerLeave('
 $headers=@(Get-ChildItem -LiteralPath (Split-Path "$repo/$path") -File|Where-Object Extension -in '.h','.hpp'|ForEach-Object {Get-Content $_.FullName -Raw})
 $definitions=($source+"`n"+($headers -join "`n"));$names=@($spec[2]);$array=''
 if($spec[0] -eq 'Siege'){$array=(Block $source 'uint32 removelist[30]')+';';$names=@([regex]::Matches($array,'\bSPELL_\w+\b')|ForEach-Object {$_.Value}|Sort-Object -Unique)}
 $constants=@()
 foreach($name in $names){
  if($name -eq 'AshranMapID'){$constants+='static constexpr uint32 AshranMapID=1191;';continue}
  $matches=@([regex]::Matches($definitions,"\b$([regex]::Escape($name))\s*=\s*(\d+)\b")|ForEach-Object {$_.Groups[1].Value}|Sort-Object -Unique)
  if($matches.Count -ne 1){throw "Resolve exact native constant $name in $path; matches=$($matches.Count)"}
  $constants+="static constexpr uint32 $name=$($matches[0]);"
 }
 $chunks+=('struct '+$spec[0]+'Leave:InstanceScript {'+($constants -join "`n")+"`n"+$array+"`n"+$body+"`n};")
}
[IO.File]::WriteAllText("$out/Callbacks.inc",($chunks -join "`n"))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath;$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/InstanceLeaveTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Instance leave source regression failed'}
