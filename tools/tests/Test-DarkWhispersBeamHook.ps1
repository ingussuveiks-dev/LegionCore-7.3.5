param([string]$BaselineRevision,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dark-whispers-beam-hook"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Section([string]$s,[string]$start,[string]$end){$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a);if($a -lt 0 -or $b -lt $a){throw "Missing native section $start"};$s.Substring($a,$b-$a)}
$s=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/dark_whispers_artifact.cpp" -Raw
if($BaselineRevision){$s=(& git -C $repo show "${BaselineRevision}:src/server/scripts/Legion/ClassHalls/dark_whispers_artifact.cpp") -join "`n";if($LASTEXITCODE){throw 'Cannot read baseline'}}
[IO.File]::WriteAllText("$out/Beam.inc",(Section $s 'class aura_dark_whispers_ritual_beam' 'void AddSC_dark_whispers_artifact()'))
$helper=Section $s 'bool StopBeam(' 'class spell_dark_whispers_ride_skull'
$helper=$helper.Substring(0,$helper.LastIndexOf('}'))
[IO.File]::WriteAllText("$out/StopBeam.inc",$helper)
$s=Get-Content "$repo/src/server/game/Spells/SpellScript.cpp" -Raw
[IO.File]::WriteAllText("$out/Target.inc",(Section $s 'Unit* AuraScript::GetTarget() const' 'AuraApplication const* AuraScript::GetTargetApplication() const'))
[IO.File]::WriteAllText("$out/Owner.inc",(Section $s 'Unit* AuraScript::GetUnitOwner() const' 'DynamicObject* AuraScript::GetDynobjOwner() const'))
$s=Get-Content "$repo/src/server/game/Spells/SpellScript.h" -Raw
[IO.File]::WriteAllText("$out/AuraHooks.inc",((Section $s 'enum SpellScriptState' '// helper class from which SpellScript')+(Section $s 'enum AuraScriptHookType' 'class TC_GAME_API AuraScript')))
$s=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuras.cpp" -Raw
[IO.File]::WriteAllText("$out/AuraUpdate.inc",(Section $s 'void Aura::CallScriptAuraUpdateHandlers(' 'void Aura::CallScriptEffectUpdatePeriodicHandlers('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/DarkWhispersBeamHookTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Dark Whispers production aura UPDATE regression failed'}
if($VerifyReleaseBinary){
 $map=Get-Content "$repo/build-extractors/bin/Release/worldserver.map" -Raw
 function SymbolAddress([string]$name){
  $match=[regex]::Match($map,('(?m)^\s+\S+\s+'+[regex]::Escape($name)+'\S*\s+([0-9A-Fa-f]{16})\s+f'))
  if(!$match.Success){throw "Missing canonical link symbol $name"}
  [Convert]::ToInt64($match.Groups[1].Value,16)
 }
 $caller=SymbolAddress '?Update@aura_dark_whispers_ritual_beam@@'
 $owner=SymbolAddress '?GetUnitOwner@AuraScript@@'
 $target=SymbolAddress '?GetTarget@AuraScript@@'
 $binary=[IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe")
 $pe=[BitConverter]::ToInt32($binary,0x3c);$optional=$pe+24
 $imageBase=[BitConverter]::ToInt64($binary,$optional+24);$rva=$caller-$imageBase
 $sections=$optional+[BitConverter]::ToUInt16($binary,$pe+20)
 function FileOffset([long]$address){
  $relative=$address-$imageBase
  for($j=0;$j -lt [BitConverter]::ToUInt16($binary,$pe+6);$j++){
   $section=$sections+40*$j;$start=[BitConverter]::ToUInt32($binary,$section+12)
   $size=[Math]::Max([BitConverter]::ToUInt32($binary,$section+8),[BitConverter]::ToUInt32($binary,$section+16))
   if($relative -ge $start -and $relative -lt $start+$size){return [long]([BitConverter]::ToUInt32($binary,$section+20)+$relative-$start)}
  }
  throw 'Address outside canonical PE sections'
 }
 function ResolveThunk([long]$address){
  for($j=0;$j -lt 10;$j++){$at=FileOffset $address;if($binary[$at] -ne 0xe9){return $address};$address+=5+[BitConverter]::ToInt32($binary,$at+1)}
  throw 'Unexpected incremental-link thunk cycle'
 }
 $offset=FileOffset $caller
 # Examine only this function, stopping at the next public code symbol.
 $callerLine=[regex]::Match($map,'(?m)^.*\?Update@aura_dark_whispers_ritual_beam@@.*$')
 $tail=$map.Substring($callerLine.Index+$callerLine.Length,[Math]::Min(8192,$map.Length-$callerLine.Index-$callerLine.Length))
 $next=$caller+512
 foreach($m in [regex]::Matches($tail,'\s([0-9A-Fa-f]{16})\s+f')){$v=[Convert]::ToInt64($m.Groups[1].Value,16);if($v -gt $caller){$next=[Math]::Min($next,$v)}}
 $length=[Math]::Min(512,$next-$caller);$hasOwner=$false;$hasTarget=$false
 for($i=0;$i -lt $length-4;$i++){if($binary[$offset+$i] -eq 0xe8){$dest=ResolveThunk ($caller+$i+5+[BitConverter]::ToInt32($binary,$offset+$i+1));if($dest -eq $owner){$hasOwner=$true};if($dest -eq $target){$hasTarget=$true}}}
 if(!$hasOwner -or $hasTarget){throw 'Canonical beam callback still lacks the safe owner call or calls application-only GetTarget'}
 'PASS: canonical Release beam Update machine code calls GetUnitOwner and does not call GetTarget'
}
