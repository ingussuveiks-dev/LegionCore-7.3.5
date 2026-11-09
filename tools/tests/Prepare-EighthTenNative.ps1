param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$plan=Get-Content "$PSScriptRoot/EighthTenRepairs.json" -Raw|ConvertFrom-Json
$source='';$records=@()
foreach($case in $plan|Where-Object {$_.npc -and $_.key -ne 'WebbedPool'}){
 if($case.key -eq 'Ghoul'){$body='return killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;'}else{
  $line=($case.edits[0].new -split "`n"|Where-Object {$_ -match 'Player\s*\*\s*player\s*=.*GetCharmerOrOwnerPlayerOrPlayerItself'}|Select-Object -First 1).Trim();if(!$line){throw 'Missing actual owner declaration'}
  if($line.StartsWith('if (')){$line=$line.Substring(4).TrimEnd(')')+';'}
  $body=$line+' return player;'
 }
 $argument=if($case.key -eq 'Baleful'){'attacker'}else{'killer'}
 $source+="static Player* EighthResolve$($case.key)(Unit*$argument){$body}`n";$records+="{$($case.npc),&EighthResolve$($case.key)}"
}
$source+='struct EighthResolver{uint32 entry;Player*(*resolve)(Unit*);};static EighthResolver eighthResolvers[]={'+($records -join ',')+'};'
$web=Get-Content "$repo/src/server/scripts/Kalimdor/zone_bloodmyst_isle.cpp" -Raw -Encoding utf8
$pool=[regex]::Match($web,'uint32 const possibleSpawns\[32\]\s*=\s*\{[^}]+\};').Value;if(!$pool){throw 'Missing actual32outcome table'}
$source+=$pool.Replace('possibleSpawns','eighthPossibleSpawns')
$hyjal=Get-Content "$repo/src/server/scripts/World/player_special.cpp" -Raw -Encoding utf8
$arrays=[regex]::Matches($hyjal,'uint32 (random|allianceNpc|hordeNpc)\[(2|33)\]\s*=\s*\{([^}]+)\};');if($arrays.Count -ne 3){throw 'Missing original faction/role arrays'}
$spells=@(96544,96547);foreach($array in $arrays){$spells+=@($array.Groups[3].Value.Split(',')|ForEach-Object {[uint32]$_.Trim()})}
$source+='static uint32 eighthHyjalSpells[]={'+(($spells|Sort-Object -Unique)-join ',')+'};'
[IO.File]::WriteAllText("$OutputDirectory/EighthResolvers.inc",$source)
