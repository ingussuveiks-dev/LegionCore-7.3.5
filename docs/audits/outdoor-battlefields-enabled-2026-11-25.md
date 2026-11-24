# Wintergrasp and Tol Barad enabled

The two battlefield settings now enable their normal entry, queue and map update paths in the canonical runtime. The distribution configuration uses the same settings. Initialization logs report the effective state for each battlefield. The Tol Barad initialization failure, if it occurs, is logged as an error.

The final Release linked the modified battlefield manager and the installed worldserver SHA-256 is `BBB9822B51716D628E34B6D11B069EACFFE2D75E9AD5D9C0CD8AC81ED4D3FE79`. An owned canonical startup reported both battlefields initialized and enabled. It reached ready and shut down normally, without appended DB errors, a crash or an unexplained warning. The deployed configuration retained its other settings. There were no manually forced public battles or player-state changes.

Complete battles, vehicle moves, ownership changes, captures, graveyard behavior and rewards still need actual client traversal. The distribution configuration retains the experimental status.
