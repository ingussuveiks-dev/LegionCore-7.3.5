param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/vrekt-perks"
New-Item -ItemType Directory -Force $out | Out-Null
function Query([string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Vrekt perk SQL check failed'}}finally{$env:MYSQL_PWD=$previous};$rows
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_18_12_vrekt_combat_perks.sql" -Raw
$rows=@(Query @"
CREATE TEMPORARY TABLE saved_spells LIKE creature_template_spell;
INSERT INTO saved_spells SELECT * FROM creature_template_spell WHERE entry IN(115846,116180,116600,116601,115025);
CREATE TEMPORARY TABLE creature_template_spell LIKE saved_spells;
INSERT INTO creature_template_spell SELECT * FROM saved_spells;
DELETE FROM creature_template_spell WHERE entry IN(115846,116180,116600,116601) AND spell IN(213738,215377);
SELECT 'retained',s.* FROM creature_template_spell s ORDER BY entry,spell;
INSERT INTO creature_template_spell VALUES(115846,213738,0,'','fixture'),(116180,213738,0,'','fixture'),(116180,215377,0,'','fixture'),(116600,213738,0,'','fixture'),(116600,215377,0,'','fixture'),(116601,213738,0,'','fixture'),(116601,215377,0,'','fixture');
$migration
SELECT 'first',s.* FROM creature_template_spell s ORDER BY entry,spell;
$migration
SELECT 'repeat',s.* FROM creature_template_spell s ORDER BY entry,spell;
SELECT 'combat',entry,spell FROM creature_template_spell WHERE entry IN(115846,116180,116600,116601) ORDER BY entry,spell;
"@)
function Tagged($tag){@($rows | Where-Object {$_ -like "$tag`t*"} | ForEach-Object {($_ -split "`t",2)[1]})}
if(((Tagged 'retained') -join '|') -ne ((Tagged 'first') -join '|') -or ((Tagged 'first') -join '|') -ne ((Tagged 'repeat') -join '|')){throw 'Perk migration changed retained fields/neighbour or repeat result'}
$expected=@("115846`t186476","115846`t186480","115846`t186553","116180`t186327","116180`t195585","116600`t186553","116600`t222484","116601`t186476","116601`t186480")
if(((Tagged 'combat') -join '|') -ne ($expected -join '|')){throw 'Retained native combat pool changed'}
if($Live){$liveRows=@(Query 'SELECT entry,spell FROM creature_template_spell WHERE entry IN(115846,116180,116600,116601) ORDER BY entry,spell;');if(($liveRows -join '|') -ne ($expected -join '|')){throw 'Deployed pool differs from tested repair'}}
function Slice($path,$start,$end){$source=Get-Content "$repo/$path" -Raw;$a=$source.IndexOf($start);$b=$source.IndexOf($end,$a);if($a -lt 0 -or $b -le $a){throw 'Missing native perk boundary'};$source.Substring($a,$b-$a)}
[IO.File]::WriteAllText("$out/NativePerkCondition.inc",(Slice 'src/server/game/AI/CoreAI/UnitAI.cpp' '        if (spellInfo->HasAttribute(SPELL_ATTR0_CANT_USED_IN_COMBAT))' '        if (AIInfo->cooldown < spellInfo->Cooldowns.RecoveryTime)'))
[IO.File]::WriteAllText("$out/NativePerkAura.inc",(Slice 'src/server/game/Spells/SpellInfo.cpp' '                    case SPELL_AURA_MOD_DAMAGE_PERCENT_DONE:' '                    default:'))
$enums='';foreach($item in @(@('src/server/game/Miscellaneous/SharedDefines.h','SpellAttr0'),@('src/server/game/Spells/Auras/SpellAuraDefines.h','AuraType'),@('src/server/game/AI/CreatureAIImpl.h','AICondition'))){
 $m=[regex]::Match((Get-Content "$repo/$($item[0])" -Raw),('(?s)enum '+$item[1]+'\b.*?\n\};'));if(!$m.Success){throw 'Missing native perk enum'};$enums+=$m.Value+"`n"
}
[IO.File]::WriteAllText("$out/NativePerkEnums.inc",$enums)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /I`"$out`" `"$PSScriptRoot/VrektPerksTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd";if($LASTEXITCODE){throw 'Vrekt native perk admission regression failed'}
'PASS: scoped repeated removal of7 unrelated artifact rows, all9 retained combat rows/full fields and neighbour preserved; native classification/aura rejection execute. Full spell dispatch remains a separate check.'
