# Broken Shore portal assault start

Yim reached Alliance scenario 786 step 4, shown as Stage 5, with 0/4 Shielded Anchors shattered. Varian and nearby allies remained idle after the reunion. The earlier screenshot and native 26972 criterion 28017 confirm that the Find Varian event 45228 is separate and had advanced.

Varian's explicit 120-yard attack against Eredar Chaos Guard 90525 ran only from an evade callback. The native reunion changes reaction and forms the allied group. It can leave Varian idle without a preceding fight that would invoke the callback. The Varian AI now searches when idle in the actual Alliance map 1460 portal stage. It uses the existing guard entry and range, checks native target validity, and starts ordinary combat. The scan runs at most once a second and stops when combat starts or the stage changes.

The 101667 Shielded Anchors retain their native guards, shield casts and real death-based criterion 45288. Neither the reunion nor the idle scan awards anchor progress. The Horde AI, existing reunion conversation and subsequent waypoint stages retain their own paths.

The active player-led check uses the deployed Release and the user's client. Separate automated/native and agent-owned startup checks were intentionally omitted. Verify Varian's movement, the allies joining the fight, all four anchor credits, and the next stage in game.
