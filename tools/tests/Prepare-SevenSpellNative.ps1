param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Extract-Class([string]$s,[string]$name){$a=$s.IndexOf("class $name ");if($a -lt 0){throw "Missing $name"};$i=$s.IndexOf('{',$a);$depth=1;for($j=$i+1;$j -lt $s.Length;$j++){if($s[$j] -eq '{'){$depth++};if($s[$j] -eq '}'){$depth--};if(!$depth){return $s.Substring($a,$j+2-$a)}};throw 'Unclosed class'}
$parts=@()
foreach($file in 'spell_generic.cpp','spell_item.cpp'){
 $s=Get-Content "$repo/src/server/scripts/Spells/$file" -Raw -Encoding utf8
 $names=if($file -eq 'spell_generic.cpp'){@('spell_gen_touch_of_elune','spell_mount_summon_chauffeur','spell_flight_masters_whistle_argus')}else{@('spell_item_leystone_hoofplates','spell_item_demonsteel_stirrups','spell_item_golem','spell_masters_hunter_seeking_crystal_teleport')}
 foreach($name in $names){$body=Extract-Class $s $name;$body=$body.Replace("PrepareAuraScript($name);","PrepareAuraScript($name);`npublic:").Replace("PrepareSpellScript($name);","PrepareSpellScript($name);`npublic:");$parts+=$body}
}
[IO.File]::WriteAllText("$OutputDirectory/SevenSpellScripts.inc",($parts -join "`n"))
