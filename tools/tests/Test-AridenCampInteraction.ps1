param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ariden-camp-interaction"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Save([string]$text,[string]$name){[IO.File]::WriteAllText("$out/$name.inc",$text)}
function Section([string]$s,[string]$start,[string]$end){$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a);if($a -lt 0 -or $b -lt $a){throw "Missing native section $start"};$s.Substring($a,$b-$a)}
$s=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
Save (Section $s 'namespace' 'GameObject::GameObject()') 'CampHelpers'
$use=Section $s 'void GameObject::Use(Unit* user)' '    // by default spell caster is user'
Save $use.Substring($use.IndexOf('{')+1) 'UseAdmission'
$update=Section $s 'void GameObject::BuildValuesUpdate(' 'void GameObject::GetRespawnPosition('
Save (Section $update '    bool campObject' '    bool targetIsGM') 'UpdateSetup'
$predicate=[regex]::Match($update,'(?m)^\s*if \((_fieldNotifyFlags[^\r\n]+)\)\s*\r?$').Groups[1].Value
if(!$predicate){throw 'Native update mask predicate missing'}
Save $predicate 'UpdatePredicate'
Save (Section $update '            else if (index == GAMEOBJECT_FIELD_FLAGS)' '            else if (index == GAMEOBJECT_FIELD_STATE_WORLD_EFFECT_ID') 'FlagsBranch'
Save (Section $update '                if (!canUseCampObject)' '                *data << uint16(dynFlags);') 'DynamicGate'
$p=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$remove=Section $p 'void Player::RemoveActiveQuest(' 'void Player::RemoveRewardedQuest('
$callback=Section $remove '        PhaseUpdateData phaseUdateData;' '    });'
Save $callback 'RemovalRefresh'
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/AridenCampInteractionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Ariden camp interaction regression failed'}
if($VerifyReleaseBinary){
 $map=Get-Content "$repo/build-extractors/bin/Release/worldserver.map" -Raw
 function Symbol([string]$name){
  $m=[regex]::Match($map,('(?m)^\s+\S+\s+'+[regex]::Escape($name)+'\S*\s+([0-9A-Fa-f]{16})\s+f'))
  if(!$m.Success){throw "Missing canonical symbol $name"}
  [Convert]::ToInt64($m.Groups[1].Value,16)
 }
 $gate=Symbol '?CanUseAridenCampObject@'
 $callers=@((Symbol '?Use@GameObject@@'),(Symbol '?ActivateToQuest@GameObject@@'),(Symbol '?BuildValuesUpdate@GameObject@@'))
 $binary=[IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe")
 $pe=[BitConverter]::ToInt32($binary,0x3c);$optional=$pe+24
 $stamp=[regex]::Match($map,'Timestamp is ([0-9A-Fa-f]+)')
 if(!$stamp.Success -or [Convert]::ToUInt32($stamp.Groups[1].Value,16) -ne [BitConverter]::ToUInt32($binary,$pe+8)){throw 'Canonical map/executable link timestamps differ'}
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
 $ends=@{};foreach($caller in $callers){$ends[$caller]=[long]::MaxValue}
 foreach($m in [regex]::Matches($map,'(?m)^\s+\S+\s+\S+\s+([0-9A-Fa-f]{16})\s+f')){
  $v=[Convert]::ToInt64($m.Groups[1].Value,16);foreach($caller in $callers){if($v -gt $caller -and $v -lt $ends[$caller]){$ends[$caller]=$v}}
 }
 foreach($caller in $callers){
  $at=Offset $caller;$length=$ends[$caller]-$caller;$found=$false
  if($length -le 0 -or $length -gt 131072){throw 'Unbounded canonical GameObject function'}
  for($i=0;$i -lt $length-4;$i++){
   if($binary[$at+$i] -eq 0xe8){
    try{$dest=Resolve ($caller+$i+5+[BitConverter]::ToInt32($binary,$at+$i+1))}catch{if($_.Exception.Message -eq 'Address outside canonical PE'){continue};throw}
    if($dest -eq $gate){$found=$true;break}
   }
  }
  if(!$found){throw "Canonical GameObject function $caller does not call camp admission"}
 }
 'PASS: deployed Release Use, ActivateToQuest and BuildValuesUpdate all call the camp admission gate'
}
