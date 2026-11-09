param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Extract-Class([string]$s,[string]$name){$a=$s.IndexOf("class $name ");if($a -lt 0){throw "Missing $name"};$i=$s.IndexOf('{',$a);$depth=1;for($j=$i+1;$j -lt $s.Length;$j++){if($s[$j] -eq '{'){$depth++};if($s[$j] -eq '}'){$depth--};if(!$depth){return $s.Substring($a,$j+2-$a)}};throw 'Unclosed class'}
$plan=Get-Content "$PSScriptRoot/ThirdTenSpellRepairs.json" -Raw|ConvertFrom-Json
$parts=@();foreach($entry in $plan){$s=Get-Content "$repo/src/server/scripts/Spells/$($entry.file)" -Raw -Encoding utf8;$part=(@($entry.classes|ForEach-Object {Extract-Class $s $_}) -join "`n");$part=[regex]::Replace($part,'(Prepare(?:Aura|Spell)Script\([^;]+\);)','$1' + "`npublic:");$parts+=$part}
[IO.File]::WriteAllText("$OutputDirectory/ThirdTenSpellScripts.inc",($parts -join "`n"))
$names=($plan|ForEach-Object {$_.classes|ForEach-Object {"'$_'"}}) -join ','
[IO.File]::WriteAllText("$OutputDirectory/ThirdTenSpellBindings.inc",'R"TEN_SQL(SELECT spell_id,ScriptName FROM spell_script_names WHERE ScriptName IN('+$names+') ORDER BY ScriptName,spell_id)TEN_SQL"')

$item=Get-Content "$repo/src/server/scripts/Spells/spell_item.cpp" -Raw -Encoding utf8
$generic=Get-Content "$repo/src/server/scripts/Spells/spell_generic.cpp" -Raw -Encoding utf8
function Slice([string]$text,[string]$first,[string]$last){$a=$text.IndexOf($first);$b=$text.IndexOf($last,$a);if($a -lt 0 -or $b -lt $a){throw 'Missing actual constants'};$text.Substring($a,$b-$a)}
$const=(Slice $item 'enum BrewfestMountTransformation' 'class spell_item_brewfest_mount_transformation')+(Slice $item 'uint32 RandomMorphs[16]' '// 74589')+(Slice $item 'uint32 MaleModels[]' 'class spell_item_demon_hunters_aspect')+(Slice $generic 'uint32 morph_list_alliance_darkmoon[49]' '// 70764')
[IO.File]::WriteAllText("$OutputDirectory/ThirdTenConstants.inc",$const)
