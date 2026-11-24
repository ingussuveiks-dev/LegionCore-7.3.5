# Talonclaw departure dialogue admission

In the live Talonclaw run, executing the native claim spell after the confirmed pickup advanced The Spear in the Shadow to Return to Dalaran. Dorro and the return portal still did not appear. A separate use of the portal's native travel spell reached Dalaran and triggered the existing arrival-credit hook.

The live database has Dorro 100005's original line in group 0 and departure BroadcastText 106397 in group 1. Both groups use MinTimer and MaxTimer zero. CreatureTextMgr::GetTextGroup serves timed text and returns null when all MinTimer values in the group are zero. TalonclawDeparture::Ready was checking for this timed group even though the departure controller schedules Dorro's dialogue itself. This rejected an otherwise valid departure before creation of either actor.

Admission now uses the existing HasBroadCastText lookup, requiring the authored departure text in its actual group 1. The two lines, the ten-second dialogue delay, the six-second portal delay, owner scope, completed pickup, cave admission, and real portal interaction stay in the existing controller. A portal creation log records successful publication for the next live check. No SQL or character data was modified by the repair.

The expiry repair and this fix are deployed together once the user stops worldserver. Separate regressions and an agent-owned startup are omitted under the active gameplay workflow. A new client acquisition must still confirm scene completion, Dorro visibility and dialogue, real portal use, and Emmarel turn-in.
