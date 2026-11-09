param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Extract-Class([string]$s,[string]$name){$a=$s.IndexOf("class $name ");if($a -lt 0){throw "Missing $name"};$b=$s.IndexOf('{',$a)+1;$d=1;while($d){$d+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b+1-$a)}
$plan=Get-Content "$PSScriptRoot/FourthTenSpellRepairs.json" -Raw|ConvertFrom-Json
$raids=Get-Content "$PSScriptRoot/FourthTenVantus.json" -Raw|ConvertFrom-Json
$parts=@();$tables=@()
foreach($raid in $raids){$header=Get-Content "$repo/$($raid.header)" -Raw -Encoding utf8;if(!$header.Replace("`r`n","`n").Contains($raid.enum.Replace("`r`n","`n"))){throw "Vantus enum drift: $($raid.header)"}}
foreach($entry in $plan){foreach($test in $entry.tests){
 $s=Get-Content "$repo/$($test.path)" -Raw -Encoding utf8;$part=Extract-Class $s $test.cls
 $part=[regex]::Replace($part,'(Prepare(?:Aura|Spell)Script\([^;]+\);)','$1'+"`npublic:")
 $prefix='';if($test.key.StartsWith('Vantus')){$raid=$raids|Where-Object {$test.key -eq ('Vantus'+$_.key)};$prefix=$raid.enum+';';if($raid.key -eq 'TOV'){$prefix='namespace Data {'+$prefix+'}'}
 $rows=@($raid.table|ForEach-Object {'{'+$_.rune+','+$_.helper+','+$_.boss+'}'}) -join ','
 $tables+=('namespace F'+$test.key+' {static LegionVantus::Encounter const cases[]={'+$rows+'}; static uint32 const mapId='+$raid.map+';}')
 }
 if($test.key -eq 'Tirathon'){$prefix='constexpr uint32 SPELL_LASER_BEAM_BETWEEN_DMG=202862;'}
 [IO.File]::WriteAllText("$OutputDirectory/$($test.key).inc",$prefix+"`n"+$part)
 $parts+=('namespace F'+$test.key+' {'+$prefix+"`n"+$part+'}')
}}
[IO.File]::WriteAllText("$OutputDirectory/FourthTenScripts.inc",($parts -join "`n"))
$valor=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/commanders_valor.cpp" -Raw -Encoding utf8
$start=$valor.IndexOf('namespace CommandersValor');$end=$valor.IndexOf('class spell_commanders_valor_location')
[IO.File]::WriteAllText("$OutputDirectory/FourthValorPredicate.inc",$valor.Substring($start,$end-$start))
[IO.File]::WriteAllText("$OutputDirectory/FourthVantusTables.inc",($tables -join "`n"))
$names=($plan.edits.classes|Sort-Object -Unique|ForEach-Object {"'$_'"}) -join ','
[IO.File]::WriteAllText("$OutputDirectory/FourthTenBindings.inc",'R"SQL(SELECT spell_id,ScriptName FROM spell_script_names WHERE ScriptName IN('+$names+') ORDER BY ScriptName,spell_id)SQL"')
