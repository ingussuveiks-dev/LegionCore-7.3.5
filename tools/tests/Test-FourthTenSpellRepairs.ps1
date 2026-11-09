param([Parameter(Mandatory=$true)][ValidateSet('Valor','Tirathon','Frog','VantusEN','VantusNH','VantusTOV','VantusTOS','VantusAntorus','Void','Swift','Zone','Stamina','Threat','Censer','Maw')][string]$Case,[switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/fourth-ten-spell-repairs/$Case-$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
function Section([string]$s,[string]$start,[string]$end){$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a);if($a -lt 0 -or $b -lt $a){throw "Missing section $start"};$s.Substring($a,$b-$a)}
function Extract-Class([string]$s,[string]$name){$a=$s.IndexOf("class $name ");if($a -lt 0){throw "Missing class $name"};$i=$s.IndexOf('{',$a);$depth=1;for($j=$i+1;$j -lt $s.Length;$j++){if($s[$j] -eq '{'){$depth++};if($s[$j] -eq '}'){$depth--};if(!$depth){return $s.Substring($a,$j+2-$a)}};throw 'Unclosed class'}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $plan=Get-Content "$PSScriptRoot/FourthTenSpellRepairs.json" -Raw|ConvertFrom-Json
 $entry=$plan.tests|Where-Object key -eq $Case
 $path=$entry.path;$s=Get-Content "$repo/$path" -Raw -Encoding utf8
 if($Baseline){$s=(& git -C $repo show "8189c1241:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned ten-fix baseline missing'}}
 $selected=Extract-Class $s $entry.cls
 $raids=Get-Content "$PSScriptRoot/FourthTenVantus.json" -Raw|ConvertFrom-Json
 if($Case.StartsWith('Vantus')){$raid=$raids|Where-Object {$Case -eq ('Vantus'+$_.key)};$prefix=$raid.enum+';';if($raid.key -eq 'TOV'){$prefix='namespace Data {'+$prefix+'}'};$selected=$prefix+"`n"+$selected}
 if($Case.StartsWith('Vantus')){
 $rows=(@($raid.table|ForEach-Object {'{'+$_.rune+','+$_.helper+','+$_.boss+'}'}) -join ',')
 $defs='#define SELECTED_RUNE '+$entry.cls+"`n#define SELECTED_ROWS "+$rows+"`n#define SELECTED_MAP "+$raid.map+"`n"
 $selected=$defs+$selected
 }
 [IO.File]::WriteAllText("$out/Selected.inc",$selected)
 Copy-Item "$repo/src/server/scripts/Legion/VantusRuneLifecycle.h" "$out/VantusRuneLifecycle.h"
 [IO.File]::WriteAllText("$out/Unit.h",'');[IO.File]::WriteAllText("$out/InstanceScript.h",'')
 & "$PSScriptRoot/Prepare-FourthTenSpellNative.ps1" -OutputDirectory $out
 $s=Get-Content "$repo/src/server/game/Spells/SpellScript.cpp" -Raw
 [IO.File]::WriteAllText("$out/Target.inc",(Section $s 'Unit* AuraScript::GetTarget() const' 'AuraApplication const* AuraScript::GetTargetApplication() const'))
 [IO.File]::WriteAllText("$out/Owner.inc",(Section $s 'Unit* AuraScript::GetUnitOwner() const' 'DynamicObject* AuraScript::GetDynobjOwner() const'))
 $s=Get-Content "$repo/src/server/game/Spells/SpellScript.h" -Raw
 [IO.File]::WriteAllText("$out/Hooks.inc",((Section $s 'enum SpellScriptState' '// helper class from which SpellScript')+(Section $s 'enum AuraScriptHookType' 'class TC_GAME_API AuraScript')))
 $s=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuras.cpp" -Raw
 [IO.File]::WriteAllText("$out/Dispatch.inc",(Section $s 'void Aura::CallScriptEffectUpdateHandlers(' 'void Aura::CallScriptEffectUpdatePeriodicHandlers('))
 $core=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuras.cpp" -Raw
 function Block([string]$t,[string]$start){$a=$t.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$t.IndexOf('{',$a)+1;$d=1;while($d){$d+=([int]($t[$b] -eq '{'))-([int]($t[$b] -eq '}'));++$b};$t.Substring($a,$b-$a)}
 [IO.File]::WriteAllText("$out/CalcDispatch.inc",(Block $core 'void Aura::CallScriptEffectCalcAmountHandlers('))
 $header=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$impl=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $header 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$impl.Substring($impl.IndexOf('FunctionProcessor::FunctionProcessor()')))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $flags="/DCASE_$($Case.ToUpperInvariant()) /fsanitize=address /Zi";if($Baseline){$flags+=' /DCOUNTERFACTUAL'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /MD $flags /I`"$out`" `"$PSScriptRoot/FourthTenSpellRepairsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/compiler.pdb`"`nif %errorlevel% neq 0 exit /b %errorlevel%`n`"$out/test.exe`"`n")
 & "$out/run.cmd" *> "$out/run.log";$code=$LASTEXITCODE;Get-Content "$out/run.log" -Tail 12
 $expectedCrash=$Baseline -and $Case -in 'Valor','Swift','Zone','Stamina','Threat'
 if($expectedCrash){if(!$code -or (Get-Content "$out/run.log" -Raw) -notmatch 'AddressSanitizer: (access-violation|SEGV|DEADLYSIGNAL)'){throw 'Expected original unsupported getter/missing caster dereference not reproduced'};'COUNTERFACTUAL original selected ASAN null dereference;must pair with repaired pass'}elseif($code){throw "$Case source regression failed: $code ($out/run.log)"}
}
# An explicitly verified counterfactual is a successful test assertion; its
# original nonzero executable exit remains recorded in run.log and the output.
$global:LASTEXITCODE=0
