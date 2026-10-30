param([string]$BaselineRevision,[switch]$VerifyReleaseBinary,[string]$InvokerCastRowsPath,[string]$BaselineInvokerCastRowsPath,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/script-pet-credit"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Extract([string]$s,[string]$start,[string]$end,[string]$name){
 $a=$s.IndexOf($start);$b=$s.IndexOf($end,$a)
 if($a -lt 0 -or $b -lt $a){throw "Missing production section $start"}
 [IO.File]::WriteAllText("$out/$name.inc",$s.Substring($a,$b-$a))
}
$fixed=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$s=$fixed
if($BaselineRevision){$s=(& git -C $repo show "${BaselineRevision}:src/server/game/AI/SmartScripts/SmartScript.cpp") -join "`n";if($LASTEXITCODE){throw 'Cannot read baseline'}}
Extract $fixed 'namespace' 'SmartScript::SmartScript()' 'CreditHelpers'
Extract $s '        case SMART_ACTION_INVOKER_CAST:' '        case SMART_ACTION_ADD_AURA:' 'InvokerCast'
Extract $s '        case SMART_ACTION_CALL_KILLEDMONSTER:' '        case SMART_ACTION_SET_INST_DATA:' 'KillAction'
Extract $s '        case SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA:' '        case SMART_ACTION_SUMMON_CONVERSATION:' 'CriteriaAction'
Extract $fixed 'namespace SmartInvokerPlayer' 'ObjectList* SmartScript::GetTargets(' 'PlayerResolver'
Extract $fixed '        case SMART_TARGET_ACTION_INVOKER_PLAYER:' '        case SMART_TARGET_POSITION:' 'PlayerTarget'
$targetSource=$fixed.Substring($fixed.IndexOf('ObjectList* SmartScript::GetTargets('))
$targetSource=$targetSource.Substring(0,$targetSource.IndexOf('ObjectList* SmartScript::GetWorldObjectsInDist('))
[IO.File]::WriteAllText("$out/TargetTail.inc",[regex]::Replace($targetSource.Substring($targetSource.LastIndexOf('    if (GetBaseObject() && GetBaseObject()->HasQuestObjectScope())')),'\bl\b','targets'))
$target=Get-Content "$out/PlayerTarget.inc" -Raw
[IO.File]::WriteAllText("$out/PlayerTarget.inc",([regex]::Replace([regex]::Replace($target,'\bl\b','targets'),'\btrigger\b','currentInvoker')).Replace('e.target.','event.target.'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.cpp" -Raw
Extract $source '        case SMART_ACTION_ADD_AURA:' '        case SMART_ACTION_CALL_AREAEXPLOREDOREVENTHAPPENS:' 'CastValidation'
$validation=Get-Content "$out/CastValidation.inc" -Raw
[IO.File]::WriteAllText("$out/CastValidation.inc",[regex]::Replace($validation,'\be\b','event'))
$s=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
Extract $s 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const' 'Player* Unit::GetAffectingPlayer() const' 'OwnerPlayer'
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
Extract $s 'enum SmartCastFlags' '// one line in DB' 'CastFlags'
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$rowArgument=if($InvokerCastRowsPath){' "'+[IO.Path]::GetFullPath($InvokerCastRowsPath)+'"'}else{''}
if($BaselineInvokerCastRowsPath){if(!$InvokerCastRowsPath){throw 'Baseline tuple requires a repaired tuple'};$rowArgument+=' "'+[IO.Path]::GetFullPath($BaselineInvokerCastRowsPath)+'"'}
$compileOptions=if($AddressSanitizer){'/fsanitize=address /Zi /Fd"'+$out+'/test.pdb" '}else{''}
$linkOptions=if($AddressSanitizer){' /link /INCREMENTAL:NO'}else{''}
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 $compileOptions/I`"$out`" `"$PSScriptRoot/ScriptPetCreditTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"$linkOptions`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"$rowArgument`r`nexit /b %errorlevel%`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Global SmartAI pet credit regression failed'}
if($VerifyReleaseBinary){
 $map=Get-Content "$repo/build-extractors/bin/Release/worldserver.map" -Raw
 $binary=[IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe")
 $pe=[BitConverter]::ToInt32($binary,0x3c);$optional=$pe+24
 $stamp=[regex]::Match($map,'Timestamp is ([0-9A-Fa-f]+)')
 if(!$stamp.Success -or [Convert]::ToUInt32($stamp.Groups[1].Value,16) -ne [BitConverter]::ToUInt32($binary,$pe+8)){throw 'Canonical map/executable link timestamps differ'}
 $imageBase=[BitConverter]::ToInt64($binary,$optional+24)
 $sections=$optional+[BitConverter]::ToUInt16($binary,$pe+20)
 function Symbol([string]$name){
  $m=[regex]::Match($map,('(?m)^\s+\S+\s+'+[regex]::Escape($name)+'\S*\s+([0-9A-Fa-f]{16})\s+f'))
  if(!$m.Success){throw "Missing canonical symbol $name"}
  [Convert]::ToInt64($m.Groups[1].Value,16)
 }
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
 $action=Symbol '?ProcessAction@SmartScript@@'
 $caster=Symbol '?GetScriptCreditCaster@'
 $player=Symbol '?GetScriptCreditPlayer@'
 $owner=Symbol '?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@'
 $ends=@{};$ends[$action]=[long]::MaxValue;$ends[$caster]=[long]::MaxValue
 foreach($m in [regex]::Matches($map,'(?m)^\s+\S+\s+\S+\s+([0-9A-Fa-f]{16})\s+f')){
  $v=[Convert]::ToInt64($m.Groups[1].Value,16)
  foreach($caller in @($action,$caster)){if($v -gt $caller -and $v -lt $ends[$caller]){$ends[$caller]=$v}}
 }
 $calls=@{}
 foreach($caller in @($action,$caster)){
  $calls[$caller]=@{};$at=Offset $caller;$length=$ends[$caller]-$caller
  if($length -le 0 -or $length -gt 524288){throw 'Unbounded canonical SmartScript function'}
  for($i=0;$i -lt $length-4;$i++){
   if($binary[$at+$i] -eq 0xe8){
    try{$dest=Resolve ($caller+$i+5+[BitConverter]::ToInt32($binary,$at+$i+1))}catch{if($_.Exception.Message -eq 'Address outside canonical PE'){continue};throw}
    if($dest -eq $player -or $dest -eq $caster -or $dest -eq $owner){if(!$calls[$caller].ContainsKey($dest)){$calls[$caller][$dest]=0};++$calls[$caller][$dest]}
   }
  }
 }
 # MSVC can inline GetScriptCreditPlayer into the caster helper, leaving its
 # direct call to the native Unit owner resolver instead.
 if($calls[$action][$player] -lt 3 -or $calls[$action][$caster] -lt 1 -or ($calls[$caster][$player] -lt 1 -and $calls[$caster][$owner] -lt 1)){throw "Canonical SmartScript credit calls: ProcessAction->player=$($calls[$action][$player]), ProcessAction->caster=$($calls[$action][$caster]), caster->player=$($calls[$caster][$player]), caster->owner=$($calls[$caster][$owner])"}
 'PASS: canonical Release ProcessAction calls the credit resolver and caster bridge; PE/map timestamps and incremental-link targets checked'
}
