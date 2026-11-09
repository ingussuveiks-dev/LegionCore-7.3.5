param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Extract-Class([string]$s,[string]$name){$a=$s.IndexOf("class $name ");if($a -lt 0){throw "Missing $name"};$i=$s.IndexOf('{',$a);$depth=1;for($j=$i+1;$j -lt $s.Length;$j++){if($s[$j] -eq '{'){$depth++};if($s[$j] -eq '}'){$depth--};if(!$depth){return $s.Substring($a,$j+2-$a)}};throw 'Unclosed class'}
$plan=Get-Content "$PSScriptRoot/TenSpellLifecycles.json" -Raw|ConvertFrom-Json
$parts=@();foreach($entry in $plan){$s=Get-Content "$repo/src/server/scripts/Spells/$($entry.file)" -Raw -Encoding utf8;$part=Extract-Class $s $entry.cls;$part=[regex]::Replace($part,'(Prepare(?:Aura|Spell)Script\([^;]+\);)','$1' + "`npublic:");$parts+=$part}
[IO.File]::WriteAllText("$OutputDirectory/TenSpellScripts.inc",($parts -join "`n"))
$names=($plan|ForEach-Object {"'$($_.cls)'"}) -join ','
[IO.File]::WriteAllText("$OutputDirectory/TenSpellBindings.inc",'R"TEN_SQL(SELECT spell_id,ScriptName FROM spell_script_names WHERE ScriptName IN('+$names+') ORDER BY ScriptName,spell_id)TEN_SQL"')
