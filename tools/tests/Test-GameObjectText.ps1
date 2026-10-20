param([string]$BaselineRevision,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/gameobject-text"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$s=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
if($BaselineRevision){$s=(& git -C $repo show "${BaselineRevision}:src/server/game/Entities/GameObject/GameObject.cpp") -join "`n";if($LASTEXITCODE){throw 'Cannot read baseline'}}
$a=$s.IndexOf('        case GAMEOBJECT_TYPE_TEXT:', $s.IndexOf('void GameObject::Use('))
$b=$s.IndexOf('        case GAMEOBJECT_TYPE_CHALLENGE_MODE_REWARD:', $a)
if($a -lt 0 -or $b -lt $a){throw 'Missing native text branch'}
[IO.File]::WriteAllText("$out/TextUse.inc",$s.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/GameObjectTextTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native text-object regression failed'}
$migration=Get-Content "$repo/sql/updates/world/2026_10_20_01_ariden_fur_blanket_page.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_page_text LIKE page_text;
INSERT INTO saved_page_text SELECT * FROM page_text WHERE ID=5233;
CREATE TEMPORARY TABLE page_text LIKE saved_page_text;
INSERT INTO page_text SELECT * FROM saved_page_text;
$migration
$migration
SELECT 'page',COUNT(*) FROM page_text WHERE ID=5234 AND Text='The bedding in the tent has been undisturbed for quite some time.' AND NextPageID=0 AND PlayerConditionID=0 AND Flags=0;
SELECT 'mapping',COUNT(*) FROM gameobject_template WHERE entry=245788 AND type=9 AND Data0=5234 AND Data4=0;
SELECT 'unrelated',COUNT(*) FROM page_text p JOIN saved_page_text s ON s.ID=p.ID WHERE p.ID=5233 AND BINARY p.Text=BINARY s.Text AND p.NextPageID=s.NextPageID AND p.PlayerConditionID=s.PlayerConditionID AND p.Flags=s.Flags;
UPDATE page_text SET Text='existing custom text',PlayerConditionID=99 WHERE ID=5234;
$migration
SELECT 'existing',COUNT(*) FROM page_text WHERE ID=5234 AND Text='existing custom text' AND PlayerConditionID=99;
SELECT 'objective',COUNT(*) FROM quest_objectives WHERE QuestID=40604 AND Type=2 AND ObjectID=245789;
SELECT 'blanket-credit',COUNT(*) FROM quest_objectives WHERE QuestID IN (40604,40835,40932) AND Type=2 AND ObjectID=245788;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$c[3]
 $rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Text-object SQL fixture failed'}
}finally{$env:MYSQL_PWD=$old}
foreach($expected in @("page`t1","mapping`t1","unrelated`t1","existing`t1","objective`t1","blanket-credit`t0")){if($rows -notcontains $expected){throw "Missing SQL check: $expected"}}
'PASS: missing page restored, repeat migration, existing/custom page and adjacent page preserved; journal objective retained'
if($VerifyReleaseBinary){
 $map=Get-Content "$repo/build-extractors/bin/Release/worldserver.map" -Raw
 function Symbol([string]$name){
  $m=[regex]::Match($map,('(?m)^\s+\S+\s+'+[regex]::Escape($name)+'\S*\s+([0-9A-Fa-f]{16})\s+f'))
  if(!$m.Success){throw "Missing canonical symbol $name"}
  [Convert]::ToInt64($m.Groups[1].Value,16)
 }
 $caller=Symbol '?Use@GameObject@@'
 $writer=Symbol '?Write@PageText@GameObject@WorldPackets@@'
 $binary=[IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe")
 $pe=[BitConverter]::ToInt32($binary,0x3c);$optional=$pe+24
 $stamp=[regex]::Match($map,'Timestamp is ([0-9A-Fa-f]+)')
 if(!$stamp.Success -or [Convert]::ToUInt32($stamp.Groups[1].Value,16) -ne [BitConverter]::ToUInt32($binary,$pe+8)){throw 'Canonical map does not match executable link timestamp'}
 $imageBase=[BitConverter]::ToInt64($binary,$optional+24)
 $sections=$optional+[BitConverter]::ToUInt16($binary,$pe+20)
 function Offset([long]$address){
  $relative=$address-$imageBase
  for($j=0;$j -lt [BitConverter]::ToUInt16($binary,$pe+6);$j++){
   $s=$sections+40*$j;$start=[BitConverter]::ToUInt32($binary,$s+12)
   $size=[Math]::Max([BitConverter]::ToUInt32($binary,$s+8),[BitConverter]::ToUInt32($binary,$s+16))
   if($relative -ge $start -and $relative -lt $start+$size){return [long]([BitConverter]::ToUInt32($binary,$s+20)+$relative-$start)}
  }
  throw 'Address outside canonical PE'
 }
 function Resolve([long]$address){
  for($j=0;$j -lt 10;$j++){$at=Offset $address;if($binary[$at] -ne 0xe9){return $address};$address+=5+[BitConverter]::ToInt32($binary,$at+1)}
  throw 'Incremental-link thunk cycle'
 }
 $next=[long]::MaxValue
 foreach($m in [regex]::Matches($map,'(?m)^\s+\S+\s+\S+\s+([0-9A-Fa-f]{16})\s+f')){
  $v=[Convert]::ToInt64($m.Groups[1].Value,16);if($v -gt $caller -and $v -lt $next){$next=$v}
 }
 if($next-$caller -gt 65536){throw 'Unbounded native Use function'}
 $at=Offset $caller;$calls=0
 for($i=0;$i -lt $next-$caller-4;$i++){
  if($binary[$at+$i] -eq 0xe8){
   # Immediate data can also contain E8. Ignore candidates outside PE sections.
   try{$dest=Resolve ($caller+$i+5+[BitConverter]::ToInt32($binary,$at+$i+1))}catch{if($_.Exception.Message -eq 'Address outside canonical PE'){continue};throw}
   if($dest -eq $writer){++$calls}
  }
 }
 # The pre-fix function has only the existing GOOBER page dispatch.
 if($calls -ne 2){throw "Expected GOOBER and TEXT page writers in deployed Use; found $calls"}
 'PASS: canonical Release GameObject::Use contains both page dispatch call sites (incremental-link thunks resolved)'
}
