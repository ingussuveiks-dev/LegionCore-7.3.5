$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-VrektRelease.ps1"
foreach($path in @('src/server/game/Entities/Player/Player.cpp','src/server/game/Entities/Player/Player.h')){
 if((Get-Item "$repo/$path").LastWriteTimeUtc -gt (Get-Item $lib).LastWriteTimeUtc){throw "Actual final-link library predates $path"}
}
$admission='?CanRewardWorldQuest@Player@@'
$gate=Resolve (Symbol $admission)
foreach($entry in @('?CompleteQuest@Player@@','?RewardQuest@Player@@')){
 $calls=@(Calls $entry)
 $gateIndex=[Array]::IndexOf($calls,$gate)
 if($gateIndex -lt 0){throw "Canonical $entry lacks world-quest admission"}
 $mutations=if($entry -like '?CompleteQuest*'){@('?SetQuestStatus@Player@@')}else{@('?RemoveActiveQuest@Player@@','?StoreNewItem@Player@@')}
 foreach($name in $mutations){
  $mutationIndex=[Array]::IndexOf($calls,(Resolve (Symbol $name)))
  if($mutationIndex -lt 0 -or $mutationIndex -le $gateIndex){throw "Canonical $entry admission does not precede $name"}
 }
}
$calls=@(Calls $admission)
foreach($name in @('?GetWorldQuest@QuestDataStoreMgr@@','?GetGameTime@GameTime@@','?WorldQuestCompleted@Player@@')){
 if($calls -notcontains (Resolve (Symbol $name))){throw "Canonical reward admission lacks $name"}
}
'PASS: actual final-link Player source/header freshness; canonical completion/direct-reward gates precede mutations and check rotation, native clock and personal completion. Incremental thunks resolved inside native exception-bounded functions.'
