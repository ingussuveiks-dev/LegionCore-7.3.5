param([string]$MigrationPath="$PSScriptRoot/../../sql/updates/world/2026_10_27_12_demons_among_us_pve_faction.sql",[switch]$VerifyLive)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/demons-among-us-pvp-data"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories $out -Test {
 $migration=Get-Content $MigrationPath -Raw
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $number=0
 function Query([string]$sql,[switch]$Expected){
  $script:number++;$stderr="$out/sql-stderr-$script:number.txt";$prior=$env:MYSQL_PWD;$priorErrors=$ErrorActionPreference
  try{$env:MYSQL_PWD=$db[3];$ErrorActionPreference='Continue';$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names --force 2> $stderr);$code=$LASTEXITCODE}finally{$env:MYSQL_PWD=$prior;$ErrorActionPreference=$priorErrors}
  $err=Get-Content $stderr -Raw
  if($Expected){if(@($err -split "`n"|Where-Object {$_ -match '^ERROR '}).Count -ne 1 -or $err -notmatch '1644 \(45000\)'){throw 'Expected scoped demon-faction refusal missing'}}elseif($code -or $err){throw 'Demon-faction TEMP/read-only query failed'}
  $rows
 }
 $filters=[ordered]@{creature_template='entry IN(101100,101104,100993,101059,102543,101146)';creature_template_wdb='Entry IN(101100,101104,100993,101059,102543,101146)';creature='id IN(101100,101104,100993,101059,102543,101146)';quest_objectives='QuestID IN(40607,40593)';creature_template_spell='entry IN(101100,101104,100993,101059,102543,101146)'}
 $setup='';foreach($t in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$t LIKE $t;INSERT INTO saved_$t SELECT * FROM $t WHERE $($filters[$t]);CREATE TEMPORARY TABLE $t LIKE saved_$t;INSERT INTO $t SELECT * FROM saved_$t;"}
 $setup+='UPDATE creature_template SET faction=2780 WHERE entry IN(101100,101104);'
 function Snapshot([string]$label){$sql='';foreach($t in $filters.Keys){$sql+="SELECT '$label-$t',a.* FROM $t a;"};$sql}
 function Signature($rows,[string]$prefix){(@($rows|Where-Object {$_ -like "$prefix`t*"}|ForEach-Object {$_.Substring($prefix.Length+1)}|Sort-Object -CaseSensitive)) -join "`n"}
 $r=Query "$setup $(Snapshot 'before') $migration $(Snapshot 'first') $migration $(Snapshot 'second') SELECT 'pve',entry,faction FROM creature_template WHERE entry IN(101100,101104) ORDER BY entry;UPDATE creature_template SET faction=2780 WHERE entry IN(101100,101104);$(Snapshot 'normalized')"
 foreach($t in $filters.Keys){if((Signature $r "first-$t") -cne (Signature $r "second-$t")){throw 'Demon faction migration not idempotent'};if((Signature $r "before-$t") -cne (Signature $r "normalized-$t")){throw "Unrelated demon-faction data changed: $t"}}
 foreach($row in @("pve`t101100`t2898","pve`t101104`t2898")){if($r -notcontains $row){throw 'Both PvE factions not installed'}}
 $changes=@("UPDATE creature_template SET ScriptName='custom' WHERE entry=101100;",'UPDATE creature_template SET faction=14 WHERE entry=101104;','UPDATE creature_template SET unit_flags=0 WHERE entry=101104;','DELETE FROM creature_template WHERE entry=101100;','UPDATE creature_template_wdb SET KillCredit1=0 WHERE Entry=101104;',"UPDATE creature SET PhaseId='1' WHERE guid=350953;",'DELETE FROM creature WHERE guid=351005;','UPDATE quest_objectives SET Amount=13 WHERE QuestID=40607 AND ObjectID=101105;')
 foreach($applied in @($false,$true)){
  $prior=if($applied){$migration}else{''}
  foreach($change in $changes){$rows=Query "$setup $prior $change $(Snapshot 'before') $migration $(Snapshot 'after')" -Expected;foreach($t in $filters.Keys){if((Signature $rows "before-$t") -cne (Signature $rows "after-$t")){throw 'Refused demon-faction migration mutated data'}}}
 }
 if($VerifyLive){
  $sha=[Security.Cryptography.SHA1]::Create();try{$hash=[BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes(([IO.File]::ReadAllText((Resolve-Path $MigrationPath))).Replace("`r`n","`n")))).Replace('-','')}finally{$sha.Dispose()}
  $rows=Query "SET SESSION TRANSACTION READ ONLY;SELECT 'pve',entry,faction FROM creature_template WHERE entry IN(101100,101104);SELECT 'credit',Entry,KillCredit1 FROM creature_template_wdb WHERE Entry IN(101100,101104);SELECT 'objective',Amount FROM quest_objectives WHERE QuestID=40607 AND Type=0 AND ObjectID=101105;SELECT 'update',hash,state FROM updates WHERE name='2026_10_27_12_demons_among_us_pve_faction.sql';"
  foreach($row in @("pve`t101100`t2898","pve`t101104`t2898","credit`t101100`t101105","credit`t101104`t101105","objective`t12","update`t$hash`tRELEASED")){if($rows -notcontains $row){throw "Live demon faction contract differs: $row"}}
 }
 'PASS five actual-schema TEMP tables/repeat/full-field preservation and16 original/applied custom refusals; no player PvP/progress writes'
}
