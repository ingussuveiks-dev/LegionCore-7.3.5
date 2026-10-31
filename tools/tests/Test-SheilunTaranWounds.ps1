param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
$path='src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/Sheylun.cpp';$header='src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/sheilun_taran_intro.h'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$intro=(Get-Content "$repo/$header" -Raw).Replace("`r",'')
$old=(& git -C $repo show "57321f47e:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-wound Taran source missing'}
$oldIntro=(& git -C $repo show "57321f47e:$header") -join "`n";if($LASTEXITCODE){throw 'Pinned opening helper missing'}
$normal=$source
foreach($role in @('npc_tazhan_chu','npc_sheylun_small_events')){
 $actual=Block $source "class $role :";$prior=Block $old "class $role :"
 $restored=$actual.Replace('        #include "sheilun_companion_wounds.h"','').Replace('            _pendingWoundHeal = false;','')
 $restored=$restored.Replace((Block $restored 'void WoundSpeech('),'')
 if($role -eq 'npc_tazhan_chu'){$restored=$restored.Replace((Block $restored 'void DamageTaken('),(Block $prior 'void HealReceived(')+"`n`n        "+(Block $prior 'void DamageTaken('))}
 else{
  foreach($method in @('void OnHealApplied(','void UpdateLifecycle(')){$restored=$restored.Replace((Block $restored $method),'')}
  $restored=$restored.Replace('        void UpdateAI(',("        "+(Block $prior 'void HealReceived(')+"`n`n        "+(Block $prior 'void DamageTaken(')+"`n`n        void UpdateAI("))
 }
 if(($restored -replace '\s','') -cne ($prior -replace '\s','')){throw "Unrelated companion change: $role"}
 $normal=$normal.Replace($actual,$restored)
}
if(($normal -replace '\s','') -cne ($old -replace '\s','')){throw 'Sheilun changed beyond final-damage/wound reception and reset marker'}
$normal=$intro.Replace('    ObserveWoundHeal();','').Replace('    RecoverWound();','')
if(($normal -replace '\s','') -cne ($oldIntro -replace '\s','')){throw 'Opening receipt changed beyond applied-wound observation/lifecycle delegation'}
if($ScopeOnly){'PASS exact two-class Sheilun companion wound/final-damage/reset-only delta; whole opening/routes/gossip/boss/pickup unchanged';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sheilun-taran-wounds$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Test-SheilunTaranIntro.ps1" -PrepareHost -OutputDirectory $out;if(!$?){throw 'Current native Taran host preparation failed'}
if($Baseline){[IO.File]::WriteAllText("$out/Intro.inc",$oldIntro);[IO.File]::WriteAllText("$out/Wounds.inc",'');[IO.File]::WriteAllText("$out/Speech.inc",'');[IO.File]::WriteAllText("$out/HealReceived.inc",(Block $old 'void HealReceived('));[IO.File]::WriteAllText("$out/Damage.inc",(Block $old 'void DamageTaken('));[IO.File]::WriteAllText("$out/Reset.inc",(Block $old 'void Reset('))}
$small=Block $(if($Baseline){$old}else{$source}) 'class npc_sheylun_small_events :'
foreach($part in @(@('SmallHeal','void HealReceived('),@('SmallDamage','void DamageTaken('),@('SmallSpeech','void WoundSpeech('),@('SmallApplied','void OnHealApplied('),@('SmallLifecycle','void UpdateLifecycle('),@('SmallReset','void Reset('))){[IO.File]::WriteAllText("$out/$($part[0]).inc",$(if($small.Contains($part[1])){Block $small $part[1]}else{''}))}
$hostSource=Get-Content "$PSScriptRoot/SheilunTaranIntroTest.cpp" -Raw;$end=$hostSource.IndexOf('int main()');if($end -lt 0){throw 'Shared actual native host boundary missing'};[IO.File]::WriteAllText("$out/NativeHost.inc",$hostSource.Substring(0,$end))
$core=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw;$prefix='    damage = uint32(std::min<double>(damage / victim->GetHealthMultiplierForTarget(this), std::numeric_limits<uint32>::max()));';$start=$core.IndexOf($prefix);if($start -lt 0){throw 'Native normalized damage prefix changed'};$bridge=Block $core.Substring($start) '    if (Creature* creature = victim->ToCreature())'
if($core.IndexOf('    if (health <= damage)',$start) -lt $start+$bridge.Length){throw 'Native policy/death order changed'}
[IO.File]::WriteAllText("$out/ScaledHook.inc",$prefix+$bridge)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath;$mode=if($Baseline){'/DWOUND_BASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SheilunTaranWoundsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Taran final-damage/applied-wound regression failed'}
if($VerifyNativeCatalog){
 $prior=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SheilunCatalog.cpp -ProbeName SheilunCatalog -OutputFile "$out/native.tsv";if(!$?){throw 'Native Sheilun wound catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
 $rows=Get-Content "$out/native.tsv"
 foreach($expected in @("CRITERION`t0`t46576`t29397`t92`t48371","EFFECT`t204237`t0`t6`t226`t0`t0`t1`t0","DURATION`t204237`t-1`t-1","EOF`tsheilun")){if($rows -notcontains $expected){throw "Native Sheilun wound contract differs: $expected"}}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'binding',entry,AIName,ScriptName FROM creature_template WHERE ScriptName IN('npc_tazhan_chu','npc_sheylun_small_events') ORDER BY entry;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry IN(SELECT entry FROM creature_template WHERE ScriptName IN('npc_tazhan_chu','npc_sheylun_small_events')) ORDER BY Entry;
SELECT 'spawn',guid,id,map,PhaseId FROM creature WHERE id IN(SELECT entry FROM creature_template WHERE ScriptName IN('npc_tazhan_chu','npc_sheylun_small_events')) ORDER BY id,map,guid;
SELECT 'EOF','sheilun-wounds';
'@
 [IO.File]::WriteAllText("$out/live.sql",$query)
 $prior=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/live.sql" -OutputFile "$out/live.tsv" -ErrorFile "$out/live.stderr";if($code -or (Get-Item "$out/live.stderr").Length){throw 'Full-EOF Sheilun binding/scaling readback failed'}}finally{$env:MYSQL_PWD=$prior}
 $expected=@("binding`t101881`t`tnpc_tazhan_chu","binding`t101882`t`tnpc_sheylun_small_events","binding`t101883`t`tnpc_sheylun_small_events","binding`t101885`t`tnpc_sheylun_small_events","binding`t102063`t`tnpc_sheylun_small_events","scaling`t101881`t98`t110","scaling`t101882`t100`t110","scaling`t101883`t100`t110","scaling`t101885`t100`t110","scaling`t102063`t100`t110","spawn`t362837`t101881`t1541`t7000","spawn`t362835`t101882`t1541`t7000 7002","spawn`t362865`t101882`t1541`t7000 7001","spawn`t362834`t101883`t1541`t7000 7002","spawn`t362869`t101883`t1541`t7000 7001","spawn`t362831`t101885`t1541`t7000 7002","spawn`t362867`t101885`t1541`t7000 7001","spawn`t362836`t102063`t1541`t7000 7002","EOF`tsheilun-wounds")
 if(Compare-Object $expected @(Get-Content "$out/live.tsv")){throw 'Review changed shared Sheilun binding/scaling/spawn scope'}
 'PASS native26972/infinite204237 and full World five bindings/five valid scaling rows/eight spawns; no persistent writes'
}
if($VerifyReleaseBinary){
 $prior=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sheilun_companion_wounds_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Sheilun companion wound policy missing'}}finally{$env:PYTHONPATH=$prior}
}
