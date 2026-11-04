-- Matching local World data. No updates or character queries.
SELECT * FROM conversation_data WHERE entry BETWEEN 2442 AND 2447 ORDER BY entry,idx;
SELECT * FROM conversation_creature WHERE entry BETWEEN 2442 AND 2447 ORDER BY entry,id;
SELECT * FROM conversation_actor WHERE entry BETWEEN 2442 AND 2447 ORDER BY entry,id;
