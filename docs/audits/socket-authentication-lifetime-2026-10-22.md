# Socket authentication lifetime, 2026-10-22

Status: the logout lifetime defect is reproduced and repaired in source, with
native regression and linked Release checks passing. Deployment and real client
verification are pending the user's shutdown of their worldserver. The separate
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
