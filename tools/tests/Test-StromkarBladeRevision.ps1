param([switch]$VerifyLiveData)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-blade-revision/run-$([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fffffff'))"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -Database Hotfix -EvidenceDirectory "$out/sql-reader" -Test {
 $path="$repo/sql/pending/hotfixes/2026_10_30_11_stromkar_blade_revision.sql";if(!(Test-Path $path)){$path="$repo/sql/updates/hotfixes/2026_10_30_11_stromkar_blade_revision.sql"}
 $migration=Get-Content $path -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object{$_ -match '^HotfixDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:bladeQuery=0
 function Query($sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$p="$out/query-$script:bladeQuery";++$script:bladeQuery
  try{
   if(!$env:CODEX_SQL_READER_USER -or !$env:CODEX_SQL_READER_PASSWORD){throw 'Restricted SQL reader required'}
   $env:MYSQL_PWD=$env:CODEX_SQL_READER_PASSWORD
   $a=@("--host=$($c[0])","--port=$($c[1])","--user=$env:CODEX_SQL_READER_USER","--database=$($c[4])",'--batch','--raw','--skip-column-names');if($Refusal){$a+='--force'}
   [IO.File]::WriteAllText("$p.sql",$sql)
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $a -InputFile "$p.sql" -OutputFile "$p.stdout" -ErrorFile "$p.stderr"
   [IO.File]::WriteAllText("$p.exit.txt",[string]$code);$err=[IO.File]::ReadAllText("$p.stderr")
   if($Refusal){if($err -notmatch 'Stromkar blade cache notice differs' -or ([regex]::Matches($err,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal: $p"}}
   elseif($code -or $err){throw "Read/TEMP query failed: $p"}
   return ,@([IO.File]::ReadAllLines("$p.stdout"))
  }finally{$env:MYSQL_PWD=$old}
 }
 $setup=@'
CREATE TEMPORARY TABLE saved_hotfix_data LIKE hotfix_data;
INSERT INTO saved_hotfix_data SELECT * FROM hotfix_data;
CREATE TEMPORARY TABLE hotfix_data LIKE saved_hotfix_data;
INSERT INTO hotfix_data SELECT * FROM saved_hotfix_data;
DELETE FROM hotfix_data WHERE TableHash=4033975491 AND RecordId=318611;
'@
 $snap="SELECT 'TAG',h.* FROM hotfix_data h ORDER BY Id,TableHash,RecordId;"
 $rows=Query ($setup+$snap.Replace('TAG','before')+$migration+$snap.Replace('TAG','first')+"SELECT 'notice',Id,TableHash,RecordId,Deleted FROM hotfix_data WHERE TableHash=4033975491 AND RecordId=318611;"+$migration+$snap.Replace('TAG','second'))
 [IO.File]::WriteAllLines("$out/temp.tsv",[string[]]$rows)
 function Signature($r,$tag){@($r|Where-Object{$_ -like "$tag`t*"}|ForEach-Object{$_.Substring($tag.Length+1)})}
 $before=Signature $rows before;$first=Signature $rows first;$second=Signature $rows second
 if([string]::Join("`n",$first) -cne [string]::Join("`n",$second)){throw 'Not idempotent'}
 $added=@(Compare-Object $before $first|Where-Object{$_.SideIndicator -eq '=>'}|ForEach-Object{$_.InputObject})
 if($added.Count -ne 1 -or (Compare-Object $before $first|Where-Object{$_.SideIndicator -eq '<='})){throw 'Unrelated notice changed'}
 $notice=@($rows|Where-Object{$_ -like "notice`t*"});if($notice.Count -ne 1){throw 'Missing notice projection'}
 $f=$notice[0].Split("`t");if($f[2] -ne '4033975491' -or $f[3] -ne '318611' -or $f[4] -ne '0' -or [long]$f[1] -le 76){throw 'Wrong revision notice'}
 foreach($variant in @('DELETE FROM hotfix_data;INSERT INTO hotfix_data(Id,TableHash,RecordId,Deleted) VALUES(76,4033975491,318611,0);','DELETE FROM hotfix_data;INSERT INTO hotfix_data(Id,TableHash,RecordId,Deleted) VALUES(77,4033975491,318611,1);','DELETE FROM hotfix_data;INSERT INTO hotfix_data(Id,TableHash,RecordId,Deleted) VALUES(77,4033975491,318611,0),(78,4033975491,318611,0);','DELETE FROM hotfix_data;INSERT INTO hotfix_data(Id,TableHash,RecordId,Deleted) VALUES(2147483647,1,1,0);','DELETE FROM hotfix_data;')){
  $r=Query ($setup+$variant+$snap.Replace('TAG','before')+$migration+$snap.Replace('TAG','after')) -Refusal
  if([string]::Join("`n",[string[]]@(Signature $r before)) -cne [string]::Join("`n",[string[]]@(Signature $r after))){throw 'Custom notice changed'}
 }
 if($VerifyLiveData){
  if($path -like '*pending*'){throw 'Pending migration cannot certify live revision'}
  $sha=(Get-FileHash $path -Algorithm SHA1).Hash
  $r=Query "SELECT hash,state FROM updates WHERE name='2026_10_30_11_stromkar_blade_revision.sql';SELECT Id,Deleted FROM hotfix_data WHERE TableHash=4033975491 AND RecordId=318611;"
  if($r.Count -ne 2 -or $r[0] -ne "$sha`tRELEASED" -or [long]$r[1].Split("`t")[0] -le 76 -or $r[1].Split("`t")[1] -ne '0'){throw 'Live revision differs'}
 }
 'PASS one new complete-record revision notice, native TEMP repeat/all unrelated notices preserved/five refused variants; restricted persistent writes denied and reader removed. Actual client refresh remains separate.'
 }
}
