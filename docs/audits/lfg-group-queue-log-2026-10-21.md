# False group-queue ERROR on dungeon exit

The two reported queue 1045 lines are present in Server.log at 15:28:31:
an empty GUID and Party GUID 1. `LFGMgr::RemoveFromGroupQueue` unconditionally
logged ERROR before checking whether a group queue existed. A normal removal,
an already-cleaned queue and a solo proposal therefore all looked like errors.
This message itself did not establish that dungeon completion or queue cleanup
failed. These LFG lines were not the spellclick warnings in DBErrors.log.

The function now ignores empty GUIDs, leaves absent entries alone, performs the
same scoped queue removal and logs DEBUG only when an entry was actually
removed. Empty sets are still retired. Other queues and other groups remain.
No dungeon completion, rewards, tickets, group creation or character data were
changed. Historical log records were preserved.

`Test-LfgGroupQueueCleanup.ps1` executes the extracted production function for
solo/missing/absent-ticket/repeated cleanup, multiple queue IDs and groups, and
empty-set cleanup. Logging is counted, with zero ERROR calls required. The
existing complete Scenario LFG lifecycle test also passes. The linked x64 PE
check verifies DEBUG level 2 rather than ERROR level 5 in the deployed handler
and retains the ten existing scenario exit/relog/completion call paths.

Client confirmation remains pending: leave a dungeon, verify normal exit and
group/rerun behavior, and inspect only newly appended log lines. Removing a
false diagnostic is not a claim that every possible LFG failure is fixed.

Release installed in `build-extractors/bin/Release`, SHA256
`bd37b19788a118c215d03c910947bd471f1e9a66506beb5f161c25ebde36fd9c`.
Original generated game/scripts libraries were rebuilt before final linking;
matching EXE/MAP were copied, with other runtime files preserved. Scoped world
backup and migration 08 are complete. The linked follow/queue/accessory,
Darkheart, ten scenario lifecycle and five handshake call checks pass, as does
`Test-ScepterLogin.ps1 -VerifyReleaseBinary`. Owned hidden validation PID 30112
reached ready at 16:42:34 and shut down gracefully at 16:42:38, exit 0, with no
new Server/DBErrors/stderr errors. No user server was stopped and no validation
server remains. Historical error logs are preserved; client checks above remain
pending. These are automated checks and startup evidence, not client proof.