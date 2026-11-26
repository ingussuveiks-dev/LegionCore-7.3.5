# Stolen Malt Sack after the fifth grain

The client showed objective 42767 ready for turn-in and 5/5 on a Stolen Malt Sack. A second interaction still opened an empty loot window. In the world database, chest 210001 contains quest-only items 138623 (five for 42767) and 77033 (eight for 30028). Chests 251422 and 251424 share quest-only loot 66720. Stolen Barley Sack 210002 has different loot with unconditional rows and is outside the change.

On map 870 the three quest-only chests now use their actual loot template and the viewer's native outstanding item objectives for their selectable and dynamic flags, use handler and loot request. Player quest-object updates resend both fields after an item is acquired. Shared spawn state and item credit remain native. A player still collecting the other quest's item can use the shared sack.

Release recompiled the game library and the final worldserver link used the ordinary Release game and scripts libraries. The map contains GranaryGrainSack::IsUnavailable from GameObject.obj. The staged executable SHA-256 is A9C6AC78696A9E4FF45D0A2955C789A2332A38A2603A0172C91846AF69FC6476. No separate test or agent-owned startup was run during active gameplay. Installation and the client cursor before and after the fifth grain are separate checks.
