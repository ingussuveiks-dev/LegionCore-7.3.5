UPDATE creature_template SET ScriptName='npc_stormheim_foothold_batrider'
WHERE entry=97250 AND gossip_menu_id=18762 AND VehicleId=4387
  AND ScriptName IN('','npc_stormheim_foothold_batrider');

-- Existing Hotfix BroadcastText98963 supplies this text to the client. Keep
-- the fallback consistent, rather than the unrelated old catapult wording.
UPDATE gossip_menu_option SET OptionText='Can you give me a lift back to the Foothold?'
WHERE MenuID=18762 AND OptionID=0 AND OptionBroadcastTextID=98963
  AND OptionText IN('Can you throw me at a stronghold?','Can you give me a lift back to the Foothold?');
