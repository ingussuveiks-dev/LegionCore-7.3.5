$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-WorldQuestRewardRelease.ps1"
$match=[regex]::Match($map,'\s(\?RewardWorldQuestItem@\S+)\s+[0-9A-Fa-f]{16}\s+f')
if(!$match.Success){throw 'Missing deployed world quest item helper'}
$helper=$match.Groups[1].Value
$address=Resolve (Symbol $helper)
$rewardCalls=@(Calls '?RewardQuest@Player@@')
if(@($rewardCalls | Where-Object {$_ -eq $address}).Count -ne 2){throw 'Both deployed recipe/main selection branches must use the delivery helper'}
$calls=@(Calls $helper)
foreach($name in @('?CanStoreItem@Player@@','?CanTakeMoreSimilarItems@Player@@','?StoreNewItem@Player@@','?CreateItem@Item@@','?SetItemRandomProperties@Item@@','?SetFixedLevel@Item@@','?RemoveFromUpdateQueueOf@Item@@','?AddItem@MailDraft@@','?SendMailTo@MailDraft@@')){
 if($calls -notcontains (Resolve (Symbol $name))){throw "Deployed delivery helper lacks $name"}
}
# SaveToDB is virtual (bags may override it). Resolve its actual Item vtable
# slot, then require that indirect dispatch in this bounded helper body.
$index=$map.IndexOf(' ??_7Item@@6B@')
if($index -lt 0){throw 'Missing native Item vtable'}
$line=$map.Substring($index,$map.IndexOf("`n",$index)-$index)
$data=[regex]::Match($line,'\s([0-9A-Fa-f]{16})\s+')
if(!$data.Success){throw 'Missing native Item vtable address'}
$vtable=Offset ([Convert]::ToInt64($data.Groups[1].Value,16))
$save=Resolve (Symbol '?SaveToDB@Item@@');$slot=-1
for($i=0;$i -lt 160;$i++){
 try{if((Resolve ([BitConverter]::ToInt64($binary,$vtable+8*$i))) -eq $save){$slot=8*$i;break}}catch{}
}
if($slot -lt 0){throw 'Missing native item serialization slot'}
$range=Bounds $helper;$at=Offset $range[0];$found=$false
for($i=0;$i -lt $range[1]-$range[0]-6;$i++){
 if($binary[$at+$i] -ne 0xff){continue};$mod=$binary[$at+$i+1]
 if($mod -ge 0x90 -and $mod -le 0x97 -and [BitConverter]::ToInt32($binary,$at+$i+2) -eq $slot){$found=$true}
 if($slot -lt 128 -and $mod -ge 0x50 -and $mod -le 0x57 -and $binary[$at+$i+2] -eq $slot){$found=$true}
}
if(!$found){throw 'Canonical delivery helper lacks native virtual item serialization'}
$out="$repo/build-extractors/runtime-test/world-quest-delivery"
$hashes=@{'actual-final-link-game.lib'=(Get-FileHash $lib).Hash;'canonical-worldserver.exe'=(Get-FileHash $exe).Hash;'Player.cpp'=(Get-FileHash "$repo/src/server/game/Entities/Player/Player.cpp").Hash}
$hashes | ConvertTo-Json | Set-Content "$out/release-inputs.json"
'PASS: canonical recipe/main calls both use the real delivery helper; exception-bounded helper calls native storage/unique admission, item initialization/queue cleanup, serialization and mail dispatcher. Actual final-link library/executable hashes recorded.'
