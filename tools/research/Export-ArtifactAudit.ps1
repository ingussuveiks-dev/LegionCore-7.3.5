param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/artifact-audit/world")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $out | Out-Null
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
function Read-AuditQuery([string]$query,[string]$label){
 $inputPath=Join-Path $out ".query-$label.sql";$stdoutPath=Join-Path $out ".query-$label.stdout";$stderrPath=Join-Path $out ".query-$label.stderr"
 [IO.File]::WriteAllText($inputPath,"SET SESSION TRANSACTION READ ONLY;`n$query;`n",[Text.UTF8Encoding]::new($false))
 # Native pipeline capture intermittently returned exit0 with a cut final row.
 # Wait for redirected native files before reading or publishing the snapshot.
 $process=Start-Process -FilePath C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe -ArgumentList @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names') -RedirectStandardInput $inputPath -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath -WindowStyle Hidden -Wait -PassThru
 try{$process.Refresh();$code=$process.ExitCode}finally{$process.Dispose()}
 $errors=[IO.File]::ReadAllText($stderrPath,[Text.Encoding]::UTF8)
 if($code -ne 0 -or $errors){throw "Native audit query failed ($code): $stderrPath"}
 return $stdoutPath
}
function Export-Table($table,$fields,$filter='1'){
 $pairs=($fields.Split(',') | ForEach-Object {"'$_',``$_``"}) -join ','
 $sql="SELECT JSON_OBJECT($pairs) FROM ``$table`` WHERE $filter"
 $stdoutPath=Read-AuditQuery $sql $table
 $count=0
 foreach($row in [IO.File]::ReadLines($stdoutPath,[Text.Encoding]::UTF8)){
  ++$count
  if(!$row.StartsWith('{') -or !$row.EndsWith('}')){throw "Incomplete audit JSON row $table/$count in $stdoutPath"}
 }
 Copy-Item -LiteralPath $stdoutPath -Destination (Join-Path $out ($table+'.jsonl')) -Force
 "$table $count"
}
try {
 $env:MYSQL_PWD=$c[3]
 Export-Table 'quest_template' 'ID,QuestType,MinLevel,RewardNextQuest,QuestSortID,QuestInfoID,StartItem,RewardSpell,RewardItem1,RewardItem2,RewardItem3,RewardItem4,AllowableRaces,Flags,FlagsEx,StartScript,CompleteScript'
 Export-Table 'quest_template_addon' 'ID,AllowableClasses,PrevQuestID,NextQuestID,ExclusiveGroup,SourceSpellID,SpecialFlags'
 Export-Table 'quest_template_locale' 'ID,LogTitle,LogDescription,QuestDescription,QuestCompletionLog' "locale='enUS'"
 Export-Table 'quest_objectives' 'ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,Description'
 foreach($table in @('creature_queststarter','creature_questender','gameobject_queststarter','gameobject_questender')){Export-Table $table 'id,quest'}
 Export-Table 'scenario_data' 'ScenarioID,MapID,DifficultyID,Team,Class,LfgDungeonID'
 Export-Table 'creature_template' 'entry,AIName,ScriptName,npcflag,unit_flags,faction'
 Export-Table 'gameobject_template' 'entry,type,Data0,Data1,Data2,Data3,Data4,Data5,AIName,ScriptName'
 Export-Table 'gameobject_loot_template' 'Entry,Item,Currency,Reference,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount'
 Export-Table 'spell_script_names' 'spell_id,ScriptName'
 Export-Table 'spell_target_position' 'id,target_map,target_position_x,target_position_y,target_position_z,target_orientation'
 Export-Table 'npc_spellclick_spells' 'npc_entry,spell_id,cast_flags,user_type'
 Export-Table 'battlepay_product_item' 'ID,ProductID,ItemID,Quantity'
 foreach($table in @('smart_scripts','conditions','playerchoice_response','playerchoice_response_reward')){
  $columnsPath=Read-AuditQuery "SELECT COLUMN_NAME FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='$table' ORDER BY ORDINAL_POSITION" "columns-$table"
  $cols=@(Get-Content -LiteralPath $columnsPath -Encoding UTF8)
  if(!$cols.Count -or @($cols|Where-Object {$_ -notmatch '^\w+$'}).Count){throw "Incomplete audit column query: $table"}
  Export-Table $table ($cols -join ',')
 }
}finally{$env:MYSQL_PWD=$old}
