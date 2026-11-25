-- Fel Fire's extinguishing spell arrives through its own SPELLHIT event.
-- Disconnect it from the preceding proximity aura action only.
UPDATE smart_scripts AS aura
JOIN smart_scripts AS hit
  ON hit.entryorguid = aura.entryorguid AND hit.source_type = aura.source_type
 AND hit.id = 1 AND hit.event_type = 8 AND hit.event_param1 = 207554
 AND hit.action_type = 33 AND hit.action_param1 = 104841 AND hit.link = 2
SET aura.link = 0
WHERE aura.entryorguid = 104841 AND aura.source_type = 0 AND aura.id = 0
  AND aura.event_type = 10 AND aura.action_type = 85 AND aura.action_param1 = 207552 AND aura.link = 1;

-- Velen starts conversation 119535 and the existing 12323200 timed list.
-- There is no authored link after the conversation action.
UPDATE smart_scripts AS conversation
JOIN smart_scripts AS entry_event
  ON entry_event.entryorguid = conversation.entryorguid AND entry_event.source_type = conversation.source_type
 AND entry_event.id = 0 AND entry_event.event_type = 54 AND entry_event.action_type = 80
 AND entry_event.action_param1 = 12323200 AND entry_event.link = 1
LEFT JOIN smart_scripts AS next_action
  ON next_action.entryorguid = conversation.entryorguid AND next_action.source_type = conversation.source_type
 AND next_action.id = conversation.link
SET conversation.link = 0
WHERE conversation.entryorguid = 123232 AND conversation.source_type = 0 AND conversation.id = 1
  AND conversation.event_type = 61 AND conversation.action_type = 220
  AND conversation.action_param1 = 119535 AND conversation.link = 2 AND next_action.id IS NULL;

-- Preserve the live correction for the single-event scenario object.
UPDATE smart_scripts AS scene
LEFT JOIN smart_scripts AS next_action
  ON next_action.entryorguid = scene.entryorguid AND next_action.source_type = scene.source_type
 AND next_action.id = scene.link
SET scene.link = 0
WHERE scene.entryorguid = 101 AND scene.source_type = 13 AND scene.id = 0
  AND scene.event_type = 89 AND scene.action_type = 205 AND scene.action_param1 = 92
  AND scene.link = 1 AND next_action.id IS NULL;
