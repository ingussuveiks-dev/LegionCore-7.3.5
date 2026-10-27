param([switch]$VerifyLive)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/ikky-breed-data"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($out) -Test {
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $migration=Get-Content "$repo/sql/updates/world/2026_10_27_15_ikky_native_breed.sql" -Raw
 $checks=@'
SELECT IF((SELECT breadsMask FROM battlepet_info WHERE NpcID=86447 AND Specie=1532)=256,'PASS native P/S mask','FAIL native P/S mask');
SELECT IF((SELECT breadsMask FROM battlepet_info WHERE NpcID=86448 AND Specie=1532)=512,'PASS other NPC retained','FAIL other NPC');
SELECT IF((SELECT breadsMask FROM battlepet_info WHERE NpcID=86447 AND Specie=1533)=512,'PASS other species retained','FAIL other species');
SELECT IF((SELECT COUNT(*) FROM battlepet_info b JOIN expected e USING(NpcID,Specie) WHERE b.maxlevel<>e.maxlevel OR b.minlevel<>e.minlevel OR b.minquality<>e.minquality OR b.Ability1<>e.Ability1 OR b.Ability2<>e.Ability2 OR b.Ability3<>e.Ability3 OR NOT(b.Comment<=>e.Comment))=0,'PASS other fields retained','FAIL other fields');
'@
 $sql=@'
CREATE TEMPORARY TABLE expected LIKE battlepet_info;
CREATE TEMPORARY TABLE battlepet_info LIKE expected;
INSERT INTO battlepet_info VALUES(86447,1532,25,1,2,512,1,2,3,'preserve'),(86448,1532,20,2,3,512,4,5,6,'other NPC'),(86447,1533,7,3,1,512,7,8,9,NULL);
INSERT INTO expected SELECT * FROM battlepet_info;
'@
 $sql+=$migration+$checks+$migration+$checks
 $sql+="UPDATE battlepet_info SET breadsMask=264 WHERE NpcID=86447 AND Specie=1532;"+$migration+"SELECT IF((SELECT breadsMask FROM battlepet_info WHERE NpcID=86447 AND Specie=1532)=264,'PASS custom mask retained','FAIL custom mask');DROP TEMPORARY TABLE battlepet_info;DROP TEMPORARY TABLE expected;"
 if($VerifyLive){$hash=(Get-FileHash "$repo/sql/updates/world/2026_10_27_15_ikky_native_breed.sql" -Algorithm SHA1).Hash;$sql+="SELECT IF((SELECT breadsMask FROM battlepet_info WHERE NpcID=86447 AND Specie=1532)=256,'PASS live P/S mask','FAIL live mask');SELECT IF((SELECT COUNT(*) FROM updates WHERE name='2026_10_27_15_ikky_native_breed.sql' AND state='RELEASED' AND hash='$hash')=1,'PASS native updater receipt/hash','FAIL updater receipt/hash');"}
 $old=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$oldErrors=$ErrorActionPreference;try{$ErrorActionPreference='Continue';$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names 2> "$out/stderr.txt");$code=$LASTEXITCODE}finally{$ErrorActionPreference=$oldErrors};if($code){Get-Content "$out/stderr.txt";throw 'Temporary data regression failed'};$rows;if($rows -match '^FAIL' -or (Get-Item "$out/stderr.txt").Length){throw 'Ikky data verification failed'}}finally{$env:MYSQL_PWD=$old}
}
