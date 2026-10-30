param([switch]$VerifyLiveData)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/odyn-gaze-self-target/run-$([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fffffff'))"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -EvidenceDirectory "$out/sql-reader" -Test {
 function Section($s,$a,$b){$i=$s.IndexOf($a);$j=$s.IndexOf($b,[Math]::Max(0,$i));if($i -lt 0 -or $j -le $i){throw "Missing native $a"};$s.Substring($i,$j-$i)}
 $s=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
 [IO.File]::WriteAllText("$out/ExplicitTargets.inc",(Section $s 'void Spell::InitExplicitTargets(' 'void Spell::SelectExplicitTargets('))
 $s=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 [IO.File]::WriteAllText("$out/ConditionObject.inc",(Section $s '    if (!object)' '    bool condMeets = false;'))
 [IO.File]::WriteAllText("$out/ClassCondition.inc",(Section $s '        case CONDITION_CLASS:' '        case CONDITION_RACE:'))
 $migration=Get-Content "$repo/sql/updates/world/2026_10_30_28_odyn_gaze_self_target.sql" -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:odynQuery=0
 function Query($sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$path="$out/query-$script:odynQuery";++$script:odynQuery
  try{
   if(!$env:CODEX_SQL_READER_USER -or !$env:CODEX_SQL_READER_PASSWORD){throw 'Restricted SQL reader required'}
   $env:MYSQL_PWD=$env:CODEX_SQL_READER_PASSWORD
   $argsSql=@("--host=$($c[0])","--port=$($c[1])","--user=$env:CODEX_SQL_READER_USER","--database=$($c[4])",'--batch','--raw','--skip-column-names')
   if($Refusal){$argsSql+='--force'}
   [IO.File]::WriteAllText("$path.sql",$sql)
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $argsSql -InputFile "$path.sql" -OutputFile "$path.stdout" -ErrorFile "$path.stderr"
   [IO.File]::WriteAllText("$path.exit.txt",[string]$code)
   $err=[IO.File]::ReadAllText("$path.stderr")
   if($Refusal){if($err -notmatch 'Odyn gaze self-target conditions differ' -or ([regex]::Matches($err,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal: $path"}}
   elseif($code -or $err){throw "Read/TEMP query failed: $path"}
   return ,@([IO.File]::ReadAllLines("$path.stdout"))
  }finally{$env:MYSQL_PWD=$old}
 }
 $setup=@'
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=215997;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
DELETE FROM conditions WHERE ConditionTarget=1;
INSERT INTO conditions SELECT SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,1,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment FROM conditions WHERE ConditionTarget=0;
'@
 $snapshot="SELECT 'TAG',c.* FROM conditions c ORDER BY ConditionTarget;"
 $rows=Query ($setup+$migration+$snapshot.Replace('TAG','first')+$migration+$snapshot.Replace('TAG','second')+"SELECT 'rule',ConditionTarget,ConditionValue1 FROM conditions;")
 [IO.File]::WriteAllLines("$out/temp.tsv",[string[]]$rows)
 function Signature($r,$tag){[string]::Join("`n",@($r|Where-Object{$_ -like "$tag`t*"}|ForEach-Object{$_.Substring($tag.Length+1)}))}
 if((Signature $rows first) -cne (Signature $rows second)){throw 'Not idempotent'}
 [IO.File]::WriteAllLines("$out/rules.tsv",[string[]]@($rows|Where-Object{$_ -like "rule`t*"}|ForEach-Object{$_.Substring(5)}))
 foreach($variant in @("UPDATE conditions SET ConditionValue1=2;","UPDATE conditions SET ScriptName='custom';",'DELETE FROM conditions WHERE ConditionTarget=0;','UPDATE conditions SET SourceGroup=1;',"UPDATE conditions SET Comment='custom';")){
  $r=Query ($setup+$variant+$snapshot.Replace('TAG','before')+$migration+$snapshot.Replace('TAG','after')) -Refusal
  if((Signature $r before) -cne (Signature $r after)){throw 'Custom state changed'}
 }
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/OdynGazeSelfTargetTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/rules.tsv`"`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native self-target regression failed'}
 if($VerifyLiveData){
  $r=Query "SELECT ConditionTarget,ConditionValue1 FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=215997;"
  if($r.Count -ne 1 -or $r[0] -ne "0`t1"){throw 'Live caster gate mismatch'}
  $sha=(Get-FileHash "$repo/sql/updates/world/2026_10_30_28_odyn_gaze_self_target.sql" -Algorithm SHA1).Hash
  $r=Query "SELECT hash,state FROM updates WHERE name='2026_10_30_28_odyn_gaze_self_target.sql';"
  if($r.Count -ne 1 -or $r[0] -ne "$sha`tRELEASED"){throw 'Live updater hash/state mismatch'}
 }
 'PASS guarded TEMP repeat and five custom refusals. No live writes by this test.'
 }
}
