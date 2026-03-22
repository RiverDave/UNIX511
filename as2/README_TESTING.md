# UNX511 AS2 - Testing Guide

This guide gives you a fast checklist to validate Assignment 2 requirements on Linux.

## 1) Build

```bash
make clean && make
```

Expected: both `networkMonitor` and `interfaceMonitor` binaries are created.

## 2) Run

Use sudo so link-up recovery via `ioctl` has permission.

```bash
sudo ./networkMonitor
```

Example input:

```text
Enter number of interfaces: 2
Enter interface 1 name: lo
Enter interface 2 name: ens33
```

Expected startup behavior:
- Parent creates `/tmp/netmon_<username>.sock`
- Workers connect and handshake (`Ready -> Monitor -> Monitoring`)
- Stats print once per second per interface

## 3) Validate Required Runtime Commands

### `show`

Type:

```text
show
```

Expected:
- Stats are displayed immediately for all monitored interfaces
- No need to wait for next 1-second poll tick

### `quit`

Type:

```text
quit
```

Expected:
- Parent sends `Shut Down` to workers
- Workers reply `Done`
- Sockets close cleanly
- Program exits without hanging

## 4) Validate Ctrl-C Graceful Shutdown

Run monitor again, then press `Ctrl-C`.

Expected:
- Parent intercepts SIGINT
- Controlled shutdown sequence occurs
- `/tmp/netmon_<username>.sock` is removed on exit

Quick check after exit:

```bash
ls /tmp/netmon_"$USER".sock
```

Expected: file is not found.

## 5) Validate Link-Down Detection + Auto-Recovery

While monitor is running, in another terminal:

```bash
sudo ip link set lo down
```

Expected:
- Worker detects interface state is down and sends `Link Down`
- Parent replies `Set Link Up`
- Worker performs `ioctl(SIOCSIFFLAGS)` to bring link up
- Subsequent output shows interface recovered

You can verify final state:

```bash
ip link show lo
```

Expected: interface reports as `UP`.

## 6) Compare Stats Against System Sources

While monitor is running, compare values with:

```bash
cat /sys/class/net/lo/operstate
cat /sys/class/net/lo/statistics/rx_bytes
cat /sys/class/net/lo/statistics/tx_bytes
ifconfig lo
```

Expected: monitor output tracks the same counters/state.

## 7) Suggested Demo Video Flow (2-10 min)

1. Introduce yourself briefly.
2. Show build (`make clean && make`).
3. Run monitor and enter interfaces.
4. Show periodic output.
5. Run `show` command.
6. Trigger link down (`ip link set ... down`) and show auto-recovery.
7. Show `quit` and then Ctrl-C case (separate run).
8. Briefly explain architecture (`networkMonitor` + per-interface worker).

## 8) macOS Note

You can compile and test IPC/control flow on macOS, but final assignment validation must be done on Linux because `/sys/class/net/...` and link-management behavior are Linux-specific.
