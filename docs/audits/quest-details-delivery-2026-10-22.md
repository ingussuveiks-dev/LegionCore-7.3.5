# Quest details delivery — 2026-10-22

Client report: accepting Hemet Nesingwary's Bear Huntin' adds it to the log but leaves its Accept/Decline window open.

SendQuestGiverQuestDetails called packet.Write() twice and dispatched both results. Write appends the full serialization to the existing WorldPacket: the second message contains two concatenated quest-detail bodies. Remove the duplicate send; retain the existing quest accept handler's SendCloseGossip and the original 7.3.5 empty GossipComplete packet.

The native client QuestFrame.lua listens for QUEST_FINISHED to hide the frame. Its Accept button calls AcceptQuest without forcibly hiding itself. An addon that simply hides the frame would conceal protocol trouble. No UI suppression is added.

The primary [WowPacketParser gossip decoder](https://github.com/TrinityCore/WowPacketParser/blob/master/WowPacketParser/Parsing/Parsers/NpcHandler.cs) confirms GossipComplete has no body before 9.2. Modern Trinity's SuppressSound bit must not be copied into this 7.3.5 packet.

Test-QuestDetailsDelivery.ps1 executes the extracted production dispatch against an append-on-Write packet and checks exactly one serialization, one send and one body. This detects the original double send. verify_highmountain_quest_release.py also counts exactly one native QuestGiverQuestDetails::Write call in the actual canonical PE method. Canonical startup/database validation passed and the owned hidden server was shut down gracefully.

Actual quest-window closure after acceptance remains a client check; this correction alone does not prove that symptom resolved.
