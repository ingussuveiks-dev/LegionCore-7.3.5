#requires -Version 7.0
param([switch]$AddressSanitizer,[switch]$VerifyLiveData,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-thoradin-singleton/run-$([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fffffff'))"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 function Block($s,$a){$i=$s.IndexOf($a);if($i -lt 0){throw "Missing native $a"};$j=$s.IndexOf('{',$i)+1;$depth=1;while($depth -and $j -lt $s.Length){if($s[$j] -eq '{'){++$depth};if($s[$j] -eq '}'){--$depth};++$j};if($depth){throw "Unclosed native $a"};$s.Substring($i,$j-$i)}
 $source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
 $baseline=(git -C $repo show 'c71ab7383:src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned native singleton counterfactual unavailable'}
 $boss=Block $source 'struct npc_stromkar_zakajz : SmartAI'
 foreach($method in @('ClaimThoradin','IsFinalThoradin','JustSummoned','HasThoradin')){$token=switch($method){JustSummoned{'void JustSummoned('}default{"bool $method("}};[IO.File]::WriteAllText("$out/$method.inc",(Block $boss $token))}
 $actor=Block $source 'struct npc_stromkar_thoradin : SmartAI';[IO.File]::WriteAllText("$out/ActorSummoned.inc",(Block $actor 'void IsSummonedBy('))
 $boss=Block $baseline 'struct npc_stromkar_zakajz : SmartAI'
 foreach($method in @('JustSummoned','HasThoradin')){$token=if($method -eq 'JustSummoned'){'void JustSummoned('}else{'bool HasThoradin('};[IO.File]::WriteAllText("$out/Baseline$method.inc",(Block $boss $token))}
 $actor=Block $baseline 'struct npc_stromkar_thoradin : SmartAI';[IO.File]::WriteAllText("$out/BaselineActorSummoned.inc",(Block $actor 'void IsSummonedBy('))
 $summon=Get-Content "$repo/src/server/game/Entities/Creature/TemporarySummon.cpp" -Raw
 [IO.File]::WriteAllText("$out/NativeInitSummon.inc",(Block $summon 'void TempSummon::InitSummon()'))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/StromkarThoradinSingletonTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Thoradin singleton regression failed'}
 if($VerifyReleaseBinary){
  if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
  $prior=$env:PYTHONPATH
  try{
   $env:PYTHONPATH="$repo/.codex/pe-review-deps;$prior"
   foreach($file in @('verify_stromkar_thoradin_cache_release.py','verify_stromkar_thoradin_dialogue_release.py')){
    $ErrorActionPreference='Continue'
    & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/$file" $ReleaseDirectory > "$out/$file.stdout.log" 2> "$out/$file.stderr.log"
    $code=$LASTEXITCODE;$ErrorActionPreference='Stop';Get-Content "$out/$file.stdout.log" -Tail 2;Get-Content "$out/$file.stderr.log"
    if($code){throw 'Linked Thoradin singleton/finale regression failed'}
   }
  }finally{$env:PYTHONPATH=$prior;$ErrorActionPreference='Stop'}
 }
 if($VerifyLiveData){
  & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -EvidenceDirectory "$out/sql-reader" -Test {
   $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object{$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
   [IO.File]::WriteAllText("$out/read.sql",@"
SET SESSION TRANSACTION READ ONLY;
START TRANSACTION WITH CONSISTENT SNAPSHOT;
SELECT 'binding',entry,ScriptName FROM creature_template WHERE entry IN(104276,104307) ORDER BY entry;
SELECT 'event',event_type,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,action_param3,target_type FROM smart_scripts WHERE entryorguid=104276 AND source_type=0 AND id=9;
SELECT 'rule',SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ScriptName FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=104276 AND SourceGroup=10 ORDER BY ConditionTypeOrReference;
COMMIT;
"@)
   $prior=$env:MYSQL_PWD
   try{$env:MYSQL_PWD=$env:CODEX_SQL_READER_PASSWORD;$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$env:CODEX_SQL_READER_USER","--database=$($c[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/read.sql" -OutputFile "$out/read.stdout" -ErrorFile "$out/read.stderr";if($code -or (Get-Item "$out/read.stderr").Length){throw 'Native singleton data read failed'}}finally{$env:MYSQL_PWD=$prior}
   $rows=@([IO.File]::ReadAllLines("$out/read.stdout"))
   $expected=@("binding`t104276`tnpc_stromkar_zakajz","binding`t104307`tnpc_stromkar_thoradin","event`t60`t0`t0`t0`t500`t500`t12`t104307`t8`t0`t8",
    "rule`t0`t0`t22`t1`t1539`t0`t0`t0`t","rule`t0`t0`t29`t1`t104307`t50`t0`t1`tcondition_stromkar_thoradin_absence","rule`t0`t0`t31`t1`t3`t104276`t0`t0`t","rule`t0`t0`t36`t1`t0`t0`t0`t0`t","rule`t0`t0`t38`t1`t1`t4`t0`t0`t","rule`t0`t0`t44`t1`t1037`t9`t0`t0`t")
   if([string]::Join("`n",$rows) -cne [string]::Join("`n",$expected)){throw 'Live Thoradin bindings/native retry/six terminal predicates differ'}
   'PASS read-only native two bindings, manual summon/retry and all six terminal/presence conditions; no persistent data write'
  }
 }
}
