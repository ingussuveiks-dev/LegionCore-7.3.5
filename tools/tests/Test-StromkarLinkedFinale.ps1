param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-linked-finale"
New-Item -ItemType Directory -Force $out|Out-Null
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$sql=@'
SELECT 'total',COUNT(*) FROM spell_linked_spell WHERE spell_trigger=206456;
SELECT 'known',COUNT(*) FROM spell_linked_spell WHERE spell_trigger=206456 AND spell_effect=201904 AND type=0 AND caster=0 AND target=0
AND hastype=0 AND hastalent=0 AND hasparam=0 AND hastype2=0 AND hastalent2=0 AND hasparam2=0 AND chance=0 AND cooldown=0 AND duration=0
AND hitmask=0 AND removeMask=0 AND effectMask=0 AND targetCountType=0 AND targetCount=-1 AND actiontype=0 AND `group`=0 AND param=0 AND randList='';
SELECT 'action',spell_effect,duration,actiontype FROM spell_linked_spell WHERE spell_trigger=206456;
SELECT 'target-total',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=201904;
SELECT 'targets',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceGroup=1 AND SourceEntry=201904 AND SourceId=0
AND ConditionTypeOrReference=31 AND ConditionTarget=0 AND ConditionValue1=3 AND ConditionValue3=0 AND NegativeCondition=0 AND ErrorTextId=0 AND ScriptName=''
AND ((ElseGroup=0 AND ConditionValue2=102039) OR (ElseGroup=1 AND ConditionValue2=104276));
SELECT 'hit-list',COUNT(*) FROM smart_scripts WHERE entryorguid=104276 AND source_type=0 AND id=8 AND event_type=8 AND event_param1=201904
AND action_type=80 AND action_param1=104276 AND target_type=1;
SELECT 'dialogue',COUNT(*) FROM smart_scripts WHERE entryorguid=104276 AND source_type=9 AND id=1 AND action_type=45 AND action_param1=1 AND action_param2=1 AND target_type=19 AND target_param1=104307 AND target_param2=50;
'@
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Linked finale read failed'}}finally{$env:MYSQL_PWD=$previous}
[IO.File]::WriteAllLines("$out/diagnostics.tsv",[string[]]$rows)
foreach($expected in @("total`t1","known`t1","target-total`t2","targets`t2","hit-list`t1","dialogue`t1")){if($rows -notcontains $expected){throw "Linked graph changed: $expected"}}
[IO.File]::WriteAllLines("$out/action.tsv",[string[]]@($rows|Where-Object {$_ -like "action`t*"}|ForEach-Object {$_.Substring(7)}))
$source=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$start=$source.IndexOf('void Spell::LinkedSpell(');$positive=$source.IndexOf('int32 spell_trigger = i.effect;',$start)
$first=$source.IndexOf('                    case LINK_ACTION_DEFAULT:',$positive);$last=$source.IndexOf('                    case LINK_ACTION_LEARN:',$first)
if($start -lt 0 -or $positive -le $start -or $first -le $positive -or $last -le $first){throw 'Missing native positive linked action'}
[IO.File]::WriteAllText("$out/LinkedDefault.inc",$source.Substring($first,$last-$first))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/StromkarLinkedFinaleTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/action.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native linked request failed'}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_stromkar_linked_finale_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical linked request failed'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: preserved live206456 ->201904 type0 link, both native target alternatives, actual boss SpellHit/list and Thoradin SetData graph checked. This disproves a disconnected-link claim, not proof that the child hit, missing actor or farewell visuals work in the client.'
