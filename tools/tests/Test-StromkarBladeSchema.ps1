param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/.codex/stromkar-blade-hotfix-schema-2026-10-29"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/hotfixes/2026_10_29_10_stromkar_attachment_parent.sql";if(!(Test-Path -LiteralPath $MigrationFile)){$MigrationFile="$repo/sql/updates/hotfixes/2026_10_29_10_stromkar_attachment_parent.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $source=Get-Content "$repo/src/server/database/Database/Implementation/HotfixDatabase.cpp" -Raw
 $fragment=[regex]::Match($source,'(?s)PrepareStatement\(HOTFIX_SEL_SPELL_VISUAL_KIT_MODEL_ATTACH,\s*(.*?)\s*, CONNECTION_SYNCH\);').Groups[1].Value
 if(!$fragment){throw 'Actual attachment prepared statement missing'}
 $projection=([regex]::Matches($fragment,'"([^"]*)"') | ForEach-Object {$_.Groups[1].Value}) -join ''
 $sql="CREATE TEMPORARY TABLE blade_original LIKE spell_visual_kit_model_attach;CREATE TEMPORARY TABLE spell_visual_kit_model_attach LIKE blade_original;ALTER TABLE spell_visual_kit_model_attach DROP COLUMN IF EXISTS SpellVisualKitID;`n$migration`nSELECT 'first-note-count',@@warning_count;`n$migration`nSHOW WARNINGS;SHOW COLUMNS FROM spell_visual_kit_model_attach;INSERT INTO spell_visual_kit_model_attach(ID,Scale,SpellVisualKitID) VALUES(318611,0.025,64756);`n"+$projection.Replace('SELECT ',"SELECT 'projection', ")+';SELECT COUNT(*) FROM blade_original;'
 $old=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$c[3];$ErrorActionPreference='Continue'
  $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names 2> "$out/stderr.txt")
  $code=$LASTEXITCODE;$ErrorActionPreference='Stop'
  if($code -or (Get-Item "$out/stderr.txt").Length){throw 'TEMP attachment schema regression failed'}
  [IO.File]::WriteAllLines("$out/result.tsv",[string[]]$rows)
  if($rows -notcontains "first-note-count`t0" -or $rows -notcontains "Note`t1060`tDuplicate column name 'SpellVisualKitID'"){throw 'Expected original/add/repeat schema behavior differs'}
  $row=@($rows | Where-Object {$_ -like "projection`t*"})
  if($row.Count -ne 1 -or $row[0].Split("`t").Count -ne 27 -or $row[0].Split("`t")[-1] -ne '64756'){throw 'Actual26-column native prepared projection differs'}
  if($VerifyLiveData){
   $sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
   $live=@(& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names --execute="SELECT COUNT(*) FROM spell_visual_kit_model_attach;SELECT COUNT(*) FROM updates WHERE name='2026_10_29_10_stromkar_attachment_parent.sql' AND hash='$hash' AND state='RELEASED';SHOW COLUMNS FROM spell_visual_kit_model_attach LIKE 'SpellVisualKitID';")
   if($LASTEXITCODE -or $live.Count -ne 3 -or $live[0] -ne '0' -or $live[1] -ne '1' -or $live[2] -notmatch '^SpellVisualKitID\s+int\(11\)\s+NO\s+\s*0\s*$'){throw 'Live native parent schema/updater hash differs'}
  }
 }finally{$env:MYSQL_PWD=$old}
 'PASS TEMP native-parent schema addition, harmless duplicate-column repeat note, actual26-column prepared projection and unchanged empty source table; no live writes by fixture'
}
