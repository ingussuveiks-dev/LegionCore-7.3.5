$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$runtime="$repo/build-extractors/bin/Release"
$names=@('?SetGUID@npc_stormheim_foothold_batrider@@','?UpdateAI@npc_stormheim_foothold_batrider@@','?BeginExit@npc_stormheim_foothold_batrider@@','?sGossipSelect@npc_stormheim_foothold_batrider@@','?EnterVehicle@Unit@@','?SetClientControl@Player@@','?MovePoint@MotionMaster@@QEAAXIMMM_NM@Z','?ExitVehicle@Unit@@','?SummonCreature@WorldObject@@QEBAPEAVTempSummon@@IAEBUPosition@@W4TempSummonType@@')
$names+='?MoveNext@npc_stormheim_foothold_batrider@@'
$rgArgs=@('-A','2','--fixed-strings');foreach($name in $names){$rgArgs+=@('-e',$name)}
$rgArgs+="$runtime/worldserver.map"
$map=(& rg @rgArgs) -join "`n"
if($LASTEXITCODE){throw 'Cannot read canonical link-map symbols'}
$binary=[IO.File]::ReadAllBytes("$runtime/worldserver.exe")
$pe=[BitConverter]::ToInt32($binary,0x3c);$optional=$pe+24
$imageBase=[BitConverter]::ToInt64($binary,$optional+24)
$sections=$optional+[BitConverter]::ToUInt16($binary,$pe+20)
function FileOffset([long]$address){
 $relative=$address-$imageBase
 for($j=0;$j -lt [BitConverter]::ToUInt16($binary,$pe+6);$j++){
  $section=$sections+40*$j;$start=[BitConverter]::ToUInt32($binary,$section+12)
  $size=[Math]::Max([BitConverter]::ToUInt32($binary,$section+8),[BitConverter]::ToUInt32($binary,$section+16))
  if($relative -ge $start -and $relative -lt $start+$size){return [long]([BitConverter]::ToUInt32($binary,$section+20)+$relative-$start)}
 }
 throw 'Address outside canonical PE sections'
}
function ResolveThunk([long]$address){
 for($j=0;$j -lt 10;$j++){$at=FileOffset $address;if($binary[$at] -ne 0xe9){return $address};$address=$address+5+[BitConverter]::ToInt32($binary,$at+1)}
 throw 'Unresolved Release thunk'
}
function FindSymbol([string]$name){
 $match=[regex]::Match($map,('(?m)^\s+\S+\s+'+[regex]::Escape($name)+'\S*\s+([0-9A-Fa-f]{16})\s+f[^\r\n]*'))
 if(!$match.Success){throw "Missing fresh Release link symbol $name (generate worldserver.map during the link)"}
 return $match
}
function Calls([string]$from,[string]$to){
 $symbol=FindSymbol $from;$address=[Convert]::ToInt64($symbol.Groups[1].Value,16)
 $target=ResolveThunk ([Convert]::ToInt64((FindSymbol $to).Groups[1].Value,16))
 $tail=$map.Substring($symbol.Index+$symbol.Length,[Math]::Min(8192,$map.Length-$symbol.Index-$symbol.Length))
 $end=$address+4096
 foreach($m in [regex]::Matches($tail,'\s([0-9A-Fa-f]{16})\s+f')){$v=[Convert]::ToInt64($m.Groups[1].Value,16);if($v -gt $address){$end=[Math]::Min($end,$v)}}
 $body=ResolveThunk $address;$at=FileOffset $body
 for($i=0;$i -lt [Math]::Min(4096,$end-$address)-4;$i++){
  if($binary[$at+$i] -eq 0xe8 -or $binary[$at+$i] -eq 0xe9){
   $dest=$body+$i+5+[BitConverter]::ToInt32($binary,$at+$i+1)
   # This bounded byte scan also sees immediates containing E8/E9. Reject
   # non-image destinations; only a resolved native function address counts.
   try{$resolved=ResolveThunk $dest}catch{continue}
   if($resolved -eq $target){return}
  }
 }
 throw "Canonical Release call missing: $from -> $to"
}
Calls '?SetGUID@npc_stormheim_foothold_batrider@@' '?EnterVehicle@Unit@@'
Calls '?UpdateAI@npc_stormheim_foothold_batrider@@' '?SetClientControl@Player@@'
Calls '?UpdateAI@npc_stormheim_foothold_batrider@@' '?MoveNext@npc_stormheim_foothold_batrider@@'
Calls '?MoveNext@npc_stormheim_foothold_batrider@@' '?MovePoint@MotionMaster@@QEAAXIMMM_NM@Z'
Calls '?BeginExit@npc_stormheim_foothold_batrider@@' '?ExitVehicle@Unit@@'
Calls '?sGossipSelect@npc_stormheim_foothold_batrider@@' '?SummonCreature@WorldObject@@QEBAPEAVTempSummon@@IAEBUPosition@@W4TempSummonType@@'
'PASS: canonical Release Bat-Rider machine-code calls boarding, active-mover handoff, point movement, native exit and personal summon. This verifies deployed calls, not client behavior or every branch.'
