param([string]$MigrationFile="$PSScriptRoot/../../sql/updates/world/2026_11_17_06_hunter_hippogryph_terminal_link.sql")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hunter-hippogryph-terminal-link"
New-Item -ItemType Directory -Force -Path $out | Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/diagnostics" -RuntimeDirectories @($out) -Test {
 & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -EvidenceDirectory "$out/reader" -Test {
    $migration=Get-Content -LiteralPath $MigrationFile -Raw
    $settings=((Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}) -split '"')[1].Split(';')
    $settings[2]=$env:CODEX_SQL_READER_USER;$settings[3]=$env:CODEX_SQL_READER_PASSWORD
    $filters=@{creature_template='entry=102177';smart_scripts='entryorguid=102177 AND source_type=0';conditions='SourceTypeOrReferenceId=22 AND SourceEntry=102177'}
    $tables=@('creature_template','smart_scripts','conditions')
    $setup=($tables | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_ WHERE $($filters[$_]); CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
    # Reconstruct only the recognized pre-fix link in TEMP, so installed data
    # still exercises the actual2->0 correction before idempotence/refusals.
    $setup+="`nUPDATE smart_scripts SET link=2 WHERE id=1 AND link=0;"
    $snapshot="SELECT 'TEMPLATE'; SELECT * FROM creature_template ORDER BY entry; SELECT 'SCRIPT'; SELECT * FROM smart_scripts ORDER BY id; SELECT 'CONDITION'; SELECT * FROM conditions ORDER BY SourceGroup,ElseGroup,ConditionTypeOrReference;"
    $cases=[ordered]@{
        repaired=''
        custom_node='INSERT INTO smart_scripts SELECT entryorguid,source_type,2,0,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment FROM saved_smart_scripts WHERE id=1;'
        custom_link='UPDATE smart_scripts SET link=3 WHERE id=1;'
        custom_action='UPDATE smart_scripts SET action_type=33 WHERE id=1;'
        custom_map='UPDATE smart_scripts SET action_param1=1221 WHERE id=1;'
        custom_target='UPDATE smart_scripts SET target_type=1 WHERE id=1;'
        custom_destination='UPDATE smart_scripts SET target_x=target_x+1 WHERE id=1;'
        custom_script="UPDATE creature_template SET ScriptName='custom_hunter_bird';"
        custom_quest='UPDATE conditions SET ConditionValue1=40393 WHERE ConditionTypeOrReference=9 AND SourceGroup=1;'
        missing_quest='DELETE FROM conditions WHERE ConditionTypeOrReference=9 AND SourceGroup=1;'
    }
    foreach($case in $cases.GetEnumerator()){
        $sql="$setup`n$($case.Value)`nSELECT 'BEFORE'; $snapshot`n$migration`nSELECT 'ONCE'; $snapshot`n"
        if($case.Key -eq 'repaired'){$sql+="$migration`nSELECT 'TWICE'; $snapshot`n"}
        [IO.File]::WriteAllText("$out/$($case.Key).sql",$sql)
        $prior=$env:MYSQL_PWD
        try{
            $env:MYSQL_PWD=$settings[3]
            $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($settings[0]) --port=$($settings[1]) --user=$($settings[2]) --database=$($settings[4]) --batch --raw --skip-column-names --force 2> "$out/$($case.Key).stderr")
            if($LASTEXITCODE){throw "SQL client failed for $($case.Key)"}
        }finally{$env:MYSQL_PWD=$prior}
        [IO.File]::WriteAllLines("$out/$($case.Key).tsv",[string[]]$rows)
        $before=[array]::IndexOf($rows,'BEFORE'); $once=[array]::IndexOf($rows,'ONCE')
        if($before -ne 0 -or $once -le 0){throw "Missing snapshots: $($case.Key)"}
        $expected=@($rows[($before+1)..($once-1)])
        if($case.Key -eq 'repaired'){
            $expected=@($expected | ForEach-Object {if($_ -match '^102177\t0\t1\t2\t'){$_ -replace '^102177\t0\t1\t2\t',"102177`t0`t1`t0`t"}else{$_}})
            $twice=[array]::IndexOf($rows,'TWICE')
            if($twice -le $once){throw 'Missing idempotent snapshot'}
            $actual=@($rows[($once+1)..($twice-1)])
            $repeat=@($rows[($twice+1)..($rows.Count-1)])
            if(($actual -join "`n") -cne ($expected -join "`n") -or ($repeat -join "`n") -cne ($expected -join "`n")){throw 'Unexpected changes or failed idempotence'}
            if((Get-Item "$out/$($case.Key).stderr").Length){throw 'Unexpected ordinary SQL diagnostics'}
        }else{
            $actual=@($rows[($once+1)..($rows.Count-1)])
            if(($actual -join "`n") -cne ($expected -join "`n")){throw "Refusal mutated custom data: $($case.Key)"}
            $errors=@(Get-Content "$out/$($case.Key).stderr" | Where-Object {$_ -match '^ERROR '})
            if($errors.Count -ne 1 -or $errors[0] -notmatch '45000.*Hunter hippogryph terminal graph differs'){throw "Wrong refusal diagnostic: $($case.Key)"}
        }
        "PASS $($case.Key): full actor/script/condition snapshots retained; TEMP shadows only"
    }
}
}
