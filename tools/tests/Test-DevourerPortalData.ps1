param([string]$MigrationPath,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$MigrationPath){$MigrationPath="$repo/sql/updates/world/2026_11_05_03_devourer_portal.sql";if(!(Test-Path -LiteralPath $MigrationPath)){$MigrationPath="$repo/.codex/pending-sql/2026_11_05_03_devourer_portal.sql"}}
$out="$repo/build-extractors/runtime-test/devourer-portal-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $migration=Get-Content -LiteralPath $MigrationPath -Raw
 $live=if($VerifyLiveData){"SELECT 'live',COUNT(*) FROM creature_text WHERE CreatureID=105488 AND ID=0 AND ((GroupID=0 AND BroadcastTextID=108250) OR (GroupID=2 AND BroadcastTextID=108257) OR (GroupID=3 AND BroadcastTextID=108249)); SELECT 'live-go',ScriptName,Data22 FROM gameobject_template WHERE entry=248901;"}else{''}
 $sql=@"
$live
CREATE TEMPORARY TABLE portal_text_seed LIKE creature_text;
CREATE TEMPORARY TABLE creature_text LIKE portal_text_seed;
INSERT INTO creature_text(CreatureID,GroupID,ID,Text) VALUES(105488,0,7,'preserve sibling'),(105488,1,0,'preserve other group'),(999999,2,0,'preserve other actor');
CREATE TEMPORARY TABLE portal_go_seed LIKE gameobject_template;
INSERT INTO portal_go_seed SELECT * FROM gameobject_template WHERE entry=248901;
CREATE TEMPORARY TABLE gameobject_template LIKE portal_go_seed;
INSERT INTO gameobject_template SELECT * FROM portal_go_seed;
$migration
SELECT 'first',CreatureID,GroupID,ID,BroadcastTextID FROM creature_text ORDER BY CreatureID,GroupID,ID;
SELECT 'first-go',ScriptName,Data22 FROM gameobject_template WHERE entry=248901;
$migration
SELECT 'second',CreatureID,GroupID,ID,BroadcastTextID FROM creature_text ORDER BY CreatureID,GroupID,ID;
SELECT 'second-go',ScriptName,Data22 FROM gameobject_template WHERE entry=248901;
UPDATE gameobject_template SET ScriptName='portal_unrelated' WHERE entry=248901;
$migration
SELECT 'foreign-script',ScriptName,Data22 FROM gameobject_template WHERE entry=248901;
"@
 [IO.File]::WriteAllText("$out/fixture.sql",$sql)
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$db[3];$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/fixture.sql" -OutputFile "$out/fixture.tsv" -ErrorFile "$out/stderr.log";if($code -or (Get-Item "$out/stderr.log").Length){throw 'Portal TEMP-only SQL replay failed'}}finally{$env:MYSQL_PWD=$previous}
 $rows=Get-Content "$out/fixture.tsv"
 foreach($label in @('first','second')){if(@($rows|Where-Object {$_ -like "$label`t*"}).Count -ne 6){throw 'Unexpected portal text rows'};foreach($row in @("105488`t0`t0`t108250","105488`t0`t7`t0","105488`t1`t0`t0","105488`t2`t0`t108257","105488`t3`t0`t108249","999999`t2`t0`t0")){if($rows -notcontains "$label`t$row"){throw 'Portal text replay changed unrelated/authored data'}};if($rows -notcontains "$label-go`tgo_devourer_misc`t39597"){throw 'Portal binding/legacy condition differs'}}
 if($rows -notcontains "foreign-script`tportal_unrelated`t39597"){throw 'Migration replaced unrelated ScriptName'}
 if($VerifyLiveData -and ($rows -notcontains "live`t3" -or $rows -notcontains "live-go`tgo_devourer_misc`t39597")){throw 'Actual live portal migration readback differs'}
 $old=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/DevourerPortalCatalog.cpp -ProbeName DevourerPortalCatalog -OutputFile "$out/catalog.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Typed native portal catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
 Get-Content "$out/catalog.tsv"|Where-Object {$_ -like 'PASS *'}
 'PASS idempotent TEMP-only three-text migration, sibling/group/actor/custom ScriptName preservation; native GO condition39597 retained. Live readback='+$VerifyLiveData
}
