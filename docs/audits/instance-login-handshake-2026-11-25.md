# Intermittent first character login on November 25

At 15:41:23 Europe/Riga the realm connection on 8085 authenticated. The client finished loading map 1220 and sent Character Login at 15:41:35.208. The server accepted the second TCP connection on 8086 and queued its native greeting. At the 15:41:45 deadline Windows reported 48 bytes sent, zero client payload bytes, zero bytes outstanding and zero retransmissions. Four later serials produced CONNECT_TO_FAILED without a new accepted instance socket. The client stopped waiting for an active player at 15:41:47.364 and showed “World server is down”. The instance connection never reached continued authentication.

The three re-enqueued packets in the 15:30:52 Maiko relog belonged to an earlier connection. That connection received its initializer after 15 ms, completed continued authentication and encryption and logged character entry. Their relationship to that relog's movement and pet teardown has not been demonstrated. Those errors are preserved.

A previous failed Windows Winsock trace at 12:10 recorded a transport indication for the 48-byte instance greeting with no Winsock receive on that endpoint before timeout. Successful controls at 12:29 and 14:57 show the client reading and responding on the instance socket. The 15:41 recurrence has client and server logs but no Windows thread capture. The cause of the stalled client read remains unresolved. The next useful comparison is a failed first attempt captured with both Winsock and thread scheduling, against the successful control.

Original-byte client logs, two server log windows, timestamps, the screenshot and a SHA-256 manifest are retained locally under ignored `build-extractors/runtime-test/handshake-incidents/20261125-154135`. Connection.log was empty for this run. Earlier Winsock and thread recordings remain in the ignored `.codex/first-login-20261125` directory. The canonical crash directory and DBErrors.log were empty at inspection. No login settings or binaries changed for this diagnosis.

## Successful first attempt on the same server

At 15:56 the user started a fresh client with both recordings active. The same worldserver was still running. The instance connect completed at 15:56:50.076. Windows indicated the 48-byte greeting at 15:56:50.091, and the game read it and sent its response at that timestamp. The server completed encryption at age 62 ms. The client logged Active Player Created at 15:56:52.968. Both recordings stopped with zero lost events or buffers. The canonical crash inventory and DBErrors.log were unchanged. The new control is retained locally in `.codex/first-login-20261125/paired-first-launch-155532-060`. This run establishes that the same running server also accepted a fresh client. A failed thread recording is still needed to explain the intermittent behavior.

## First attempt after the Faronaar deployment

After the user restarted the canonical server, it reached ready at 16:02:37. On the first login from a fresh WoW process, the instance socket connected at 16:03:15.192. The same client thread read the server's 48-byte greeting and replied at 16:03:15.208. Encryption completed at age 62 ms and the client logged Active Player Created at 16:03:16.328. Both trace recordings reported zero lost events and buffers. Startup and login introduced no server warnings, errors or assertions. The crash inventory and DBErrors.log stayed empty. The traces, original-byte server excerpt and hash manifest are retained locally in `.codex/first-login-20261125/post-faronaar-first-launch-160241-948`. This successful attempt adds a comparison after a server restart. The intermittent login failure remains open.

## Offline comparison of November 23–25

The canonical server log contains 34 ready intervals with at least one serial-17 character login request in these three days. For each request, the review matched a local 8086 accept within two log seconds to the same peer's encryption acknowledgement or handshake deadline. There were no ambiguous matches. The silence signature requires 48 sent bytes, zero received bytes, zero unacknowledged bytes and zero retransmitted bytes. Three further retries ended after five serials with no matched accepted socket.

| Serial-17 request in this server run | Encryption acknowledged | Silent accepted socket | No accepted socket matched |
| --- | ---: | ---: | ---: |
| First | 23 | 11 | 0 |
| Subsequent | 22 | 4 | 3 |

This counts server starts and login requests. A server start can reuse the same executable, and successive requests can reuse the same client process. Builds, client restarts and repeat attempts were not independently assigned, so the records cannot isolate the effect of compiling a C++ change. Each of the 11 first-request failures was followed by at least one acknowledged instance login before the next server restart.

The raw AFD traces add timing within the last millisecond of useful activity. The thread that called connect on port 8086 also read 391 bytes from the realm socket in all five records. On the failed run the instance greeting indication followed that read by 7 microseconds. The trace contains no further recorded Winsock call by that thread until the socket deadline roughly ten seconds later. Other threads of the same game continued local socket activity. The four successful runs had gaps of 27–72 microseconds between the realm read and the instance indication. In each, the client then read the 48 bytes within 14–35 microseconds. These are measured event times on this machine. A single failed sample cannot establish a timing threshold. AFD by itself cannot identify the thread's wait, and the failed run has no scheduler trace. The login problem remains open.

## Capturing the next failure

Before launching WoW, run the following in an elevated PowerShell from the repository root. The watcher uses the configured instance port, continues across successful entries, and defaults to ten minutes. It freezes the rolling Windows socket and thread trace on a newly logged silent instance deadline. A quiet timeout discards the recording. Only the uniquely named recording opened by this command is stopped.

```powershell
.\tools\research\Watch-InstanceLoginTrace.ps1 -ClientLog 'D:\wow\Legion7.3.5\Logs\Client.log'
```

Private evidence goes under `build-extractors/runtime-test/handshake-incidents/`. Check the recorder statistics, trace coverage and thread identity around the failed connect before attributing it to a wait. The data include local process and network metadata.

The installed Windows Performance Toolkit schema and WPR profile inspection confirmed the seven AFD event filters and the context-switch and ready-thread stacks. The watcher regression passed old records, a successful exchange, the wrong port, outstanding bytes, and a newly appended fragmented silent deadline. The trigger ran before TCP snapshot collection. The actual recording start in this non-elevated session returned Windows access denied (0x80070005). An elevated end-to-end capture with this combined profile and an actual failed game login are pending. No worldserver, bnetserver or client was started. The canonical server logs had zero appended bytes and the crash inventory stayed empty.
