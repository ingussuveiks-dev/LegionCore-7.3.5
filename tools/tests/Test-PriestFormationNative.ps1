param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/priest-formation-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force "$out/logs"|Out-Null
$context=Get-Content "$PSScriptRoot/PriestFormationContext.json" -Raw|ConvertFrom-Json -AsHashtable
$expected=Get-Content "$PSScriptRoot/PriestFormationExpected.json" -Raw|ConvertFrom-Json -AsHashtable
$effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
if($effects -notmatch '&Spell::EffectKillCreditPersonal,\s*// 90 SPELL_EFFECT_KILL_CREDIT'){throw 'Native90 personal dispatch changed; reassess credit audience'}
function Value($v){if($null -eq $v){return 'NULL'};if($v -is [string]){if(!$v.Length){return "''"};return 'CONVERT(0x'+[Convert]::ToHexString([Text.Encoding]::UTF8.GetBytes($v))+' USING utf8mb4)'};[Convert]::ToString($v,[Globalization.CultureInfo]::InvariantCulture)}
function HashQuery($table){$columns=$context.columns[$table];$values=($columns|ForEach-Object{"COALESCE(CAST(``$_`` AS CHAR CHARACTER SET utf8mb4),'NULL')"})-join ',';$order=($columns|ForEach-Object{"``$_``"})-join ',';"SELECT SHA2(COALESCE(GROUP_CONCAT(CONCAT_WS('|',$values) ORDER BY $order SEPARATOR '\n'),''),256) FROM $table WHERE $($context.filters[$table]);"}
$questSource=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$query=[regex]::Match($questSource,'(?s)void QuestDataStoreMgr::LoadQuests\(\).*?WorldDatabase.Query\(("SELECT ".*?" FROM quest_template")\);').Groups[1].Value
if(!$query){throw 'Native quest loader SELECT unavailable'}
[IO.File]::WriteAllText("$out/NativePriestQuest.inc",$query+' " WHERE ID IN(40938,41015,41019)"')
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/diagnostics" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -EvidenceDirectory "$out/reader" -Test {
  $config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
  $parts=([regex]::Match($config,'(?m)^WorldDatabaseInfo\s*=\s*"([^"]+)"')).Groups[1].Value.Split(';')
  if($parts.Count -ne 5 -or $parts[4] -notmatch '^\w+$'){throw 'Unexpected private World configuration'}
  $hostName=$parts[0];$port=$parts[1];$original=$parts[4];$clone='codex_priest_'+[guid]::NewGuid().ToString('N').Substring(0,16);$created=$false
  $mysql='C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe';$oldPassword=$env:MYSQL_PWD
  function SqlFile($sql,$label,[switch]$Admin,[string]$Database=$original){
   $env:MYSQL_PWD=if($Admin){$env:CODEX_MYSQL_ADMIN_PASSWORD}else{$env:CODEX_SQL_READER_PASSWORD}
   [IO.File]::WriteAllText("$out/$label.sql",$sql)
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable $mysql -Arguments @("--host=$hostName","--port=$port",('--user='+$(if($Admin){'root'}else{$env:CODEX_SQL_READER_USER})),"--database=$Database",'--batch','--raw','--skip-column-names','--default-character-set=utf8mb4') -InputFile "$out/$label.sql" -OutputFile "$out/$label.tsv" -ErrorFile "$out/$label.stderr"
   if($code -ne 0 -or (Get-Item "$out/$label.stderr").Length){throw "Isolated native SQL failed: $label"}
   [IO.File]::ReadAllLines("$out/$label.tsv")
  }
  try{
   $tables=@(SqlFile "SELECT TABLE_NAME FROM information_schema.tables WHERE TABLE_SCHEMA='$original' ORDER BY TABLE_NAME;" tables)
   if(!$tables.Count -or @($tables|Where-Object{$_ -notmatch '^\w+$'}).Count){throw 'Unexpected World table names'}
   $physical=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
   foreach($name in $context.filters.Keys){[void]$physical.Add($name)}
   # Keep the exact native preparation, including its DML metadata checks. Each
   # prepared DML target is a real table inside the disposable schema; other
   # tables are invoker-security views with SELECT-only production permission.
   $world=Get-Content "$repo/src/server/database/Database/Implementation/WorldDatabase.cpp" -Raw
   foreach($match in [regex]::Matches($world,'PrepareStatement\([^,]+, "([^"\r\n]+)"')){
    $sql=$match.Groups[1].Value
    if($sql -match '^(?:DELETE\s+FROM|INSERT\s+(?:IGNORE\s+)?INTO|REPLACE(?:\s+INTO)?|UPDATE)\s+`?([A-Za-z0-9_]+)'){[void]$physical.Add($Matches[1])}
    elseif($sql -notmatch '^SELECT '){throw 'Unreviewed native World statement kind'}
   }
   foreach($table in $physical){if($table -notin $tables){throw "Missing native DML table: $table"}}
   SqlFile ('CREATE DATABASE `{0}` CHARACTER SET utf8mb4;' -f $clone) create-clone -Admin|Out-Null;$created=$true
   SqlFile ('GRANT ALL PRIVILEGES ON `{0}`.* TO ''{1}''@''localhost'';' -f $clone,$env:CODEX_SQL_READER_USER) grant-clone -Admin|Out-Null
   $ddl='';foreach($table in $tables){
    if($physical.Contains($table)){$ddl+=('CREATE TABLE `{0}`.`{1}` LIKE `{2}`.`{1}`;' -f $clone,$table,$original)+"`n";if($table -notin @('creature','gameobject','smart_scripts')){$ddl+=('INSERT INTO `{0}`.`{1}` SELECT * FROM `{2}`.`{1}`;' -f $clone,$table,$original)+"`n"}}
    else{$ddl+=('CREATE SQL SECURITY INVOKER VIEW `{0}`.`{1}` AS SELECT * FROM `{2}`.`{1}`;' -f $clone,$table,$original)+"`n"}
   }
   SqlFile $ddl clone-schema -Admin|Out-Null
   $seed='SET SESSION group_concat_max_len=1048576;'+"`n"
   foreach($table in $context.filters.Keys){
    $seed+="DELETE FROM $table WHERE $($context.filters[$table]);`n";$columns=($context.columns[$table]|ForEach-Object{"``$_``"})-join ','
    foreach($row in $expected[$table]){$values=($context.columns[$table]|ForEach-Object{Value $row[$_]})-join ',';$seed+="INSERT INTO $table($columns) VALUES($values);`n"}
   }
   SqlFile $seed applied-fixture -Database $clone|Out-Null
   $snapshot="SET SESSION group_concat_max_len=1048576;`n"+[string]::Join("`n",@($context.filters.Keys|ForEach-Object{HashQuery $_}))
   $before=@(SqlFile $snapshot production-before)
   $checks='';foreach($table in $context.filters.Keys){
    $query=HashQuery $table;$columns=($context.columns[$table]|ForEach-Object{"``$_``"})-join ','
    $expectedSql="SET SESSION group_concat_max_len=1048576;`nCREATE TEMPORARY TABLE priest_hash_$table LIKE $table;`n"
    foreach($row in $expected[$table]){$values=($context.columns[$table]|ForEach-Object{Value $row[$_]})-join ',';$expectedSql+="INSERT INTO priest_hash_$table($columns) VALUES($values);`n"}
    $expectedSql+=$query.Replace(" FROM $table WHERE"," FROM priest_hash_$table WHERE")
    $hash=@(SqlFile $expectedSql "expected-$table")
    if($hash.Count -ne 1 -or $hash[0] -notmatch '^[0-9a-f]{64}$'){throw 'Independent expected context hash missing'}
    $actual=@(SqlFile ('SET SESSION group_concat_max_len=1048576;'+$query) "clone-$table" -Database $clone)
    if($actual.Count -ne 1 -or $actual[0] -cne $hash[0]){throw "Clone differs from independently pinned25-table fixture: $table"}
    $checks+="$($hash[0])`t$query`n"
   }
   [IO.File]::WriteAllText("$out/checks.tsv",$checks);[IO.File]::WriteAllText("$out/setup.sql","SET SESSION group_concat_max_len=1048576;`n")
   $parts[2]=$env:CODEX_SQL_READER_USER;$parts[3]=$env:CODEX_SQL_READER_PASSWORD;$parts[4]=$clone
   $config=[regex]::Replace($config,'(?m)^WorldDatabaseInfo\s*=.*$',('WorldDatabaseInfo = "'+($parts -join ';')+'"'))
   $config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
   $config+="`nLogger.spells=2,Server`nLogger.maps=2,Server`n"
   [IO.File]::WriteAllText("$out/worldserver.conf",$config)
   [ordered]@{schema=$clone;physical_tables=$physical.Count;readonly_views=$tables.Count-$physical.Count;production=$original;character_rows_copied=0}|ConvertTo-Json|Set-Content "$out/isolation.json"
   $previous=$env:LEGION_DIALOGUE_CONFIG;$priorSetup=$env:LEGION_PRIEST_SETUP;$priorChecks=$env:LEGION_PRIEST_CHECKS;$compiler=$env:CL;$link=$env:_LINK_
   try{
    $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:LEGION_PRIEST_SETUP="$out/setup.sql";$env:LEGION_PRIEST_CHECKS="$out/checks.tsv"
    $env:CL="$compiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`"";$env:_LINK_="$link /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
    Push-Location "$repo/build-extractors/bin/Release"
    try{& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/PriestFormationNative.cpp -ProbeName PriestFormationNative -OutputFile "$out/native.tsv" *> "$out/compiler.log";if(!$? -or $LASTEXITCODE){throw "Native Priest sequence failed; evidence retained: $out"}}finally{Pop-Location}
   }finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:LEGION_PRIEST_SETUP=$priorSetup;$env:LEGION_PRIEST_CHECKS=$priorChecks;$env:CL=$compiler;$env:_LINK_=$link}
   Get-Content "$out/native.tsv"|Where-Object{$_ -match '^(PASS|STATE)'}
  }finally{
   try{
    try{if($null -ne $before){$after=@(SqlFile $snapshot production-after);if(($before-join "`n") -cne ($after-join "`n")){throw 'Production context changed during isolated native test'}}}
    finally{if($created){if($clone -notmatch '^codex_priest_[a-f0-9]{16}$' -or $clone -eq $original){throw 'Refusing unexpected clone cleanup target'};SqlFile ('DROP DATABASE `{0}`;' -f $clone) drop-clone -Admin|Out-Null;'PASS disposable World clone removed; native diagnostics retained, no production DML permission or character rows copied'}}
   }finally{$env:MYSQL_PWD=$oldPassword}
  }
 }
}
