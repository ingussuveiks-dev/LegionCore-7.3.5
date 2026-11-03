param([string]$MigrationFile,[switch]$VerifyOriginal,[switch]$VerifyLiveData,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/monk-intro-text/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"};$out=[IO.Path]::GetFullPath($OutputDirectory)
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_11_03_01_monk_intro_default_text.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_11_03_01_monk_intro_default_text.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($repo,$out) -Test {
 $context=Get-Content "$PSScriptRoot/MonkIntroTextOriginal.json" -Raw|ConvertFrom-Json;$migration=[IO.File]::ReadAllText($MigrationFile)
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$script:index=0
 function Query([string]$sql,[switch]$Refusal){
  $stem="$out/sql-$script:index";++$script:index;[IO.File]::WriteAllText("$stem.sql",$sql+"`nSELECT 'EOF','monk-intro-text';`n")
  $prior=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$connection[3];$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile "$stem.sql" -OutputFile "$stem.stdout" -ErrorFile "$stem.stderr"}finally{$env:MYSQL_PWD=$prior}
  $rows=@(Get-Content "$stem.stdout");$errors=@(Get-Content "$stem.stderr"|Where-Object {$_ -match '\S'})
  if($rows[-1] -cne "EOF`tmonk-intro-text"){throw 'Native SQL did not reach complete EOF'}
  if($Refusal){if($errors.Count -ne 4 -or $errors[0] -cne '--------------' -or $errors[1] -cne 'INSERT INTO monk_intro_text_guard VALUES(@monk_intro_text_safe)' -or $errors[2] -cne '--------------' -or $errors[3] -notmatch '^ERROR 4025 .*monk_intro_text_context'){throw 'Unexpected native SQL refusal diagnostic'}}elseif($code -or $errors.Count){throw 'Native SQL exit/stderr failure'}
  $rows
 }
 function Literal($v){if($null -eq $v){return 'NULL'};if($v -is [string]){if(!$v.Length){return "''"};return 'CONVERT(0x'+[Convert]::ToHexString([Text.Encoding]::UTF8.GetBytes($v))+' USING utf8mb4)'};[Convert]::ToString($v,[Globalization.CultureInfo]::InvariantCulture)}
 function Insert([string]$table,$row){'INSERT INTO '+$table+'('+[string]::Join(',',@($row.PSObject.Properties.Name|ForEach-Object {'`'+$_+'`'}))+') VALUES('+[string]::Join(',',@($row.PSObject.Properties.Value|ForEach-Object {Literal $_}))+');' + "`n"}
 $tables=@($context.tables.PSObject.Properties.Name);$setup=''
 $schema='SET SESSION group_concat_max_len=65536;';foreach($table in $tables){$schema+="SELECT 'SCHEMA','$table',GROUP_CONCAT(COLUMN_NAME ORDER BY ORDINAL_POSITION SEPARATOR ',') FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='$table';"}
 $schemaRows=Query $schema;foreach($table in $tables){$want=[string]::Join(',',@($context.tables.$table[0].PSObject.Properties.Name));if($schemaRows -notcontains "SCHEMA`t$table`t$want"){throw "Native full schema differs: $table"}}
 foreach($table in $tables){$setup+="CREATE TEMPORARY TABLE schema_$table LIKE $table;CREATE TEMPORARY TABLE $table LIKE schema_$table;DROP TEMPORARY TABLE schema_$table;`n";foreach($row in @($context.tables.$table)){$setup+=Insert $table $row}}
 $sentinel=$context.tables.quest_template[0]|ConvertTo-Json -Depth 5|ConvertFrom-Json;$sentinel.ID=1;$sentinel.LogTitle='Unrelated native sentinel';$setup+=Insert quest_template $sentinel
 $extraLocale=$context.tables.quest_template_locale[0]|ConvertTo-Json -Depth 5|ConvertFrom-Json;$extraLocale.locale='deDE';$extraLocale.LogTitle='Custom translation sentinel';$setup+=Insert quest_template_locale $extraLocale
 $expected='DELETE FROM quest_template WHERE ID=12103;'+(Insert quest_template $context.expected_quest)
 function Snapshot([string]$tag,[switch]$Scoped){$s='';foreach($table in $tables){$where=if($Scoped){' WHERE '+$context.filters.$table}else{''};$pairs=[string]::Join(',',@($context.tables.$table[0].PSObject.Properties.Name|ForEach-Object {"'$_',t.``$_``"}));$s+="SELECT '$tag-$table',JSON_OBJECT($pairs) FROM $table t$where ORDER BY $($context.orders.$table);`n"};$s}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 $expectedRows=Query ($setup+$expected+(Snapshot expected)+(Snapshot scoped -Scoped))
 $rows=Query ($setup+(Snapshot original -Scoped)+(Snapshot before)+$migration+(Snapshot first)+$migration+(Snapshot second))
 foreach($table in $tables){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table") -or (Signature $rows "first-$table") -cne (Signature $expectedRows "expected-$table")){throw "Full-column/idempotence differs: $table"};if($table -ne 'quest_template' -and (Signature $rows "before-$table") -cne (Signature $rows "first-$table")){throw "Other native context changed: $table"}}
 $variants=[ordered]@{
  'custom-default-title'="UPDATE quest_template SET LogTitle='Custom' WHERE ID=12103;"
  'default-case'='UPDATE quest_template SET LogTitle=LOWER(LogTitle) WHERE ID=12103;'
  'default-space'="UPDATE quest_template SET LogDescription=CONCAT(LogDescription,' ') WHERE ID=12103;"
  'partial-title'="UPDATE quest_template SET LogTitle='Before the Storm' WHERE ID=12103;"
  'partial-completion'="UPDATE quest_template SET QuestCompletionLog='' WHERE ID=12103;"
  'custom-base-description'="UPDATE quest_template SET QuestDescription='Custom' WHERE ID=12103;"
  'changed-native-locale'="UPDATE quest_template_locale SET QuestDescription='Custom' WHERE ID=12103;"
  'changed-locale-title'="UPDATE quest_template_locale SET LogTitle='Custom' WHERE ID=12103;"
  'empty-locale-summary'="UPDATE quest_template_locale SET LogDescription='' WHERE ID=12103;"
  'locale-completion'="UPDATE quest_template_locale SET QuestCompletionLog='Custom' WHERE ID=12103;"
  'locale-provenance'='UPDATE quest_template_locale SET VerifiedBuild=1 WHERE ID=12103;'
  'missing-locale'='DELETE FROM quest_template_locale WHERE ID=12103;'
  'missing-quest'='DELETE FROM quest_template WHERE ID=12103;'
  'changed-class'='UPDATE quest_template_addon SET AllowableClasses=1 WHERE ID=12103;'
  'changed-reward'='UPDATE quest_template SET RewardMoney=1 WHERE ID=12103;'
  'changed-quest-type'='UPDATE quest_template SET QuestType=1 WHERE ID=12103;'
  'changed-objective'='UPDATE quest_objectives SET Amount=2 WHERE QuestID=12103 AND ObjectID=98514;'
  'changed-storage'='UPDATE quest_objectives SET StorageIndex=9 WHERE QuestID=12103 AND ObjectID=98514;'
  'changed-giver'='UPDATE creature_queststarter SET id=1 WHERE quest=12103;'
  'changed-ender'='UPDATE creature_questender SET id=1 WHERE quest=12103;'
  'custom-base-portrait'="UPDATE quest_template SET PortraitGiverText='Custom' WHERE ID=12103;"
  'custom-locale-portrait'="UPDATE quest_template_locale SET PortraitGiverName='Custom' WHERE ID=12103;"
 }
 for($mask=1;$mask -lt 15;++$mask){$sets=@();for($bit=0;$bit -lt 4;++$bit){if($mask -band (1 -shl $bit)){$name=$context.changed_fields[$bit];$sets+="``$name``="+(Literal $context.expected_quest.$name)}};$variants["partial-image-$mask"]='UPDATE quest_template SET '+[string]::Join(',',$sets)+' WHERE ID=12103;'}
 foreach($name in $variants.Keys){$refused=Query ($setup+$variants[$name]+(Snapshot before)+$migration+(Snapshot after)) -Refusal;foreach($table in $tables){if((Signature $refused "before-$table") -cne (Signature $refused "after-$table")){throw "Refused $name changed $table"}}}
 if($VerifyOriginal -or $VerifyLiveData){$live=Query ('SET SESSION TRANSACTION READ ONLY;'+(Snapshot live -Scoped));foreach($table in $tables){$wanted=if($VerifyOriginal){Signature $rows "original-$table"}else{Signature $expectedRows "scoped-$table"};if((Signature $live "live-$table") -cne $wanted){throw "Live full native context differs: $table"}}}
 if($VerifyLiveData){$hasher=[Security.Cryptography.SHA1]::Create();try{$expectedHash=[Convert]::ToHexString($hasher.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n")))).ToLowerInvariant()}finally{$hasher.Dispose()};$name=[IO.Path]::GetFileName($MigrationFile);$updater=Query "SET SESSION TRANSACTION READ ONLY;SELECT 'UPDATER',name,hash,state FROM updates WHERE name='$name';";if($updater -notcontains "UPDATER`t$name`t$expectedHash`tRELEASED"){throw 'Actual native updater record/hash differs'}}
 "PASS native SQL:6 complete TEMP tables/full-column JSON snapshots, exact4 default-text cells, unchanged enUS/numeric/objectives/admission/sentinel, idempotence,$($variants.Count) unchanged expected refusals; original=$VerifyOriginal/live=$VerifyLiveData"
}
