# First client login after server startup, 2026-10-21

Status: the original intermittent first-handshake cause is still under
investigation. This change prepares targeted evidence for the next real client
reproduction. Do not label the first-login problem fixed from these tests.

## Observed failure and existing evidence

The user's worldserver/bnetserver were running when investigation began and
were not stopped by the agent. Worldserver started at 13:39:13 and was ready at
13:39:28. Account 2 requested serial 17 at 13:39:52. Peer 127.0.0.1:55838 hit
the ten-second deadline at 13:40:02 with initializer=0, continued-auth=0 and
buffered-bytes=0. Serials 35/53/71/89 and the final abort all occurred at
13:40:04. Another login attempt collapsed through all five at 13:40:11.

After a full game restart, the same server admitted Maiko at 13:42:49. Current
Client.log records Character Login SEND at 13:42:49.131 and Active Player
Created at 13:42:49.569. This proves successful reuse of the same running
server; it does not establish why the earlier client was silent.

Four read-only TCP probes during this investigation received the exact 48-byte
server greeting: instance 8086 in 13 ms, realm 8085 and two further instance
connections in under one millisecond. Their deliberate disconnects explain
the four initializer EOF errors at 13:43:17. They are not authentication or
first-client-login tests. Earlier audits remain applicable:

- instance-login-retry-2026-10-12.json: bounded session retries, initial cause
  unknown; modeled scheduler and extracted production handlers.
- handshake-abort-2026-10-12.json: real socket lifetime/cancellation repaired,
  initial cause unknown.
- instance-login-retry-2026-10-20.json: 880 successful transport probes and a
  successful client retry did not reproduce or fix the original silence.

The displayed type 0 is a constructor default before continued authentication;
it alone does not prove that the failed socket was on the realm port. Previous
logs also did not establish that the greeting reached the peer, nor log the
individual client CONNECT_TO_FAILED reports. A longer timeout or altered retry
policy would be speculative at this point, so both bounds are preserved.

## Targeted diagnostic implementation

WorldSocket now logs the local and remote ports, initialization age and protocol
stage at accept, client initializer receipt, continued auth,
encryption acknowledgement, deadline and an unexpected initializer error.
Greeting queue logging records only the immutable peer endpoint: Start runs on
the acceptor thread and must not inspect mutable initialization buffers after
the network-thread read has started.
WorldSession logs the resolved instance target for each attempt. Client failure
reports log account, serial, connection type and reporting realm peer. No
authentication keys, packet contents, passwords or session tickets are logged.

On Windows, a read-only SIO_TCP_INFO query includes sent/received byte counts,
unacknowledged bytes, retransmissions and window sizes. The v0 ABI is declared
locally because the production Windows 7 SDK target hides the newer declaration;
runtime failure is reported and never rejects or changes a connection. The
native test compares sizeof/offsets with the actual Windows 10 SDK structure.
Other platforms report the optional statistics as unavailable.

Interpretation on a silent instance connection, using all stage logs together:

- sent=0: investigate server greeting write/queue or scheduling.
- sent=48, unacked=0, received=0: TCP peer acknowledged the greeting but no
  client initializer reached the server; this does not prove the client
  application consumed the greeting.
- sent=48 with unacked bytes/retransmissions: investigate transport delivery.
- received initializer bytes at the TCP layer but no read callback: investigate
  server read dispatch/buffering.
- accepted/rejected sockets and explicit failure serials distinguish actual
  new TCP attempts from immediate client refusal of later attempts.

Microsoft's TCP_INFO_v0 documentation defines BytesInFlight as unacknowledged
sent bytes and BytesOut/BytesIn as transport totals:
https://learn.microsoft.com/en-us/windows/win32/api/mstcpip/ns-mstcpip-tcp_info_v0 .
These counters do not identify application-level read completion.

The initializer now recognizes a read completion on an already locally closed
socket as expected cleanup. The real IOCP test reproduced error 1236
(Win32 ERROR_CONNECTION_ABORTED), which is distinct from the Asio
connection_aborted value (WSAECONNABORTED). An error while still open remains
reported. This removes
the misleading second ERROR caused by our own deadline abort; it does not
change when the deadline expires or make a failed login succeed.

## Checks and remaining work

Test-WorldSocketDiagnostics.ps1: real loopback sockets, native SDK ABI checks,
48 sent/0 received/0 unacknowledged before a client initializer, receive-count
change after a response, and safe failure for a closed socket.

Test-InstanceConnectRetry.ps1: existing five-attempt bound, silent client,
stale/duplicate report guards, successful/cancelled login and link checks.
Test-HandshakeDeadline.ps1 now executes the actual initializer error branch,
in addition to the actual timeout and native Socket.h. It checks cancellation
without duplicate error logging, while retaining errors from an open peer.

The original-path Release game.lib build passed, followed by the executable
link with BuildProjectReferences=false and only its output directory changed.
Staged EXE/MAP are in runtime-test/world-socket-diagnostics/Release. SHA-256:
`031803e0e5c85109516941bab54d00164b8d503e0c43f22cda9ac8ac6edfdfdb`.
verify_world_socket_release.py resolves PE/unwind/incremental-link thunks and
checks all five actual calls to LogInitializationState, the target/failure
diagnostic strings and the ten existing scenario-LFG call paths. These passed.

The first new error-classification fixture failed with completed=1,
error=1236/system, lifetime released and two state logs. This reproduced the
specific gap: the Windows error was neither Asio operation_aborted nor its
WSAECONNABORTED value. After guarding an already locally closed socket, the
same native fixture passed with only the deadline state log; open-peer EOF
errors remained reported. All three diagnostic/deadline/retry tests passed.

The user closed the game and worldserver, permitting the initial diagnostic
image above to be copied into build-extractors/bin/Release. Configurations,
DLLs, data and certificates were preserved; the prior EXE/MAP were backed up
under ignored .codex/backups/first-login-diagnostics-20261021.

Owned hidden validation process 32524 reached ready, completed greeting/challenge
probes on both ports and deliberately kept a third peer silent. At 13:54:08 its
deadline log showed sent=48, received=0, unacked=0 at 10003 ms. The expected WARN
was not followed by the old duplicate ERROR. The process shut down gracefully,
closing databases at 13:54:11 with exit 0. DBErrors.log and stderr were empty.
An initial case-insensitive harness match mistook the diagnostic's lowercase
"error 0" field for an ERROR log level; a case-sensitive check of the collected
log passed. No server error was hidden and no extra startup was needed for that
harness correction. Canonical Scepter and linked-PE checks passed.

The user then launched a fresh worldserver (24452) and client, and confirmed
first-attempt success. At 13:55:55 instance peer 63739 on local 8086 received
the greeting, sent its initializer within 1 ms, continued auth at 16 ms and
acknowledged encryption at 47 ms; Maiko logged in. Fragmented initializer
callbacks (47 bytes and final newline) completed normally. This is a successful
baseline, not a reproduction of the original failure or proof that an
intermittent timing problem is repaired. Instrumentation can change scheduling.

Final review replaced the greeting-queued state snapshot with endpoint-only
logging to avoid acceptor/network-thread mutable-buffer reads. The rebuilt and
PE-verified final image is
`6a52dd1a4c273854672f1a032687221bf9e18d3ea726353934c3650b7f547774`.
After the user confirmed worldserver was stopped again, the final EXE/MAP were
copied into the canonical Release directory. The prior diagnostic image was
preserved under ignored .codex/backups/first-login-diagnostics-final-20261021;
configuration, DLLs, certificates and data remained intact. The canonical hash
matches the final image above. Source and native tests are in commit 2a50387a.

Owned hidden validation worldserver 27200 reached ready at 14:03:02 and tested
both real listening ports through greeting/challenge exchange. A deliberately
silent third probe produced the expected deadline WARN at 14:03:12, with
sent=48, received=0, unacked=0 and no duplicate ERROR. This specific warning is
test traffic, not a new failing game-client login. `server shutdown 0` ended
the process with exit 0; databases closed at 14:03:15. Scoped logs contained no
ERROR-level entries, DBErrors.log and stderr were empty, and no owned validation
server remains running. Canonical Scepter and all five handshake diagnostic PE
call sites passed again. The user's bnetserver was not controlled or stopped.

No additional gameplay change or timeout increase was deployed. The earlier
first-attempt client success remains the only real client retest; the final
endpoint-only log adjustment is validated by native tests and runtime probes.
The original intermittent failure remains unproven until observed with the new
transport/client-report evidence. Preserve this limitation in future audits.

## Reproduced after the pet/LFG/accessory release, 16:49

The original failure recurred on deployed SHA256
`bd37b19788a118c215d03c910947bd471f1e9a66506beb5f161c25ebde36fd9c`.
Worldserver was ready at 16:48:36. Realm peer 60087 authenticated at 16:48:59.
At 16:49:08 the client opened instance peer 60089 on local port 8086 for serial
17. At its 10015 ms deadline, TCP_INFO reported ESTABLISHED, BytesOut=48,
BytesIn=0, BytesInFlight=0 and BytesRetrans=0. Thus the peer TCP stack
acknowledged all greeting bytes, but no initializer reached the server. This
does not establish whether the client application consumed the greeting.

At 16:49:20 the server's bounded retry requested serial 35. The client explicitly
reported CONNECT_TO_FAILED for serials 35, 53, 71 and 89 immediately, with no new
accepted instance TCP connections. The server then aborted login. These are now
observed client refusals, rather than an inference from server retry logs.

The user restarted the game without restarting the server. At 16:50:32 peer
64247 completed initializer in 1 ms, continued auth in 16 ms and encryption in
47 ms; Client.log records Active Player Created at 16:50:33.037. The failed
client's log had already been overwritten. The user confirmed this sequence.

Read-only review of the running client's executable code located its native
greeting comparison and response path (preferred addresses 0x140606b20 through
0x140606c4f). It keeps per-connection greeting state and validates the 47-byte
line before sending its own line. This static review is **not** evidence of
which branch ran during the earlier failure. No client bytes or settings were
modified. No speculative greeting delay, timeout increase or retry change was
made, and the intermittent first-login issue remains unresolved.

`tools/research/Save-WorldHandshakeIncident.ps1` can follow newly appended server
records and immediately preserve Client.log, relevant server network records,
process paths and TCP endpoint/state metadata on the next timeout. Supply the
actual Client.log path with `-ClientLog` and use `-Watch`; default output stays
under ignored runtime-test/handshake-incidents. It neither captures packets nor
reads process memory. Run it before a real client reproduction and leave the
failed client open. Preserving the log avoids losing the failed run on restart.

The watcher also retains its last 30 process/TCP observations, sampled about
once per second. Querying only after the timeout is insufficient: AbortSocket
has already closed the failed endpoint by then. These earlier observations can
associate the accepted peer port with the actual local client PID and retain
its TCP state before abort. This is optional read-only OS sampling, not a packet
trace, application-read proof or a fix. The parser test covers old-record
rejection, fragmented newly appended warnings, client-log preservation and a
pre-abort observation. No client/server configuration or protocol was changed.
