param([string]$Migration='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$Migration){$Migration="$repo/sql/pending/2026_10_31_01_sharasdal_breaking_bonds.sql";if(!(Test-Path $Migration)){$Migration="$repo/sql/updates/world/2026_10_31_01_sharasdal_breaking_bonds.sql"}}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-rescue-sql"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$migrationSql=[IO.File]::ReadAllText([IO.Path]::GetFullPath($Migration))
$settings=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$world=$settings[4];if($world -notmatch '^\w+$'){throw 'Unsupported source database identifier'}
$tables=@('creature_template','npc_spellclick_spells','scenario_data','creature','conditions','spell_script_names')
$filters=@{creature_template='entry=104853';npc_spellclick_spells='npc_entry=104853 OR spell_id=208171';scenario_data='MapID=1600';creature='id=104853';conditions='(SourceTypeOrReferenceId=18 AND SourceGroup=104853) OR (SourceTypeOrReferenceId=17 AND SourceEntry=208171)';spell_script_names='ABS(spell_id)=208171'}
$orders=@{creature_template='entry';npc_spellclick_spells='npc_entry,spell_id';scenario_data='ScenarioID,Team,Class';creature='guid';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference';spell_script_names='spell_id,ScriptName'}
$setup=(@($tables|ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_ WHERE $($filters[$_]); CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n")+"`nDELETE FROM spell_script_names WHERE spell_id=208171 AND ScriptName='spell_sharasdal_breaking_bonds'; INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(100,'codex_fixture_preserve');`n"
function Frame([string]$tag,[switch]$IncludeBindings){
 $queries=@("SELECT 'BEGIN','$tag';")
 foreach($table in $tables){if(!$IncludeBindings -and $table -eq 'spell_script_names'){continue};$queries+="SELECT '$table';SELECT * FROM $table ORDER BY $($orders[$table]);"}
 return ($queries+"SELECT 'END','$tag';") -join "`n"
}
function Extract([string]$value,[string]$tag){$begin="BEGIN`t$tag`n";$end="END`t$tag";$first=$value.IndexOf($begin);if($first -lt 0){throw "Missing frame $tag"};$first+=$begin.Length;$last=$value.IndexOf($end,$first);if($last -lt 0){throw "Undrained frame $tag"};$value.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -EvidenceDirectory "$out/reader" -Test {
 $readerArguments=@("--host=$($settings[0])","--port=$($settings[1])","--user=$env:CODEX_SQL_READER_USER","--database=$world",'--batch','--raw','--skip-column-names','--force')
 function Session([string]$sql,[string]$label,[switch]$Reject){
  $saved=$env:MYSQL_PWD
  try{
   $env:MYSQL_PWD=$env:CODEX_SQL_READER_PASSWORD
   [IO.File]::WriteAllText("$out/$label.sql",$sql+"`nSELECT 'EOF','$label';`n")
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $readerArguments -InputFile "$out/$label.sql" -OutputFile "$out/$label.tsv" -ErrorFile "$out/$label.stderr"
   $errorText=[IO.File]::ReadAllText("$out/$label.stderr");$value=[IO.File]::ReadAllText("$out/$label.tsv").Replace("`r",'')
   [IO.File]::WriteAllText("$out/$label.exit",[string]$code)
   if(!$value.Contains("EOF`t$label")){throw "Native final result missing: $label"}
   if($Reject){
    if($code -notin @(0,1)){throw "Unexpected native client exit after guard refusal: $label ($code)"}
    $errors=@($errorText -split "`n"|Where-Object {$_ -match '^ERROR '})
    if($errors.Count -ne 1 -or $errors[0] -notmatch 'ERROR 1644 \(45000\).*Sharasdal Breaking Bonds native context differs'){throw "Expected isolated guard refusal missing/unexplained failure: $label"}
   }elseif($code -or $errorText){throw "Native temporary SQL status/stderr failed: $label"}
   return $value
  }finally{$env:MYSQL_PWD=$saved}
 }
 $positive=$setup+(Frame 'before')+$migrationSql+(Frame 'after1')+(Frame 'binding1' -IncludeBindings)+$migrationSql+(Frame 'after2')+(Frame 'binding2' -IncludeBindings)
 $result=Session $positive 'valid-idempotence'
 if((Extract $result 'before') -cne (Extract $result 'after1') -or (Extract $result 'before') -cne (Extract $result 'after2')){throw 'Binding changed its data/actor context'}
 if((Extract $result 'binding1') -cne (Extract $result 'binding2')){throw 'Binding is not idempotent'}
 if($result -notmatch "(?m)^208171`tspell_sharasdal_breaking_bonds$" -or $result -notmatch "(?m)^100`tcodex_fixture_preserve$"){throw 'Own/unrelated binding not retained'}
 $mutations=@(
  "UPDATE creature_template SET ScriptName='codex_custom_actor';",
  'UPDATE creature_template SET npcflag=1;',
  'UPDATE npc_spellclick_spells SET cast_flags=0;',
  'UPDATE npc_spellclick_spells SET spell_id=208172;',
  'UPDATE npc_spellclick_spells SET npc_entry=104854;',
  'UPDATE scenario_data SET Class=8;',
  'UPDATE scenario_data SET ScenarioID=1067;',
  'UPDATE creature SET position_z=800 WHERE guid=362078;',
  'UPDATE creature SET position_z=250 WHERE guid=362094;',
  'UPDATE creature SET PhaseId=1 WHERE guid=362078;',
  "INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(208171,'codex_custom_spell');",
  "INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(-208171,'spell_sharasdal_breaking_bonds');"
 )
 for($caseIndex=0;$caseIndex -lt $mutations.Count;++$caseIndex){
  $result=Session ($setup+$mutations[$caseIndex]+(Frame 'before' -IncludeBindings)+$migrationSql+(Frame 'after' -IncludeBindings)) "rejected-$caseIndex" -Reject
  if((Extract $result 'before') -cne (Extract $result 'after')){throw "Rejected migration changed temporary custom/partial fixture: $caseIndex"}
 }
 'PASS SELECT+TEMP-only native reader with denied persistent-DML canary: one binding/idempotence/unrelated contexts preserved;12 explicit custom/partial refusals unchanged even under --force, with full final EOF/stderr/status capture. Every writable table is session-temporary; no characters or persistent World rows modified'
}
