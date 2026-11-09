param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$d=1;while($d){$d+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
$shaman=Get-Content "$repo/src/server/scripts/Spells/spell_shaman.cpp" -Raw -Encoding utf8
$dh=Get-Content "$repo/src/server/scripts/Spells/spell_dh.cpp" -Raw -Encoding utf8
$paladin=Get-Content "$repo/src/server/scripts/Spells/spell_paladin.cpp" -Raw -Encoding utf8
$parts=@()
$parts+='namespace FSpirit {'+(Block $shaman 'class spell_sha_spirit_link :')+';}'
$a=$shaman.IndexOf('namespace');$b=$shaman.IndexOf('// Spirit Link -');$helper=$shaman.Substring($a,$b-$a)
$parts+='namespace FRain {'+$helper+(Block $shaman 'class spell_sha_healing_rain :')+';}'
$parts+='namespace FBarrier {enum{SoulFragment1=204255,SoulFragment2=203795,SoulFragment3=204062};'+(Block $dh 'class spell_dh_soul_barrier :')+';}'
$parts+='namespace FDevotion {'+(Block $paladin 'enum PaladinSpells')+';'+(Block $paladin 'struct spell_pal_at_aura_of_sacrifice :')+';'+(Block $paladin 'struct spell_pal_at_devotion_aura :')+';}'
$s=$parts -join "`n";$s=[regex]::Replace($s,'(Prepare(?:Aura|Spell)Script\([^;]+\);)','$1'+"`npublic:")
[IO.File]::WriteAllText("$OutputDirectory/FifthTenScripts.inc",$s)
