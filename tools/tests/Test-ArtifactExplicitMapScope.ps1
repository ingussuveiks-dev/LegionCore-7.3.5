param([string]$RouteReport,[string]$ActorDirectory,[string]$PreviousDirectory,[string]$PreviousReport)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-explicit-map-scope"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 . "$repo/tools/research/ArtifactMapScope.ps1"
 $synthetic=@{routes=@(@{item=128820;class_id=8;scenario_entry_actions=@(@{entry=99415;event=2;scenario=957;native_routes=@()})},@{item=128292;class_id=6;scenario_entry_actions=@(@{entry=101441;event=3;scenario=901;native_routes=@(@{map=1480})})},@{item=128942;class_id=9;scenario_entry_actions=@(@{entry=101456;event=13;scenario=962;native_routes=@(@{map=1522})})},@{item=128832;class_id=12;scenario_entry_actions=@(@{entry=1;event=0;scenario=12345;native_routes=@(@{map=1583})})},@{item=1;class_id=1;scenario_entry_actions=@(@{entry=2;event=0;scenario=9999;native_routes=@(@{map=0})})})}
 $rows=@(@{ScenarioID=957;MapID=1480;Class=0},@{ScenarioID=901;MapID=1480;Class=0},@{ScenarioID=9999;MapID=0;Class=0})
 $scope=Get-ArtifactExplicitScenarioScope $synthetic $rows
 if(($scope.Maps -join ',') -cne '1480,1522,1583' -or $scope.Sources.Count -ne 5 -or $scope.Unmapped.Count -ne 1 -or $scope.Unmapped[0].Scenario -ne 9999){throw 'Explicit selector/native-only/dedup/zero-map/unmapped contract differs'}
 if(@($rows|Where-Object {$_.Class -gt 0}).Count){throw 'Expected class-only counterfactual to miss these maps'}
 $objects=0
 if($RouteReport -or $ActorDirectory -or $PreviousDirectory -or $PreviousReport){
 if(!$RouteReport -or !$ActorDirectory -or !$PreviousDirectory -or !$PreviousReport){throw 'Supply all four paths to validate captured all36 inputs'}
 $route=Get-Content $RouteReport -Raw|ConvertFrom-Json
 $manifest=Get-Content "$ActorDirectory/scope.json" -Raw|ConvertFrom-Json
 $previous=Get-Content "$PreviousDirectory/scope.json" -Raw|ConvertFrom-Json
 $scenarios=@(Get-Content "$ActorDirectory/scenario_data.jsonl"|ForEach-Object {$_|ConvertFrom-Json})
 $actual=Get-ArtifactExplicitScenarioScope $route $scenarios
 if(@($route.routes).Count -ne 36 -or $manifest.RouteSHA256 -cne (Get-FileHash $RouteReport).Hash){throw 'Capture is not from this exact36 route report'}
 foreach($map in @(1480,1522,1583)){
  if($map -in $previous.Maps -or $map -notin $actual.Maps -or $map -notin $manifest.Maps){throw "Old/new real shared-map counterfactual differs: $map"}
 }
 foreach($map in $actual.Maps){if($map -notin $manifest.Maps){throw 'Explicit scenario map lost from final constructor/class union'}}
 if($actual.Unmapped.Count -or $manifest.UnmappedScenarioSources.Count){throw 'Current explicit selector map remains unresolved'}
 $oldAudit=Get-Content $PreviousReport -Raw|ConvertFrom-Json
 $newAudit=Get-Content "$ActorDirectory/map-links.json" -Raw|ConvertFrom-Json
 $oldKeys=@($oldAudit.review_findings|ForEach-Object {"$($_.owner_source):$($_.owner_entry):$($_.event):$($_.link):$($_.reason)"})
 $newKeys=@($newAudit.review_findings|ForEach-Object {"$($_.owner_source):$($_.owner_entry):$($_.event):$($_.link):$($_.reason)"})
 foreach($key in $oldKeys){if($key -notin $newKeys){throw "Existing review finding disappeared: $key"}}
 $added=@($newKeys|Where-Object {$_ -notin $oldKeys}|Sort-Object)
 $expected=@('0:93949:5:3:invalid_target_event','0:99615:21:22:missing_target','0:99615:31:32:missing_target','0:104832:0:3:invalid_target_event')
 if($oldKeys.Count -ne 19 -or $newKeys.Count -ne 23 -or @(Compare-Object $added $expected).Count){throw 'Expected four newly visible primary-link review candidates'}
 $tables=@('scenario_data','instance_template','creature','gameobject');$objects=0
 foreach($table in $tables){
  $path="$ActorDirectory/$table.jsonl";$native="$ActorDirectory/.query-$table.stdout"
  if((Get-Item "$ActorDirectory/.query-$table.stderr").Length){throw 'Native export stderr is not empty'}
  $data=@(Get-Content $path -Encoding UTF8);$reported=([IO.File]::ReadLines($native)|Select-Object -Last 1)
  if($reported -notmatch '^__ARTIFACT_EXPORT_ROWS__(\d+)$' -or [long]$Matches[1] -ne $data.Count){throw "Native counted actor rows differ: $table"}
  foreach($line in $data){if(($line|ConvertFrom-Json) -isnot [pscustomobject]){throw 'Actor JSON is incomplete'}}
  $stream=[IO.File]::OpenRead($native);$hash=[Security.Cryptography.SHA256]::Create()
  try{$prefix=[IO.MemoryStream]::new();$size=(Get-Item $path).Length;$buffer=[byte[]]::new(81920);while($size -gt 0){$read=$stream.Read($buffer,0,[int][Math]::Min($size,$buffer.Length));if(!$read){throw 'Native actor prefix incomplete'};$prefix.Write($buffer,0,$read);$size-=$read};$prefix.Position=0;$digest=[Convert]::ToHexString($hash.ComputeHash($prefix));if($digest -cne (Get-FileHash $path).Hash){throw 'Published actor bytes differ from native prefix'}}finally{$stream.Dispose();$hash.Dispose();if($prefix){$prefix.Dispose()}}
  $objects+=$data.Count
 }
 }
 $tokens=$null;$errors=$null;$ast=[Management.Automation.Language.Parser]::ParseFile("$repo/tools/research/Export-ArtifactMapActors.ps1",[ref]$tokens,[ref]$errors)
 if($errors.Count){throw 'Actor exporter parse failure'}
 $publisher=$ast.Find({param($node)$node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Publish-ArtifactActorRows'},$true)
 if(!$publisher){throw 'Missing actual actor publisher'};. ([scriptblock]::Create($publisher.Extent.Text))
 $source="$out/publisher.stdout";$target="$out/publisher.jsonl";$utf8=[Text.UTF8Encoding]::new($false)
 $valid="{`"id`":1}`n{`"id`":2}`n__ARTIFACT_EXPORT_ROWS__2`n";[IO.File]::WriteAllText($source,$valid,$utf8)
 if((Publish-ArtifactActorRows $source $target) -ne 2 -or (Get-Content $target).Count -ne 2){throw 'Complete actor publication failed'}
 foreach($invalid in @("{`"id`":1}`n","{`"id`":1}`n__ARTIFACT_EXPORT_ROWS__2`n","{`"id`":1}`n__ARTIFACT_EXPORT_ROWS__1`nextra`n","{`n__ARTIFACT_EXPORT_ROWS__1`n","{`"id`":1}`n__ARTIFACT_EXPORT_ROWS__1")){
  [IO.File]::WriteAllText($source,$invalid,$utf8);$before=(Get-FileHash $source).Hash;$saved=(Get-FileHash $target).Hash;$rejected=$false
  try{Publish-ArtifactActorRows $source $target|Out-Null}catch{$rejected=$true}
  if(!$rejected -or (Get-FileHash $source).Hash -cne $before -or (Get-FileHash $target).Hash -cne $saved){throw 'Incomplete actor publication was accepted or overwrote retained evidence'}
 }
 $captureEvidence=if($objects){"Actual all36 captures retain all19 prior findings and expose four additional candidates;4tables/$objects objects match server counts/exact native prefixes."}else{'Captured-data comparison omitted; standalone synthetic scope and actual publisher checks only.'}
 "PASS actual explicit scenario map union/native-only/dedup/zero/unmapped cases. $captureEvidence Actual actor publisher rejects five incomplete counterfactuals without changing source/prior publication. No character/live data writes."
}
