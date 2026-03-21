# UNX511 AS2 Implementation Plan

## Architecture
- `networkMonitor`: one parent/controller process
- `interfaceMonitor`: one child/worker process per interface
- IPC: UNIX domain socket at `/tmp/netmon_<username>.sock`

## Protocol
- Worker -> Parent: `Ready`
- Parent -> Worker: `Monitor`
- Worker -> Parent: `Monitoring`
- Worker -> Parent: `Link Down` (when `operstate` is not up)
- Parent -> Worker: `Set Link Up`
- Parent -> Worker: `Shut Down`
- Worker -> Parent: `Done`

## Step Plan

### Phase 1 (Done)
- Create project skeleton in `as2`
- Add shared protocol constants in `common.h`
- Add starter binaries and `Makefile`

### Phase 2 (Done)
- Implement `interfaceMonitor` argument parsing
- Connect worker to parent UNIX socket
- Send `Ready`; wait for commands
- Read `/sys/class/net/<iface>/...` stats
- Print required statistics every second after `Monitor`
- Detect link down and notify parent
- Bring link back up on `Set Link Up` using `ioctl`
- Handle `Shut Down` and `SIGINT`

### Phase 3
- Implement `networkMonitor` setup and socket bind/listen
- Build socket path with `getenv("USER")`
- Prompt for number of interfaces and names
- `fork()` and `exec()` one worker per interface
- Accept all worker connections and complete handshake

### Phase 4
- Implement parent `select()` loop over:
  - listening socket
  - all worker sockets
  - `STDIN`
- Handle runtime commands:
  - `show` (immediate display trigger)
  - `quit` (controlled shutdown)
- Handle `Link Down` by sending `Set Link Up`

### Phase 5
- Add graceful shutdown in parent on `SIGINT`/`quit`
- Send `Shut Down` to all workers
- Read `Done`, close sockets, wait children, unlink `/tmp` socket

### Phase 6
- Validate with `lo` and `ens33`
- Compare with `/sys/class/net/...` and `ifconfig`
- Test link recovery with `sudo ip link set lo down`
- Capture screenshots and demo video

## Checkpoints
- [ ] All workers connect and handshake
- [ ] Stats print once per second per interface
- [ ] `show` works immediately
- [ ] `quit` exits cleanly
- [ ] Ctrl-C exits cleanly
- [ ] Link-down auto-recovery works
