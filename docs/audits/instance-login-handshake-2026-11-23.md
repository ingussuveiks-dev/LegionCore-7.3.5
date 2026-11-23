# Silent instance connection on the first client run

The deployed worldserver was `403A1ED742DE579E2A22D5A90E0A933B6894D0CB3951B29CEF04F7547603CE57`. Times are Europe/Riga.

At 10:12:51 the realm connection on port `8085` authenticated. At 10:12:57 the first instance connection on `8086` was accepted and the server sent the greeting. At the 10:13:07 deadline, Windows reported all 48 server bytes acknowledged, zero client payload bytes, zero unacknowledged bytes and zero retransmissions. The four remaining serials were rejected immediately at 10:13:09 without another accepted instance socket. The server never reached `HandleContinuePlayerLogin` for this attempt.

The failed client log records `Load Map End` and `Character Login SEND` at 10:12:57.107. At 10:13:09.263 it stopped waiting for an active player and destroyed the object manager. The connection log independently records `Handle Character Login Failed: 1`. The client log does not record whether the client application read the instance greeting.

The game client restarted at 10:15:51 with the same worldserver and bnetserver processes still running. On the same instance port at 10:16:05 the client sent its initializer in 15 ms, continued authentication in 31 ms and acknowledged encryption in 62 ms. Both server and client logs confirm entry into the same map `1666`. Both runs had finished the initial map load when they sent the login request. These observations do not explain why the failed client sent no instance initializer.

Failed and successful client logs and the failed server network excerpt are preserved in the ignored local incident directory `build-extractors/runtime-test/handshake-incidents/20261123-101519-067`. The failed TCP endpoint was already closed at capture, so the snapshot cannot establish application receive or dispatch. The crash directory and `DBErrors.log` were empty. No server, client, protocol, timeout or character state changes were made for this diagnosis.

This matches the unresolved pre-login failure in the [October incident](instance-login-handshake-2026-10-23.json), including its recovery by restarting only the client. A failed-run application-level receive or dispatch trace is still needed to distinguish where the client stopped processing the handshake.
