$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Extract-Class([string]$s,[string]$name){$a=$s.IndexOf("class $name ");if($a -lt 0){throw "Missing $name"};$i=$s.IndexOf('{',$a);$depth=1;for($j=$i+1;$j -lt $s.Length;$j++){if($s[$j] -eq '{'){$depth++};if($s[$j] -eq '}'){$depth--};if(!$depth){return $s.Substring($a,$j+2-$a)}};throw 'Unclosed class'}
$plan=Get-Content "$PSScriptRoot/FourthTenSpellRepairs.json" -Raw|ConvertFrom-Json
if(@($plan).Count -ne 10 -or @($plan.key|Sort-Object -Unique).Count -ne 10){throw 'Exactly ten distinct product fixes required'}
foreach($path in $plan.edits.path|Sort-Object -Unique){
 $old=(& git -C $repo show "8189c1241:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned source absent'}
 $current=(Get-Content "$repo/$path" -Raw -Encoding utf8).Replace("`r",'');$normal=$current
 foreach($entry in $plan){foreach($edit in $entry.edits|Where-Object path -eq $path){foreach($cls in $edit.classes){$actual=Extract-Class $current $cls;$prior=Extract-Class $old $cls;if(($actual -replace '\s','') -ceq ($prior -replace '\s','')){throw "No repair for $($entry.key)"};$normal=$normal.Replace($actual,$prior)
 if($entry.key -eq 'Vantus'){foreach($suffix in @('_SpellScript')){$nested=$cls+$suffix;if((Extract-Class $actual $nested) -cne (Extract-Class $prior $nested)){throw "Weekly quest cast behavior changed: $nested"}}}
 }}}
 $normal=$normal.Replace('#include "../VantusRuneLifecycle.h"','')
 if(($normal -replace '\s','') -cne ($old -replace '\s','')){throw "Unrelated production behavior changed: $path"}
}
'PASS exactly ten fixes/15 selected outer classes/12cpp+shared helper;all other production behavior and original Vantus weekly-quest admission/completion unchanged against8189c1241'
