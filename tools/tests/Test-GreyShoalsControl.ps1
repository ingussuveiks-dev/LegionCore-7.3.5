$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/grey-shoals-control"
New-Item -ItemType Directory -Force $out | Out-Null
function Export([string]$file,[string]$start,[string]$end,[string]$name){
 $source=Get-Content "$repo/$file" -Raw;$a=$source.IndexOf($start);$b=$source.IndexOf($end,$a)
 if($a -lt 0 -or $b -le $a){throw "Missing native control boundary $name"}
 [IO.File]::WriteAllText("$out/$name.inc",$source.Substring($a,$b-$a))
}
Export 'src/server/game/Entities/Unit/Unit.cpp' 'bool Unit::SetCharmedBy(' 'void Unit::RestoreFaction()' 'ControlCharm'
Export 'src/server/game/Entities/Unit/Unit.cpp' 'void Unit::SetCharm(' 'int32 Unit::DealHeal(' 'ControlRelations'
Export 'src/server/game/Entities/Unit/Unit.cpp' 'void Unit::RemoveCharmAuras()' 'void Unit::UnsummonAllTotems()' 'ControlAuras'
Export 'src/server/game/Entities/Unit/Unit.cpp' 'Unit* Unit::GetFirstControlled() const' 'void Unit::RemoveAllControlled()' 'ControlFirst'
Export 'src/server/game/AI/SmartScripts/SmartAI.cpp' 'void SmartAI::OnCharmed(' 'void SmartAI::DoAction(' 'ControlSmartHook'
Export 'src/server/game/AI/CreatureAI.cpp' 'void CreatureAI::OnCharmed(' 'CreatureAI::CreatureAI(' 'ControlBaseHook'
Export 'src/server/game/Spells/Auras/SpellAuraEffects.cpp' 'void AuraEffect::HandleModPossess(' 'void AuraEffect::HandleAuraControlVehicle(' 'ControlAuraHooks'
$dispatch=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuraEffects.cpp" -Raw
foreach($pair in @(@(2,'HandleModPossess'),@(6,'HandleModCharm'),@(177,'HandleCharmConvert'))){
 if($dispatch -notmatch ('&AuraEffect::'+$pair[1]+',\s*//\s*'+$pair[0]+'\s')){throw "Native aura dispatch mapping changed for $($pair[0])"}
}
$unitHeader=Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw
$enums=''
foreach($name in @('CharmType','ReactStates')){
 $match=[regex]::Match($unitHeader,"(?s)enum $name\b.*?\n\};")
 if(!$match.Success){throw "Missing native control enum $name"};$enums+=$match.Value+"`n"
}
[IO.File]::WriteAllText("$out/ControlEnums.inc",$enums)
$source=Get-Content "$repo/src/server/scripts/Legion/grey_shoals.h" -Raw
[IO.File]::WriteAllText("$out/ControlCurrent.inc",$source)
$old=(& git -C $repo show 'f9bc6062:src/server/scripts/Legion/grey_shoals.h') -join "`n"
if($LASTEXITCODE){throw 'Missing pinned ordinary-rotation baseline'}
[IO.File]::WriteAllText("$out/ControlBaseline.inc",$old)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($mode in @('Current','Baseline')){
 $define=if($mode -eq 'Baseline'){'/DCONTROL_BASELINE'}else{''}
 $cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 $define /I`"$out`" `"$PSScriptRoot/GreyShoalsControlTest.cpp`" /Fe`"$out/$mode.exe`" /Fo`"$out/$mode.obj`"`nif not %errorlevel%==0 exit /b %errorlevel%`n`"$out/$mode.exe`"`n"
 [IO.File]::WriteAllText("$out/run-$mode.cmd",$cmd);& "$out/run-$mode.cmd"
 if($LASTEXITCODE){throw "Grey Shoals native control regression failed ($mode)"}
}
'PASS: production rotation + native charm application/removal, aura handlers, SmartAI charm callback and control relations. Aura storage, CharmInfo initialization/state, movement/packet services and spell creation remain fixtures; client control/portrait still pending.'
