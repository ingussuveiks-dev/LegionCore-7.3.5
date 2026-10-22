# Socket authentication lifetime, 2026-10-22

Status: the logout lifetime defect is reproduced and repaired in source, with
native regression and linked Release checks passing. The canonical Release is
deployed and runtime probes passed; Maiko's real client login, logout/relogin and
full client restart succeeded. The separate
silent first-instance-handshake failure remains unresolved.

## Observed failure

Maiko logged out at 09:26:04. The old authenticated instance connection then
reported an initialization timeout at age 684834 ms, with initializer=1,
continued-auth=1 and substantial earlier bidirectional traffic. Krau successfully
logged in at 09:39:33, but logout at 09:40:28 produced the same incorrect timeout
classification for that established connection, at age 55149 ms.

`LogoutPlayer` intentionally detaches the instance socket's session, then closes
it. `SetWorldSession(nullptr)` reset `_authed` to false. `WorldSocket::Update`
checked the ten-second initialization deadline before checking whether the socket
was already closed. An established, intentionally retired socket was consequently
treated as an unfinished handshake. The real loopback fixture, executing the
extracted production setter, failed before the fix with:
`FAIL: session detach reset a completed socket authentication`.

## Repair and checks

Authentication now remains latched for the socket's lifetime when session
ownership is detached. Update rejects closed/closing sockets before processing
initialization or queued traffic. Late duplicate realm/continued-auth messages
remain rejected, with safe diagnostics when the owning session is already null.
The ten-second deadline and five-attempt connection policy remain in force for
actual unfinished handshakes. No diagnostic severity or error output was disabled.

- `Test-HandshakeDeadline.ps1`: real loopback expiry, cancellation and lifetime
  release; ordinary open-peer EOF; authenticated and young sockets; extracted
  session binding/detach; duplicate authentication with and without an attached
  session; already closed sockets.
- `Test-InstanceConnectRetry.ps1`: silent-client deadline, five attempts, queued
  reports, stale reports, successful/cancelled login, stale/duplicate socket links.
- `Test-WorldSocketDiagnostics.ps1`: native TCP counters and SDK ABI, response
  receive-count change, safe closed-socket diagnostic failure.
- `Test-HandshakeDeadline.ps1 -VerifyReleaseBinary -ReleaseDirectory
  build-extractors/runtime-test/handshake-deadline/Release`: resolves linked PE
  symbols/thunks, checks the constant authentication latch and early close guards,
  plus the existing five handshake diagnostic and ten scenario lifecycle calls.
  The same verifier rejects the previous canonical executable as expected.

Release game.lib was built in its original generated dependency location,
`build-extractors/src/server/game/Release/game.lib`. Only the final executable
link used a separate OutDir, with BuildProjectReferences=false. The staged
EXE/MAP are under `build-extractors/runtime-test/handshake-deadline/Release`.
Staged executable SHA256:
`b5c22c23e08e1404b40b651a728c9dbd183ab14e5243a9dbb55d46e0d4a4d533`.

## Separate unresolved login and UI reports

Maiko's attempts at 09:26:22, 09:34:08 and 09:40:33 reached the listener on
127.0.0.1:8086. After ten seconds TCP counters showed 48 bytes sent, zero
received, zero unacknowledged and zero retransmitted. The client then rejected
the later connection serials immediately. Its log showed map 1220 loading and
Character Login SEND, followed by failure while waiting for the active player.
The server had not reached Maiko's Player construction/load. A complete client
restart failed to recover this run. Krau succeeded on the same server; switching
back to Maiko in that client still failed. None of this proves corrupted Maiko
data or that moving the player portrait caused the login failure.

Private incidents were preserved under ignored runtime-test/handshake-incidents
(20261022-093606-189 and 20261022-094028-598). A temporary client hardware-debug
observation ended with the client exiting and captured no callback evidence; it
was discontinued. No client file, account cache, character position, quest or
pet data was changed. The user's servers were not stopped or relaunched by the
agent during this investigation.

PlayerFrame.lua, PlayerFrame.xml, UnitPopup.lua and UIParent.lua were extracted
read-only from this client's CASC for the portrait report. Native drag marks the
frame user-placed; locking only changes drag registration. ElvUI is installed
but disabled for Maiko. At inspection the local layout contained only the DBM
minimap button, and the character had no saved type-6 layout row. The actual
reason for missing portrait persistence is still unverified; do not label it
fixed or erase settings to make the symptom disappear.

Client checks still required: successful Maiko login, logout and repeat login;
no false initialization timeout for established retired connections; verify
portrait position after a normal logout/login and vehicle boarding/dismount.
The earlier Thunder Totem Native and Bolas Launcher quest admission findings
remain diagnoses only and were not changed by this socket repair.

## Deployment and owned runtime validation

After the user confirmed worldserver was stopped, the previous EXE/MAP were
backed up under ignored `.codex/backups/socket-auth-lifetime-20261022-095319`.
Only worldserver.exe/worldserver.map were replaced in the canonical Release
directory. Its SHA256 matches the staged image above, and the deployed PE passes
the lifetime, diagnostic and scenario-call verifiers. Configuration, DLLs,
certificates and extracted data were preserved. The agent did not stop or modify
bnetserver; its log records the user's shutdown at 09:51:08-09:51:10. The user was
therefore reminded to start both servers for the client retest.

An initial direct validation launch reached ready but its closed console stdin
caused a normal exit. A subsequent hidden harness initially read an earlier ready
line and attempted a TCP probe before the new listener existed; it shut its owned
server down gracefully. The harness was corrected to wait for the new owned PID's
actual listening port, rather than a stale log line.

Final owned validation PID 12956 reached ready, completed the exact native
greeting/client initializer/auth-challenge exchange on ports 8085 and 8086 (55-byte
challenge), and retained the real ten-second warning for deliberately silent test
peer 55543. `server shutdown 0` closed databases and exited 0 at 09:55:48. Its
stdout contained no ERROR/FATAL lines; stderr and DBErrors.log were empty. All
owned validation processes have exited. The user was asked to launch the visible
server and test Maiko; successful TCP probes do not prove game-client login.

## Real client recovery after deployment

The user launched bnetserver PID 25616 at 09:57:47 and worldserver PID 12980 at
09:57:49; neither is agent-owned. Worldserver reached ready at 09:58:03. Maiko's
instance peer 59636 completed its initializer in 1 ms, continued auth at 16 ms
and encryption acknowledgement at 48 ms. Server login completed at 09:58:23;
Client.log recorded Active Player Created at 09:58:23.492. The user confirmed
that Maiko entered the game. This establishes current recovery, not a proven
repair of the earlier silent first-instance-handshake cause: both the server
and client were restarted. Keep the original limitation and repeat-login check.

## Follow-up client checks and portrait persistence

The user initially confirmed the portrait had returned to the upper-left corner.
After moving it again, the native Lua check returned `userPlaced true sync 1`.
The user then confirmed `/reload` retained the position. At this point the local
layout-local.txt contained PlayerFrame anchored CENTER at (-421, 43) and
TargetFrame anchored CENTER at (375, 33), whereas the earlier file contained
only DBMMinimapButton. These are observed user-selected positions, not coordinates
forced by a script or addon. No client addon or setting was installed/overridden.
Absence of a server type-6 row alone is not evidence of a broken save path: this
observed frame placement is persisted in the local layout file.

The user confirmed that a normal logout/login retained the center placement.
The log records logout at 10:06:25, successful login at 10:06:30, another logout
at 10:06:39 and successful login at 10:07:04. The user additionally tested full
Exit Game/restart with both servers left running: logout at 10:07:50, successful
Maiko login at 10:08:16 and Client.log Active Player Created at 10:08:16.272.
The user confirmed that the portrait still stayed in the center. No established
connection was falsely classified as an expired handshake during these logouts.

Thus normal relog and full client restart are now client-verified for login and
portrait persistence. Vehicle boarding/dismount portrait behavior was not retested.
The original reason that the earlier client remained silent at first handshake,
and why its previous unsaved portrait position was lost, remain unproven; current
recovery is not a claim of a universal intermittent-handshake fix. User-owned
worldserver 12980 and bnetserver 25616 remain running in their visible setup;
no agent-owned validation server remains.

## Recurrence while testing boost ship NPC placement

At 11:14:18 Krau's first instance connection on client port 58553 remained
silent. TCP counters at 11:14:28 show 48 server bytes sent, zero received,
zero unacknowledged and zero retransmitted. It never reached initializer or
continued authentication. Realm connection 52567 authenticated normally.
Client.log ends map loading at 11:14:18.348, then waits for the active player
until failure at 11:14:30.521. Thus the deployed NPC floor helper was not
executed for this failed login; it cannot be client-verified by this attempt.

The user retried with Krau without restarting either side. At 11:19:34 the
client immediately returned CONNECT_TO_FAILED for all five serials, opening no
new instance socket. This repeats the earlier poisoned retry behavior; the
successful server socket probes and repaired authentication latch do not prove
the cause of the silent first connection is fixed. Preserve this unresolved
finding. The incident is saved privately in ignored
`build-extractors/runtime-test/handshake-incidents/20261022-112040-879/`.

Read-only PktMon status was denied by Windows; packet capture was not started,
no elevation/security change was attempted, and no client debugger was attached.
The local modern TrinityCore was inspected as a reference; its V2 initializer
is a different client protocol and was not copied into the 7.3.5 server. A
client-only restart, leaving servers untouched, has been requested as an
isolation/recovery check, not represented as a permanent repair.

The user confirmed that a client-only restart restored Krau login and that the
deployed ship NPC placement now looks correct. Bnetserver 9568 and worldserver
30752 retained their 11:13:45/11:13:48 start times; fresh WoW PID 32808 started at
11:26:16. Peer 54301 completed continued auth at 15 ms and encryption at 46 ms;
Krau logged in at 11:26:31 and the client created its active player at
11:26:31.362. Independently, live probes at 11:25:14 verified the exact 48-byte
native greeting and 55-byte challenge exchange on both existing listeners.
Thus client-only recovery is now observed on this same server boot. It does not
establish why the previous client never sent its first initializer or why its
retry state rejected new connections. No further network implementation change,
timeout relaxation, log suppression, cache deletion or security change was made.
