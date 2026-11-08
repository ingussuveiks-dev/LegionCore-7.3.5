param([Parameter(Mandatory=$true)][ValidateSet('Elune','Blacksmith','Golem','Chauffeur','Crystal','Argus')][string]$Case,[switch]$Baseline,[switch]$AddressSanitizer,[switch]$ReproduceDanglingBaseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/seven-spell-lifecycles/$Case-$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
function Section([string]$s,[string]$start,[string]$end){$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a);if($a -lt 0 -or $b -lt $a){throw "Missing section $start"};$s.Substring($a,$b-$a)}
function Extract-Class([string]$s,[string]$name){$a=$s.IndexOf("class $name ");if($a -lt 0){throw "Missing class $name"};$i=$s.IndexOf('{',$a);$depth=1;for($j=$i+1;$j -lt $s.Length;$j++){if($s[$j] -eq '{'){$depth++};if($s[$j] -eq '}'){$depth--};if(!$depth){return $s.Substring($a,$j+2-$a)}};throw 'Unclosed class'}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $file=if($Case -in 'Elune','Chauffeur','Argus'){'spell_generic.cpp'}else{'spell_item.cpp'}
 $path="src/server/scripts/Spells/$file";$s=Get-Content "$repo/$path" -Raw -Encoding utf8
 if($Baseline){$s=(& git -C $repo show "6dc3bc59f:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned baseline missing'}}
 $classes=@{Elune=@('spell_gen_touch_of_elune');Blacksmith=@('spell_item_leystone_hoofplates','spell_item_demonsteel_stirrups');Golem=@('spell_item_golem');Chauffeur=@('spell_mount_summon_chauffeur');Crystal=@('spell_masters_hunter_seeking_crystal_teleport');Argus=@('spell_flight_masters_whistle_argus')}
 [IO.File]::WriteAllText("$out/Selected.inc",(($classes[$Case]|ForEach-Object {Extract-Class $s $_}) -join "`n"))
 $s=Get-Content "$repo/src/server/game/Spells/SpellScript.cpp" -Raw
 [IO.File]::WriteAllText("$out/Target.inc",(Section $s 'Unit* AuraScript::GetTarget() const' 'AuraApplication const* AuraScript::GetTargetApplication() const'))
 [IO.File]::WriteAllText("$out/Owner.inc",(Section $s 'Unit* AuraScript::GetUnitOwner() const' 'DynamicObject* AuraScript::GetDynobjOwner() const'))
 $s=Get-Content "$repo/src/server/game/Spells/SpellScript.h" -Raw
 [IO.File]::WriteAllText("$out/Hooks.inc",((Section $s 'enum SpellScriptState' '// helper class from which SpellScript')+(Section $s 'enum AuraScriptHookType' 'class TC_GAME_API AuraScript')))
 $s=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuras.cpp" -Raw
 [IO.File]::WriteAllText("$out/Dispatch.inc",(Section $s 'void Aura::CallScriptEffectUpdateHandlers(' 'void Aura::CallScriptEffectUpdatePeriodicHandlers('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 if($ReproduceDanglingBaseline -and (!($Baseline -and $AddressSanitizer) -or $Case -ne 'Chauffeur')){throw 'Dangling reproduction is only the ASAN chauffeur counterfactual'};$flags="/DCASE_$($Case.ToUpperInvariant())";if($Baseline){$flags+=' /DCOUNTERFACTUAL'};if($AddressSanitizer){$flags+=' /fsanitize=address /Zi'}
 if($ReproduceDanglingBaseline){$flags+=' /DRUN_DANGLING_BASELINE'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /MD $flags /I`"$out`" `"$PSScriptRoot/SevenSpellLifecyclesTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/compiler.pdb`"`nif %errorlevel% neq 0 exit /b %errorlevel%`n`"$out/test.exe`"`n")
 & "$out/run.cmd" *> "$out/run.log";$code=$LASTEXITCODE;Get-Content "$out/run.log" -Tail 12
 if($ReproduceDanglingBaseline){$log=Get-Content "$out/run.log" -Raw;if(!$code -or $log -notmatch 'AddressSanitizer: heap-use-after-free'){throw 'Expected original callback UAF not reproduced'};"COUNTERFACTUAL captured original chauffeur heap-use-after-free, exit=$code; requires repaired ASAN pair"}elseif($code){throw "$Case source regression failed: $code ($out/run.log)"}
}
