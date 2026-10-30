function Get-ArtifactExplicitScenarioScope($Route,$Scenarios){
 $maps=[Collections.Generic.HashSet[int]]::new();$sources=@();$unmapped=@()
 foreach($r in $Route.routes){
  foreach($action in $r.scenario_entry_actions){
   $matched=@($Scenarios|Where-Object {[int]$_.ScenarioID -eq [int]$action.scenario -and [int]$_.MapID -gt 0}|ForEach-Object {[int]$_.MapID})
   $native=@($action.native_routes|Where-Object {[int]$_.map -gt 0}|ForEach-Object {[int]$_.map})
   $selected=@(@($matched)+@($native)|Sort-Object -Unique)
   foreach($map in $selected){[void]$maps.Add($map)}
   $source=[ordered]@{Item=[int]$r.item;Class=[int]$r.class_id;Entry=[int]$action.entry;Event=[int]$action.event;Scenario=[int]$action.scenario;ScenarioDataMaps=$matched;NativeLfgMaps=$native;Maps=$selected}
   $sources+=$source
   if(!$selected.Count){$unmapped+=$source}
  }
 }
 @{Maps=@($maps|Sort-Object);Sources=$sources;Unmapped=$unmapped}
}
