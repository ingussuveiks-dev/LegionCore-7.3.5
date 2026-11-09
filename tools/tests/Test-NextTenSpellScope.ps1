$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Extract-Class([string]$s,[string]$name){$a=$s.IndexOf("class $name ");if($a -lt 0){throw "Missing $name"};$i=$s.IndexOf('{',$a);$depth=1;for($j=$i+1;$j -lt $s.Length;$j++){if($s[$j] -eq '{'){$depth++};if($s[$j] -eq '}'){$depth--};if(!$depth){return $s.Substring($a,$j+2-$a)}};throw 'Unclosed class'}
$plan=Get-Content "$PSScriptRoot/NextTenSpellRepairs.json" -Raw|ConvertFrom-Json
if(@($plan).Count -ne 10 -or @($plan.key|Sort-Object -Unique).Count -ne 10){throw 'Exactly ten distinct product cases required'}
foreach($file in $plan.file|Sort-Object -Unique){
 $path="src/server/scripts/Spells/$file";$old=(& git -C $repo show "50b42e107:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned ten-fix source missing'}
 $current=(Get-Content "$repo/$path" -Raw -Encoding utf8).Replace("`r",'');$normal=$current
 foreach($entry in $plan|Where-Object file -eq $file){$actual=Extract-Class $current $entry.cls;$prior=Extract-Class $old $entry.cls;if(($actual -replace '\s','') -ceq ($prior -replace '\s','')){throw "No repair for $($entry.key)"};$normal=$normal.Replace($actual,$prior)}
 if(($normal -replace '\s','') -cne ($old -replace '\s','')){throw "Unrelated production behavior changed in $file"}
}
'PASS exactly ten selected complete classes changed;five whole production files equal pinned 50b42e107 outside those classes,including all registrations/other spells'
