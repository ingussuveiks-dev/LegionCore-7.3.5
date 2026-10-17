$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-runes-bind"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$sql=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags,q.QuestPackageID FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(38823,38815) ORDER BY q.ID;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID=38823 ORDER BY StorageIndex;
SELECT 'starter',id,quest FROM gameobject_queststarter WHERE quest=38823;
SELECT 'next-starter',id,quest FROM creature_queststarter WHERE quest=38815;
SELECT 'ender',id,quest FROM creature_questender WHERE quest=38823;
SELECT 'go',entry,type,displayId,flags,size,Data0,Data1,Data2,Data3,Data7,Data10,Data14,Data19,Data20,Data22,Data26,SpellStateAnimKitID,ScriptName FROM gameobject_template WHERE entry IN(241840,241849,251412,251413) ORDER BY entry;
SELECT 'spawn',guid,id,map,position_x,position_y,position_z,PhaseId,state FROM gameobject WHERE id IN(241840,241849,251412,251413) ORDER BY guid;
SELECT 'gossip',g.MenuID,g.TextID,n.BroadcastTextID0 FROM gossip_menu g JOIN npc_text n ON n.ID=g.TextID WHERE g.MenuID=19257;
SELECT 'ashildir',guid,id,map,position_x,position_y,position_z,PhaseId FROM creature WHERE id=97270 ORDER BY guid;
SELECT 'conditions',COUNT(*) FROM conditions WHERE SourceEntry IN(38823,241849,251412,251413) AND SourceTypeOrReferenceId IN(13,17,19,20,22);
SELECT 'go-scripts',COUNT(*) FROM gameobject_scripts WHERE id IN(115143,115167,115176,115191);
SELECT 'start-scripts',COUNT(*) FROM quest_start_scripts WHERE id=38823 OR(command=7 AND datalong=38823);
'@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Runes Bind World query failed'}}finally{$env:MYSQL_PWD=$previous}
$rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$rows | Set-Content "$out/world.tsv"
foreach($row in @("quest`t38823`t33554440`t0`t18446744073709551615`t0`t38815`t0`t38815`t0`t0`t0","quest`t38815`t33554440`t0`t18446744073709551615`t0`t38818`t38816`t38818`t0`t0`t0","starter`t241840`t38823","next-starter`t97270`t38815","ender`t97270`t38823","gossip`t19257`t28290`t104957","objective`t38823`t284809`t2`t241849`t1`t0`t0","objective`t38823`t284810`t2`t251412`t1`t1`t0","objective`t38823`t284811`t2`t251413`t1`t2`t0","objective`t38823`t278478`t0`t109061`t3`t3`t28")){if($rows -notcontains $row){throw "Runes Bind contract changed: $row"}}
foreach($pair in @(@(241849,8859),@(251412,11129),@(251413,11129))){if($rows -notcontains "go`t$($pair[0])`t10`t28714`t262148`t0.75`t1691`t38823`t0`t3000`t0`t0`t20138`t0`t0`t0`t0`t$($pair[1])`t"){throw 'Ritual stone native template changed'}}
foreach($row in @("spawn`t115143`t241849`t1220`t3555.14`t1001.38`t103.702`t`t1","spawn`t115167`t241840`t1220`t3407.49`t982.872`t101.165`t`t1","spawn`t115176`t251413`t1220`t3427.26`t887.076`t88.9982`t`t1","spawn`t115191`t251412`t1220`t3473.61`t797.974`t78.8167`t`t1")){if($rows -notcontains $row){throw "Ritual access anchor changed: $row"}}
if($rows -notcontains "go`t241840`t2`t7356`t0`t0.5`t0`t19257`t0`t0`t0`t0`t0`t0`t0`t0`t0`t0`t"){throw 'Tablet questgiver/gossip changed'}
foreach($kind in @('conditions','go-scripts','start-scripts')){if($rows -notcontains "$kind`t0"){throw "Unexpected Runes Bind $kind"}}
$sql=@'
SELECT 'display',COUNT(*) FROM gameobject_display_info WHERE ID IN(7356,28714); SELECT 'lock',COUNT(*) FROM `lock` WHERE ID=1691; SELECT 'anim',COUNT(*) FROM anim_kit WHERE ID IN(8859,11129); SELECT 'text',ID,LENGTH(Text),LENGTH(Text1) FROM broadcast_text WHERE ID IN(104957,20138) ORDER BY ID;
'@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Runes Bind Hotfix query failed'}}finally{$env:MYSQL_PWD=$previous}
$rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$rows | Set-Content "$out/hotfix.tsv"
foreach($kind in @('display','lock','anim')){if($rows -notcontains "$kind`t0"){throw "Unexpected Runes Bind native $kind override"}}
foreach($id in @(104957,20138)){$text=@($rows | Where-Object {$_ -like "text`t$id`t*"});if($text.Count -ne 1){throw "Missing actual ritual broadcast $id"};$fields=$text[0].Split("`t");if(([int]$fields[2]+[int]$fields[3]) -le 0){throw "Empty ritual broadcast $id"}}
'PASS: original three distinct ritual GO objectives, optional counter28, tablet starter/text and Ashildir ender/38815 relation; native lock/model/animation template settings preserved.'
