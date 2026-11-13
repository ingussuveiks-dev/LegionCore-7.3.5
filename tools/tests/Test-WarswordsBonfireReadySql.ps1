#requires -Version 7.0
param([string]$Migration='', [string]$OutputDirectory='', [switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$Migration){$Migration="$repo/sql/pending/2026_11_13_00_warswords_bonfire_ready.sql";if(!(Test-Path $Migration)){$Migration="$repo/sql/updates/world/2026_11_13_00_warswords_bonfire_ready.sql"}}
if(!$OutputDirectory){$OutputDirectory="$repo/.codex/bonfire-interaction-20261113/sql-$(Get-Date -Format yyyyMMdd-HHmmss-fff)"}
$out=[IO.Path]::GetFullPath($OutputDirectory);[void][IO.Directory]::CreateDirectory($out)
$migrationSql=[IO.File]::ReadAllText([IO.Path]::GetFullPath($Migration))
$settings=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
if($settings.Count -ne 5 -or $settings[4] -notmatch '^\w+$'){throw 'Unexpected source database settings'}
$world=$settings[4]
$setup=@'
CREATE TEMPORARY TABLE saved_gameobject LIKE gameobject;
INSERT INTO saved_gameobject SELECT * FROM gameobject WHERE guid=134117 OR guid=(SELECT MIN(guid) FROM gameobject WHERE guid<>134117);
CREATE TEMPORARY TABLE saved_gameobject_template LIKE gameobject_template;
INSERT INTO saved_gameobject_template SELECT * FROM gameobject_template WHERE entry=245188;
CREATE TEMPORARY TABLE gameobject LIKE saved_gameobject;
INSERT INTO gameobject SELECT * FROM saved_gameobject;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_gameobject_template;
INSERT INTO gameobject_template SELECT * FROM saved_gameobject_template;
UPDATE gameobject SET state=0 WHERE guid=134117;
'@
function Frame([string]$tag,[switch]$Expected){$spawn=if($Expected){'expected_gameobject'}else{'gameobject'};return "SELECT 'BEGIN','$tag';SELECT 'gameobject';SELECT * FROM $spawn ORDER BY guid;SELECT 'gameobject_template';SELECT * FROM gameobject_template ORDER BY entry;SELECT 'END','$tag';`n"}
function Extract([string]$value,[string]$tag){$begin="BEGIN`t$tag`n";$end="END`t$tag";$a=$value.IndexOf($begin);if($a -lt 0){throw "Missing $tag"};$a+=$begin.Length;$b=$value.IndexOf($end,$a);if($b -lt 0){throw "Incomplete $tag"};return $value.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -EvidenceDirectory "$out/reader" -Test {
  function Session([string]$sql,[string]$label,[switch]$Reject){
   $saved=$env:MYSQL_PWD
   try{
    $env:MYSQL_PWD=$env:CODEX_SQL_READER_PASSWORD
    [IO.File]::WriteAllText("$out/$label.sql",$sql+"`nSELECT 'EOF','$label';`n")
    $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($settings[0])","--port=$($settings[1])","--user=$env:CODEX_SQL_READER_USER","--database=$world",'--batch','--raw','--skip-column-names','--force') -InputFile "$out/$label.sql" -OutputFile "$out/$label.tsv" -ErrorFile "$out/$label.stderr"
    $errors=[IO.File]::ReadAllText("$out/$label.stderr");$result=[IO.File]::ReadAllText("$out/$label.tsv").Replace("`r",'')
    [IO.File]::WriteAllText("$out/$label.exit",[string]$code)
    if(!$result.Contains("EOF`t$label")){throw "Final SQL result missing: $label"}
    if($Reject){$lines=@($errors -split "`n"|Where-Object {$_ -match '^ERROR '});if($code -notin @(0,1) -or $lines.Count -ne 1 -or $lines[0] -notmatch 'ERROR 1644 \(45000\).*Warswords bonfire ready: source context differs'){throw "Expected isolated refusal missing: $label"}}
    elseif($code -or $errors){throw "Unexpected SQL exit/stderr: $label"}
    return $result
   }finally{$env:MYSQL_PWD=$saved}
  }
  $expected="CREATE TEMPORARY TABLE expected_gameobject LIKE gameobject;INSERT INTO expected_gameobject SELECT * FROM gameobject;UPDATE expected_gameobject SET state=1 WHERE guid=134117;`n"
  $result=Session ($setup+$expected+(Frame 'expected' -Expected)+$migrationSql+(Frame 'after1')+$migrationSql+(Frame 'after2')) 'valid-idempotent'
  if((Extract $result 'expected') -cne (Extract $result 'after1') -or (Extract $result 'after1') -cne (Extract $result 'after2')){throw 'Migration changed more than the exact state or is not idempotent'}
  $mutations=@('DELETE FROM gameobject WHERE guid=134117;','UPDATE gameobject SET id=1 WHERE guid=134117;','UPDATE gameobject SET map=1220 WHERE guid=134117;','UPDATE gameobject SET state=2 WHERE guid=134117;','UPDATE gameobject_template SET type=3;','UPDATE gameobject_template SET Data0=1;','UPDATE gameobject_template SET Data10=3365;',"UPDATE gameobject_template SET ScriptName='codex_custom';",'DELETE FROM gameobject_template;')
  for($caseIndex=0;$caseIndex -lt $mutations.Count;++$caseIndex){$result=Session ($setup+$mutations[$caseIndex]+(Frame 'before')+$migrationSql+(Frame 'after')) "refused-$caseIndex" -Reject;if((Extract $result 'before') -cne (Extract $result 'after')){throw "Refused context modified: $caseIndex"}}
  if($VerifyLiveData){$result=Session "SELECT 'LIVE_STATE',g.guid,g.id,g.map,g.state,t.type,t.Data0,t.Data10,t.ScriptName FROM gameobject AS g INNER JOIN gameobject_template AS t ON t.entry=g.id WHERE g.guid=134117;" 'live-readback';if($result -notmatch "(?m)^LIVE_STATE`t134117`t245188`t1511`t1`t10`t2474`t198513`tgo_mystic_bonfire$"){throw 'Canonical live bonfire state/binding differs'};'PASS actual persistent bonfire134117/245188/map1511 READY1/native lock2474/spell198513/binding readback'}
  'PASS real MariaDB SELECT+TEMP-only reader/persistent-DML refusal: exact spawn state0->1,all other fields/neighbor/template unchanged,idempotence,and9 custom/missing-context refusals unchanged under --force;full EOF/stdout/stderr/exit capture'
 }
}
