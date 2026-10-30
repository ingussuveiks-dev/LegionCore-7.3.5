param([Parameter(Mandatory=$true)][string]$RouteReport,[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force $out|Out-Null
$route=Get-Content -LiteralPath $RouteReport -Raw|ConvertFrom-Json
if(@($route.routes).Count -ne 36){throw 'Expected a fresh all36 route report'}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}|Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
. "$PSScriptRoot/ArtifactMapScope.ps1"
function Export-Table([string]$table,[string]$fields,[string]$filter='1'){
 $pairs=($fields.Split(',')|ForEach-Object {"'$_',``$_``"}) -join ','
 $inputPath="$out/.query-$table.sql";$stdoutPath="$out/.query-$table.stdout";$stderrPath="$out/.query-$table.stderr"
 [IO.File]::WriteAllText($inputPath,"SET SESSION TRANSACTION READ ONLY;START TRANSACTION WITH CONSISTENT SNAPSHOT;SELECT JSON_OBJECT($pairs) FROM ``$table`` WHERE $filter;SELECT CONCAT('__ARTIFACT_EXPORT_ROWS__',COUNT(*)) FROM ``$table`` WHERE $filter;COMMIT;",[Text.UTF8Encoding]::new($false))
 $code=& "$PSScriptRoot/../tests/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names') -InputFile $inputPath -OutputFile $stdoutPath -ErrorFile $stderrPath
 if($code -ne 0 -or (Get-Item -LiteralPath $stderrPath).Length){throw "Read-only native actor export failed: $stderrPath"}
 $count=Publish-ArtifactActorRows $stdoutPath "$out/$table.jsonl"
 "$table $count"
}
function Publish-ArtifactActorRows([string]$stdoutPath,[string]$destinationPath){
 $count=0;$reported=$null;$trailer=$null
 $reader=[IO.File]::OpenText($stdoutPath)
 try{while($null -ne ($line=$reader.ReadLine())){
  if($null -ne $reported){throw "Native actor bytes after row count: $stdoutPath"}
  if($line -match '^__ARTIFACT_EXPORT_ROWS__(\d+)$'){$reported=[long]$Matches[1];$trailer=$line;continue}
  $data=$line|ConvertFrom-Json
  if($data -isnot [pscustomobject]){throw "Invalid native actor JSON: $stdoutPath"};++$count
 }}finally{$reader.Dispose()}
 if($null -eq $reported -or $count -ne $reported){throw "Native actor row count differs: $stdoutPath"}
 $bytes=[IO.File]::ReadAllBytes($stdoutPath);$eol=if($bytes.Length -ge 2 -and $bytes[-2] -eq 13 -and $bytes[-1] -eq 10){2}elseif($bytes[-1] -eq 10){1}else{throw 'Native actor trailer lacks terminator'}
 $size=[Text.Encoding]::UTF8.GetByteCount($trailer);$prefix=$bytes.Length-$size-$eol
 if([Text.Encoding]::UTF8.GetString($bytes,$prefix,$size) -cne $trailer){throw 'Native actor trailer bytes differ'}
 $file=[IO.File]::Open($destinationPath,[IO.FileMode]::Create,[IO.FileAccess]::Write,[IO.FileShare]::Read)
 try{$file.Write($bytes,0,$prefix)}finally{$file.Dispose()}
 return $count
}
try{
 $env:MYSQL_PWD=$c[3]
 Export-Table 'scenario_data' 'ScenarioID,MapID,DifficultyID,Team,Class,LfgDungeonID'
 Export-Table 'instance_template' 'map,parent,script,allowMount,bonusChance'
 $scenarios=@(Get-Content "$out/scenario_data.jsonl"|ForEach-Object {$_|ConvertFrom-Json})
 $instances=@(Get-Content "$out/instance_template.jsonl"|ForEach-Object {$_|ConvertFrom-Json})
 $maps=[Collections.Generic.HashSet[int]]::new()
 foreach($row in $scenarios){if([int]$row.Class -gt 0 -and [int]$row.MapID -gt 0){[void]$maps.Add([int]$row.MapID)}}
 $explicit=Get-ArtifactExplicitScenarioScope $route $scenarios
 foreach($map in $explicit.Maps){[void]$maps.Add([int]$map)}
 $sources=@()
 foreach($file in @(Get-ChildItem "$repo/src/server/scripts/Scenario/Artifacts" -Recurse -File -Filter '*.cpp')){
  $text=[IO.File]::ReadAllText($file.FullName)
  foreach($match in [regex]::Matches($text,'InstanceMapScript\("([a-zA-Z0-9_]+)"\s*,\s*([^\)]+)\)')){
   $script=$match.Groups[1].Value;$expression=$match.Groups[2].Value.Trim()
   $literal=0
   if([int]::TryParse($expression,[ref]$literal)){[void]$maps.Add($literal)}
   $bound=@($instances|Where-Object {$_.script -ceq $script}|ForEach-Object {[int]$_.map})
   foreach($map in $bound){[void]$maps.Add($map)}
   $sources+=@{Path=$file.FullName.Substring($repo.Length+1).Replace('\','/');SHA256=(Get-FileHash -LiteralPath $file.FullName).Hash;Script=$script;MapExpression=$expression;BoundMaps=$bound}
  }
 }
 $creatures=[Collections.Generic.HashSet[int]]::new();$objects=[Collections.Generic.HashSet[int]]::new()
 foreach($r in $route.routes){
  foreach($q in $r.quest_details){
   foreach($relation in @($q.starters,$q.enders)){
    foreach($id in @($relation.creature_queststarter)+@($relation.creature_questender)){if($id){[void]$creatures.Add([int]$id)}}
    foreach($id in @($relation.gameobject_queststarter)+@($relation.gameobject_questender)){if($id){[void]$objects.Add([int]$id)}}
   }
   foreach($o in $q.objectives){if($o.Type -eq 0 -and $o.ObjectID -gt 0){[void]$creatures.Add([int]$o.ObjectID)}}
  }
  foreach($o in $r.pickup_objects){[void]$objects.Add([int]$o.entry)}
 }
 $mapList=(@($maps)|Sort-Object) -join ','
 $creatureList=(@($creatures)|Sort-Object) -join ',';$objectList=(@($objects)|Sort-Object) -join ','
 if(!$mapList -or !$creatureList -or !$objectList){throw 'Incomplete map/actor scope'}
 Export-Table 'creature' 'guid,id,map,PhaseId,phaseMask,spawnMask' "map IN($mapList) OR id IN($creatureList)"
 Export-Table 'gameobject' 'guid,id,map,PhaseId,phaseMask,spawnMask' "map IN($mapList) OR id IN($objectList)"
 $hashes=@{}
 foreach($file in Get-ChildItem -LiteralPath $out -Filter '*.jsonl'){$hashes[$file.Name]=(Get-FileHash -LiteralPath $file.FullName).Hash}
 $manifest=[ordered]@{Utc=[DateTime]::UtcNow.ToString('o');RouteSHA256=(Get-FileHash -LiteralPath $RouteReport).Hash;Maps=@($maps|Sort-Object);ExplicitScenarioSources=$explicit.Sources;UnmappedScenarioSources=$explicit.Unmapped;DirectCreatureEntries=@($creatures|Sort-Object);DirectGameObjectEntries=@($objects|Sort-Object);InstanceSources=$sources;Hashes=$hashes;Limits='Static candidates from class-specific scenario_data, route-specific explicit SmartAI204 selectors matched to all scenario_data/native LFG maps, artifact InstanceMapScript bindings and direct route actors. Positive map IDs only; unmatched selectors retained. Shared maps include unrelated actors; listed maps do not prove traversal. Native SQL fully drained/per-query flushed/server-count checked with exact stdout-prefix publication. Dynamic summons, phases/conditions/difficulty/C++ callbacks and client traversal require separate review. Read-only SELECT exports; no character data.'}
 [IO.File]::WriteAllText("$out/scope.json",($manifest|ConvertTo-Json -Depth 8),[Text.UTF8Encoding]::new($false))
 "Exported static candidate actors on $($maps.Count) maps plus $($creatures.Count) creature/$($objects.Count) object route entries; no live data changed"
}finally{$env:MYSQL_PWD=$old}
