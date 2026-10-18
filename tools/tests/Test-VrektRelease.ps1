$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$exe="$repo/build-extractors/bin/Release/worldserver.exe"
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'game\\Release\\game.lib$'} | Select-Object -First 1
if(!$relative){throw 'Missing actual game final-link input'}
$lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
if((Get-Item "$repo/src/server/game/Globals/QuestData.cpp").LastWriteTimeUtc -gt (Get-Item $lib).LastWriteTimeUtc -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical executable/core library predates quest activation source'}
$map=Get-Content "$repo/build-extractors/bin/Release/worldserver.map" -Raw
function Symbol([string]$name){
 $m=[regex]::Match($map,('(?m)^\s+\S+\s+'+[regex]::Escape($name)+'\S*\s+([0-9A-Fa-f]{16})\s+f'))
 if(!$m.Success){throw "Missing canonical symbol $name"};[Convert]::ToInt64($m.Groups[1].Value,16)
}
$binary=[IO.File]::ReadAllBytes($exe);$pe=[BitConverter]::ToInt32($binary,0x3c);$optional=$pe+24
$imageBase=[BitConverter]::ToInt64($binary,$optional+24);$sections=$optional+[BitConverter]::ToUInt16($binary,$pe+20)
function Offset([long]$address){
 $rva=$address-$imageBase
 for($i=0;$i -lt [BitConverter]::ToUInt16($binary,$pe+6);$i++){
  $section=$sections+40*$i;$start=[BitConverter]::ToUInt32($binary,$section+12)
  $size=[Math]::Max([BitConverter]::ToUInt32($binary,$section+8),[BitConverter]::ToUInt32($binary,$section+16))
  if($rva -ge $start -and $rva -lt $start+$size){return [long]([BitConverter]::ToUInt32($binary,$section+20)+$rva-$start)}
 };throw 'Address outside canonical PE'
}
function Resolve([long]$address){
 for($i=0;$i -lt 10;$i++){$at=Offset $address;if($binary[$at] -ne 0xe9){return $address};$address+=5+[BitConverter]::ToInt32($binary,$at+1)}
 throw 'Unexpected incremental-link thunk cycle'
}
# Bound each x64 function with the executable's own exception directory,
# including when the public MAP symbol resolves through an incremental thunk.
$pdata=Offset ($imageBase+[BitConverter]::ToUInt32($binary,$optional+112+3*8))
$entries=[BitConverter]::ToUInt32($binary,$optional+112+3*8+4)/12
function Bounds([string]$name){
 $address=Resolve (Symbol $name);$rva=$address-$imageBase;$low=0;$high=[int]$entries-1
 while($low -le $high){$mid=[int][Math]::Floor(($low+$high)/2);$at=$pdata+12*$mid
  $start=[BitConverter]::ToUInt32($binary,$at);$end=[BitConverter]::ToUInt32($binary,$at+4)
  if($rva -lt $start){$high=$mid-1}elseif($rva -ge $end){$low=$mid+1}else{return @($address,[long]($imageBase+$end))}
 };throw "Missing x64 function range $name"
}
function Calls([string]$name){
 $range=Bounds $name;$at=Offset $range[0]
 for($i=0;$i -lt $range[1]-$range[0]-4;$i++){
  if($binary[$at+$i] -eq 0xe8){try{Resolve ($range[0]+$i+5+[BitConverter]::ToInt32($binary,$at+$i+1))}catch{}}
 }
}
$admission='?CanBeActivate@QuestDataStoreMgr@@';$range=Bounds $admission;$at=Offset $range[0];$markers=0
for($i=0;$i -lt $range[1]-$range[0]-3;$i++){
 if($binary[$at+$i] -eq 0x86 -and $binary[$at+$i+1] -eq 0x31 -and $binary[$at+$i+2] -eq 0 -and $binary[$at+$i+3] -eq 0){++$markers}
}
if($markers -lt 2){
 # MSVC can hoist12678 into one register and reuse it for the uint32
 # candidate variable and native uint16 WorldQuestState key comparisons.
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $dump=Get-ChildItem "$vs/VC/Tools/MSVC/*/bin/Hostx64/x64/dumpbin.exe" | Select-Object -Last 1
 if(!$dump){throw 'Native MSVC disassembler unavailable'}
 $argument='/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]
 $listing=(& $dump.FullName /DISASM:NOBYTES $argument $exe) -join "`n"
 if($LASTEXITCODE){throw 'Native bounded admission disassembly failed'}
 $constant=[regex]::Match($listing,'\bmov\s+e(ax|bx|cx|dx|si|di|bp),3186h')
 $register=$constant.Groups[1].Value
 if(!$constant.Success -or $listing -notmatch ('\bcmp\s+dword ptr \[[^\]]+\],e'+$register+'\b') -or
  $listing -notmatch ('\bcmp\s+word ptr \[[^\]]+\],'+$register+'\b')){throw 'Deployed admission lacks both candidate and active-state12678 comparisons'}
 $out="$repo/build-extractors/runtime-test/vrekt-world-quest"
 New-Item -ItemType Directory -Force $out | Out-Null
 $listing | Set-Content "$out/admission-disassembly.txt"
}
if(@(Calls $admission) -notcontains (Resolve (Symbol '?GetGameTime@GameTime@@'))){throw 'Deployed admission lacks native time lookup'}
if(@(Calls '?GenerateWorldQuestUpdate@QuestDataStoreMgr@@') -notcontains (Resolve (Symbol $admission))){throw 'Deployed automatic scheduler does not call admission'}
'PASS: actual canonical game.lib source freshness; PE exception-bounded admission contains both shared-state12678 checks and clock call; deployed automatic scheduler calls it. Binary inspection supplements native behavioral tests; no live quest/client claim.'
