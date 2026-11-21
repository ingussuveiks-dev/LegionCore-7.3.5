-- Use the existing four questgivers and keep their ordinary quest menu and reward routing.
UPDATE `creature_template` SET `ScriptName` = 'npc_hallows_end_crate_return'
WHERE `entry` IN (53949, 54118, 54141, 54170) AND `ScriptName` = '' AND `AIName` = '';

-- Replace the older introduction and abbreviated conclusion installed with Taking Precautions.
DELETE FROM `creature_text` WHERE (`CreatureID`, `GroupID`, `ID`, `comment`) IN (
    (54021, 0, 0, 'Hudson Barnes - question Quincy'), (54021, 1, 0, 'Hudson Barnes - address the witness'),
    (54146, 0, 0, 'Delian Sunshade - question Dran'), (54146, 1, 0, 'Delian Sunshade - address the witness'));

-- Reconstructed English dialogue. Groups follow the order in each faction's scene.
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `comment`) VALUES
(54021, 10, 0, 'Quincy, I know the thief showed you the crate. Where did he take it?', 12, 100, 'Hallow End - Shady Lady scene'),
(54021, 11, 0, 'Its contents are dangerous. They belong under lock and key, far from anyone who would meddle with them.', 12, 100, 'Hallow End - Shady Lady scene'),
(53991, 12, 0, 'Lower your voice. I still have customers to serve.', 12, 100, 'Hallow End - Shady Lady scene'),
(53991, 13, 0, 'Why waste time on a crate? I can offer you a fair price on some new pieces from Uldum.', 12, 100, 'Hallow End - Shady Lady scene'),
(54021, 14, 0, 'Perhaps the city guard would like to know where you got those pieces. They might ask about the painting, too.', 12, 100, 'Hallow End - Shady Lady scene'),
(53991, 15, 0, 'All right. I gave him the name of a private collector who wanted to buy what was inside.', 12, 100, 'Hallow End - Shady Lady scene'),
(54021, 16, 0, 'That gives me somewhere to start. Now, who sent you looking for the crate?', 12, 100, 'Hallow End - Shady Lady scene'),
(54146, 10, 0, 'Dran, the thief knew exactly which shop would take his stolen crate. I know he was here.', 12, 100, 'Hallow End - Salvage scene'),
(6986, 11, 0, 'I do not know what you mean. Why are you poking around my shop?', 12, 100, 'Hallow End - Salvage scene'),
(6986, 12, 0, 'There are a few things from Uldum I could set aside for the Reliquary, if that is what brought you here.', 12, 100, 'Hallow End - Salvage scene'),
(54146, 13, 0, 'Then perhaps we should ask the grunts what they make of those antiquities.', 12, 100, 'Hallow End - Salvage scene'),
(6986, 14, 0, 'Keep it down! I put the thief in touch with a buyer. That is all I did.', 12, 100, 'Hallow End - Salvage scene'),
(54146, 15, 0, 'Good. Now we have a buyer to look for.', 12, 100, 'Hallow End - Salvage scene'),
(54146, 16, 0, 'And what brings you here? Tell me what you know about the crate.', 12, 100, 'Hallow End - Salvage scene'),
(54118, 10, 0, 'You may think I have betrayed you, but Hastings offered me a chance I cannot pass up.', 15, 100, 'Hallow End - Golden Keg exchange'),
(53949, 10, 0, 'Barnes. I trust you have the crate?', 15, 100, 'Hallow End - Anson ritual'),
(54118, 11, 0, 'Our friend brought it back. First, show me you have the charm.', 15, 100, 'Hallow End - Golden Keg exchange'),
(53949, 11, 0, 'I brought it, and I will unlock its power as soon as the crate is mine.', 15, 100, 'Hallow End - Anson ritual'),
(53949, 12, 0, 'With this charm you can explore Karazhan undisturbed and make whatever discoveries you find there your own.', 15, 100, 'Hallow End - Anson ritual'),
(54118, 12, 0, 'Take the crate. I have waited a long time to get into that library.', 15, 100, 'Hallow End - Golden Keg exchange'),
(53949, 13, 0, 'Everything is ready. I have prepared for this transformation for years.', 15, 100, 'Hallow End - Anson ritual'),
(54118, 13, 0, 'Wait. How do I use the charm?', 15, 100, 'Hallow End - Golden Keg exchange'),
(53949, 14, 0, 'This was not supposed to happen. The pain!', 15, 100, 'Hallow End - Anson ritual'),
(53949, 15, 0, 'You took a great risk bringing back my family''s crate. You have my thanks.', 15, 100, 'Hallow End - Anson ritual'),
(54170, 10, 0, 'Goodwin has offered the Reliquary a remarkable discovery in exchange for his belongings.', 15, 100, 'Hallow End - Wyvern Tail exchange'),
(54141, 10, 0, 'Let us get on with it, Sunshade. Have you recovered what is mine?', 15, 100, 'Hallow End - Edgar ritual'),
(54170, 11, 0, 'The crate is here. Did you bring the grimoire?', 15, 100, 'Hallow End - Wyvern Tail exchange'),
(54141, 11, 0, 'I did. Medivh''s grimoire will be a worthy addition to the Reliquary.', 15, 100, 'Hallow End - Edgar ritual'),
(54141, 12, 0, 'You will have it as soon as I have the crate.', 15, 100, 'Hallow End - Edgar ritual'),
(54170, 12, 0, 'The crate is yours, Goodwin. Belloc will be pleased with our exchange.', 15, 100, 'Hallow End - Wyvern Tail exchange'),
(54141, 13, 0, 'At last I can finish the spell. Watch what it makes of me.', 15, 100, 'Hallow End - Edgar ritual'),
(54170, 13, 0, 'The Reliquary may be keeping both after all.', 15, 100, 'Hallow End - Wyvern Tail exchange'),
(54141, 14, 0, 'Something has gone wrong. It burns!', 15, 100, 'Hallow End - Edgar ritual');
