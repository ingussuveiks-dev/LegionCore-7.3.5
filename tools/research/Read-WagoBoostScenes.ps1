param(
    [string]$CachePath = "$env:TEMP/legion-wago-735-scenes",
    [string]$OutputPath = "$PSScriptRoot/../../docs/boost-tutorial-wago-evidence.json"
)
$ErrorActionPreference='Stop'
$build='7.3.5.26972'
New-Item -ItemType Directory -Path $CachePath -Force | Out-Null
$tables=@{}
$sources=@()
foreach($table in @('SceneScriptText','SceneScriptGlobalText','SceneScript','SceneScriptPackage','SceneScriptPackageMember','Scenario','ScenarioStep','TaxiPathNode','Vehicle','VehicleSeat','SpellEffect')) {
    $url="https://wago.tools/db2/$table/csv?build=$build"
    $path=Join-Path $CachePath "$table.csv"
    if(-not (Test-Path -LiteralPath $path) -or (Get-Item -LiteralPath $path).Length -eq 0) { Invoke-WebRequest $url -UseBasicParsing -TimeoutSec 90 -OutFile $path }
    $data=@(Import-Csv -LiteralPath $path)
    if(-not $data.Count -or -not ($data[0].PSObject.Properties.Name -contains 'ID')) {throw "Invalid CSV: $table"}
    $tables[$table]=$data
    $sources+= [ordered]@{Table=$table;URL=$url;Rows=$data.Count;SHA256=(Get-FileHash $path -Algorithm SHA256).Hash}
    Write-Host "$table : $($data.Count) rows"
}
$textIndex=@{}; foreach($r in $tables.SceneScriptText){$textIndex[$r.ID]=$r}
$scriptIndex=@{}; foreach($r in $tables.SceneScript){$scriptIndex[$r.ID]=$r}
$packageIndex=@{}; foreach($r in $tables.SceneScriptPackage){$packageIndex[$r.ID]=$r}
$members=@{}; foreach($r in $tables.SceneScriptPackageMember){if(-not $members.ContainsKey($r.SceneScriptPackageID)){$members[$r.SceneScriptPackageID]=@()};$members[$r.SceneScriptPackageID]+=$r}
$roots=@($tables.SceneScriptPackage | Where-Object { $_.Name -match 'Broken Shore Scenario|Leave For Broken Shore|boost|training|class.?trial|tutorial|Pre-Launch.*Client Scene' })
$graphs=@()
foreach($root in $roots){
    $queue=[Collections.Generic.Queue[string]]::new();$queue.Enqueue($root.ID)
    $seen=@{};$scriptSeen=@{};$packageRows=@();$memberRows=@();$scriptRows=@();$textRows=@();$missing=@()
    while($queue.Count){
        $id=$queue.Dequeue();if($seen.ContainsKey($id)){continue};$seen[$id]=$true
        if($packageIndex.ContainsKey($id)){$packageRows+=$packageIndex[$id]}else{$missing+="Package:$id"}
        foreach($m in $members[$id]){
            $memberRows+=$m
            if([int]$m.ChildSceneScriptPackageID -ne 0){$queue.Enqueue($m.ChildSceneScriptPackageID)}
            $sid=$m.SceneScriptID
            while($sid -and $sid -ne '0' -and -not $scriptSeen.ContainsKey($sid)){
                $scriptSeen[$sid]=$true
                if(-not $scriptIndex.ContainsKey($sid)){$missing+="Script:$sid";break}
                $sr=$scriptIndex[$sid];$scriptRows+=$sr
                if($textIndex.ContainsKey($sid)){
                    $tx=$textIndex[$sid]
                    # Keep provenance and relevant lines, not the complete copyrighted scripts.
                    $lines=@($tx.Script -split "`n");$selected=@()
                    for($i=0;$i -lt $lines.Count;$i++){if($lines[$i] -match 'Server|Trigger|Teleport|Queue|LFG|Scenario|MapID|ContinentID|105317|100448|219615|219912|227058'){$selected+=[ordered]@{Line=$i+1;Text=$lines[$i].Trim()}}}
                    $textRows+=[ordered]@{ID=$tx.ID;Name=$tx.Name;Length=$tx.Script.Length;RelevantLines=$selected}
                }else{$missing+="Text:$sid"}
                $sid=$sr.NextSceneScriptID
            }
        }
    }
    $graphs+=[ordered]@{Root=$root;Packages=$packageRows;Members=$memberRows;Scripts=$scriptRows;Texts=$textRows;Missing=$missing}
}
$exact='(?<![\d.])(105317|100448|112565|107543|219615|219912|227058|230503|231511)(?![\d.])|Zakgra|Hughes|boost|class.?trial|combat.?training'
$hits=@($tables.SceneScriptText|Where-Object { $_.Name -match $exact -or $_.Script -match $exact }|Select-Object ID,Name)
$globalHits=@($tables.SceneScriptGlobalText|Where-Object { ($_|ConvertTo-Json -Compress) -match $exact }|Select-Object ID,Name)
$local=Get-Content "$PSScriptRoot/../../docs/boost-tutorial-db2-evidence.json" -Raw|ConvertFrom-Json
$stepDifferences=@()
foreach($step in @($local.Steps|Where-Object ScenarioID -eq 1084)){
    $remote=@($tables.ScenarioStep|Where-Object ID -eq ([string]$step.ID))
    if($remote.Count -ne 1){$stepDifferences+="Missing/duplicate step $($step.ID)";continue}
    foreach($field in @('ID','ScenarioID','Supersedes','RewardQuestID','OrderIndex','Flags','CriteriaTreeID','RelatedStep')){
        if([string]$step.$field -ne [string]$remote[0].$field){$stepDifferences+="$($step.ID):$field"}
    }
    if($step.Title -ne $remote[0].Title_lang -or $step.Description -ne $remote[0].Description_lang){$stepDifferences+="$($step.ID):text"}
}
$vehicle=@($tables.Vehicle|Where-Object ID -eq '4933')
$seatIDs=@($vehicle|ForEach-Object {$_.PSObject.Properties|Where-Object Name -like 'SeatID_*'|ForEach-Object Value}|Where-Object {$_ -ne '0'})
$result=[ordered]@{
    Build=$build;Sources=$sources;Warning='Client scene data and reference paths, not a complete server tutorial. Cached downloads are not executed.'
    TutorialTextHits=$hits;GlobalTextHits=$globalHits;SceneGraphs=$graphs
    LocalWarlockStepComparison=[ordered]@{LocalCount=@($local.Steps|Where-Object ScenarioID -eq 1084).Count;RemoteCount=@($tables.ScenarioStep|Where-Object ScenarioID -eq '1084').Count;Differences=$stepDifferences}
    WarlockScenario=@($tables.Scenario|Where-Object ID -eq '1084')
    WarlockSteps=@($tables.ScenarioStep|Where-Object ScenarioID -eq '1084'|Sort-Object {[int]$_.OrderIndex})
    TransportNodes=@($tables.TaxiPathNode|Where-Object PathID -in '5528','5529','5950','5951')
    ExitVehicle=$vehicle
    ExitVehicleSeats=@($tables.VehicleSeat|Where-Object { $seatIDs -contains $_.ID })
    SelectedEffects=@($tables.SpellEffect|Where-Object SpellID -in '219615','223763','219912','227058','230503','231511','216356','225147')
}
$result|ConvertTo-Json -Depth 12|Set-Content -LiteralPath $OutputPath -Encoding UTF8
Write-Host "Saved evidence: $OutputPath"
