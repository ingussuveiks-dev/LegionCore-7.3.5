param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyNativeCatalog,[switch]$VerifyLiveData,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
$path='src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/sheilun_plan.h';$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
$old=(& git -C $repo show "872b3da2a:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned no-tactics helper missing'}
. "$PSScriptRoot/SheilunAcquisitionScope.ps1"
$comparison=if($source.Contains('#include "sheilun_acquisition_api.h"')){Get-SheilunAcquisitionRetainedPlan $source}else{$source}
$normal=$comparison.Replace('#include "sheilun_tactics.h"','').Replace('    if (!ApplyTactic(player, actor, *role, option, map, recipient, source, attempt)) return;','')
if(($normal -replace '\s','') -cne ($old -replace '\s','')){throw 'Plan helper changed beyond acknowledged native tactic call/include'}
if($ScopeOnly){'PASS exact872b3da2a retained plan header delta after explicit acquisition observer certificate: only selected native tactic acknowledgement before role criteria; native admission/receipts/start commands retained';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sheilun-tactics$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Test-SheilunPlan.ps1" -PrepareHost -OutputDirectory $out;if(!$?){throw 'Actual native plan/gossip host preparation failed'}
if($Baseline){[IO.File]::WriteAllText("$out/Plan.inc",$old)}
$hostSource=Get-Content "$PSScriptRoot/SheilunPlanTest.cpp" -Raw;$end=$hostSource.IndexOf('int main()');if($end -lt 0){throw 'Native plan host boundary missing'};[IO.File]::WriteAllText("$out/NativeHost.inc",$hostSource.Substring(0,$end))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw;$unitHeader=Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw;$auraHeader=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuraDefines.h" -Raw
[IO.File]::WriteAllText("$out/Enums.inc",((Block $unitHeader 'enum UnitMods')+';'+(Block $unitHeader 'enum UnitModifierType')+';'+(Block $unitHeader 'enum WeaponAttackType')+';'+(Block $auraHeader 'enum AuraType')+';'+(Block $auraHeader 'enum AuraEffectHandleModes')+';'))
foreach($pair in @(@('Stats','bool Unit::HandleStatModifier('),@('Multiplier','float Unit::GetTotalAuraMultiplier(AuraType auratype, std::function'),@('Mask','float Unit::GetTotalAuraMultiplierByMiscMask('))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $unit $pair[1]))}
[IO.File]::WriteAllText("$out/DamageHandler.inc",(Block (Get-Content "$repo/src/server/game/Spells/Auras/SpellAuraEffects.cpp" -Raw) 'void AuraEffect::HandleModDamagePercentDone('))
[IO.File]::WriteAllText("$out/Percent.inc",(Block (Get-Content "$repo/src/common/Utilities/Util.cpp" -Raw) 'void ApplyPercentModFloatVar('))
$utility=Get-Content "$repo/src/common/Utilities/Util.h" -Raw
[IO.File]::WriteAllText("$out/AddPercent.inc",('template<class T,class U> '+(Block $utility 'T CalculatePct(')+'template<class T,class U> '+(Block $utility 'T AddPct(')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath;$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SheilunTacticsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native selected Sheilun tactic regression failed'}
if(!$Baseline){
 [IO.File]::WriteAllText("$out/damage.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/SheilunTacticsDamageTest.cpp`" /Fe`"$out/damage.exe`" /Fo`"$out/damage.obj`" /Fd`"$out/damage.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/damage.exe`"`nexit /b %errorlevel%`n")
 & "$out/damage.cmd";if($LASTEXITCODE){throw 'Native tactical aura damage consumer regression failed'}
}
if($VerifyNativeCatalog){$prior=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SheilunTacticsCatalog.cpp -ProbeName SheilunTacticsCatalog -OutputFile "$out/native.tsv";if(!$?){throw 'Native tactical catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$prior};$rows=Get-Content "$out/native.tsv";foreach($line in @("EFFECT`t204653`t0`t6`t79`t25`t0`t127`t0`t1`t0`t0","EFFECT`t204653`t1`t6`t87`t50`t0`t127`t0`t1`t0`t0","EFFECT`t204654`t0`t6`t4`t0`t0`t127`t0`t1`t0`t0","DURATION`t204653`t-1`t-1","DURATION`t204654`t-1`t-1")){if($rows -notcontains $line){throw "Native tactic effect differs: $line"}};if($rows[-1] -notlike "EOF`tsheilun-tactics`t*"){throw 'Native tactics catalog EOF missing'}}
if($VerifyLiveData){
 $query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'groups',COUNT(*) FROM spell_group WHERE ABS(spell_id) IN(204653,204654);
SELECT 'scripts',COUNT(*) FROM spell_script_names WHERE ABS(spell_id) IN(204653,204654);
SELECT 'links',COUNT(*) FROM spell_linked_spell WHERE ABS(spell_trigger) IN(204653,204654) OR ABS(spell_effect) IN(204653,204654);
SELECT 'cast-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=17 AND ABS(SourceEntry) IN(204653,204654);
SELECT 'template-addons',COUNT(*) FROM creature_template_addon WHERE entry IN(101882,101883,101885);
SELECT 'spawn-addons',COUNT(*) FROM creature_addon WHERE guid IN(362831,362834,362835,362865,362867,362869);
SELECT 'EOF','sheilun-tactics';
'@
 [IO.File]::WriteAllText("$out/live.sql",$query)
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$db[3];$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/live.sql" -OutputFile "$out/live.tsv" -ErrorFile "$out/live.stderr";if($code -or (Get-Item "$out/live.stderr").Length){throw 'Full-EOF tactics data readback failed'}}finally{$env:MYSQL_PWD=$prior}
 $expected=@("groups`t0","scripts`t0","links`t0","cast-conditions`t0","template-addons`t0","spawn-addons`t0","EOF`tsheilun-tactics")
 if(Compare-Object $expected @(Get-Content "$out/live.tsv")){throw 'Review changed tactic groups/scripts/links/cast conditions/addons'}
 'PASS readonly full EOF native tactics have no custom same-effect group/script/link/source17 or source addon conflict; no persistent writes'
}
if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sheilun_tactics_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual Sheilun native tactic Release implementation missing'}}finally{$env:PYTHONPATH=$previous}}
