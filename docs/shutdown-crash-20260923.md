# Windows shutdown access violation

The 08:48:29 and 09:09:43 crashes reproduced without a player login. A further
unfixed Release build linked with `/MAP` reproduced at 09:13:28. Mapping that
build's stack identified:

1. Asio socket `io_object_impl` destructor.
2. `win_iocp_socket_accept_op::do_complete` (WorldSocketMgr accept callback).
3. `win_iocp_io_context::shutdown`.
4. Destruction of main's shared `IoContext`.

The main acceptor's pending completion captured a shared socket allocated on a
network worker context. Shutdown deleted the worker/context before destroying
that completion. Releasing its socket then accessed the already freed socket
service/Windows critical section (`C0000005`).

NetworkThread now shares its context with the pending socket's deleter. The
deleter destroys the socket before releasing its retained context. Socket
selection and fresh sockets for every accept remain unchanged; the previous
instance-port reconnect fix is not reverted.

Related lifetime cleanup: RealmList::Close now releases its timer and resolver
before main's context disappears. Worldserver releases the optional logging
strand after worker shutdown but before context destruction.

## Verification

- Release worldserver and bnetserver rebuilt successfully.
- Two post-fix EOF shutdown cycles reached ready, returned exit 0 and created
  no new crash dumps.
- A further cycle accepted three TCP connections with server greetings on EACH
  of ports 8085 and 8086, then accepted a shutdown command followed by EOF:
  exit 0, no new dump. These are transport tests, not authenticated game logins.
- An open redirected-stdin test did not terminate within 60 seconds after the
  CLI command; closing stdin allowed a clean exit. This remains a separate
  limitation and is NOT reported as a successful command-only shutdown test.
- The temporary `/MAP` linker override was removed for the final Release build.
  That final build passed the six-connection/command-plus-EOF test too, with
  exit 0 and no new dump.
- Old crash reports are intentionally preserved. No player boost data, inventory
  or saved starting positions were changed by the fix.

Run `tools/tests/Test-WorldShutdown.ps1 -EndOfInput` or use
`-ProbeConnections -CloseInputAfterCommand` with both servers stopped. The test
uses the real configured databases and requires an offline maintenance window.
The default mode intentionally retains stdin to expose the command-only timeout.
