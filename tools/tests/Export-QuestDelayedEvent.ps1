param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$definitions=Get-Content "$repo/src/server/game/Quests/QuestDef.h" -Raw
$start=$definitions.IndexOf('typedef std::map<int32/*idx*/, int32/*data*/> QuestStatusDatas;')
$end=$definitions.IndexOf('#endif',$start)
[IO.File]::WriteAllText("$OutputDirectory/QuestStatusData.inc",$definitions.Substring($start,$end-$start))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$start=$source.IndexOf('void Player::AddQuestDelayedEvent(')
$end=$source.IndexOf('void Player::RemoveActiveQuest(',$start)
[IO.File]::WriteAllText("$OutputDirectory/QuestDelayedEvent.inc",$source.Substring($start,$end-$start))
# Acceptance must invalidate even a retained status record; NONE invalidates
# without erasing. Normal abandon/reward destroys the status in RemoveActiveQuest.
if($source -notmatch 'QuestStatusData& status_q = m_QuestStatus\[quest_id\];\s+status_q.ScriptLifetime.reset\(\);' -or
   $source -notmatch 'if \(status == QUEST_STATUS_NONE\)\s+q_status.ScriptLifetime.reset\(\);' -or
   $source -notmatch 'm_QuestStatus.erase\(quest_id\);'){throw 'Missing quest lifetime invalidation'}
