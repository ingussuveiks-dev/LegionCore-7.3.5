param([Parameter(Mandatory=$true)][string]$RouteReport,[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force $out|Out-Null
$route=Get-Content -LiteralPath $RouteReport -Raw|ConvertFrom-Json
if(@($route.routes).Count -ne 36){throw 'Expected a fresh all36 route report'}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}|Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
function Export-Table([string]$table,[string]$fields,[string]$filter='1'){
 $pairs=($fields.Split(',')|ForEach-Object {"'$_',``$_``"}) -join ','
 $rows=@(& C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --default-character-set=utf8mb4 --batch --raw --skip-column-names --execute="SELECT JSON_OBJECT($pairs) FROM ``$table`` WHERE $filter")
 if($LASTEXITCODE){throw "Read-only export failed: $table"}
 [IO.File]::WriteAllLines("$out/$table.jsonl",[string[]]$rows,[Text.UTF8Encoding]::new($false))
 "$table $($rows.Count)"
}
try{
 $env:MYSQL_PWD=$c[3]
 Export-Table 'scenario_data' 'ScenarioID,MapID,DifficultyID,Team,Class,LfgDungeonID' 'Class>0'
 Export-Table 'instance_template' 'map,parent,script,allowMount,bonusChance'
 $scenarios=@(Get-Content "$out/scenario_data.jsonl"|ForEach-Object {$_|ConvertFrom-Json})
 $instances=@(Get-Content "$out/instance_template.jsonl"|ForEach-Object {$_|ConvertFrom-Json})
 $maps=[Collections.Generic.HashSet[int]]::new()
 foreach($row in $scenarios){[void]$maps.Add([int]$row.MapID)}
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
 $manifest=[ordered]@{Utc=[DateTime]::UtcNow.ToString('o');RouteSHA256=(Get-FileHash -LiteralPath $RouteReport).Hash;Maps=@($maps|Sort-Object);DirectCreatureEntries=@($creatures|Sort-Object);DirectGameObjectEntries=@($objects|Sort-Object);InstanceSources=$sources;Hashes=$hashes;Limits='Static world candidates from class-specific scenario_data plus artifact InstanceMapScript constructors/bindings and direct route actors. Shared maps include unrelated actors. Dynamic summons, phasing, runtime loader difficulty/conditions, C++ callbacks and client traversal require separate review. Read-only SELECT exports; no character data.'}
 [IO.File]::WriteAllText("$out/scope.json",($manifest|ConvertTo-Json -Depth 8),[Text.UTF8Encoding]::new($false))
 "Exported static candidate actors on $($maps.Count) maps plus $($creatures.Count) creature/$($objects.Count) object route entries; no live data changed"
}finally{$env:MYSQL_PWD=$old}
