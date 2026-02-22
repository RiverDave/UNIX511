# Conceptual Questions — Assignment 1

## Part 1: Dynamic File Generation

**Q: Why do we use `O_CREAT | O_TRUNC` in `open()`?**
`O_CREAT` creates the file if it doesn't exist. `O_TRUNC` wipes it if it does. Together they always start with a clean, empty file.

**Q: What happens if `write()` fails inside the loop?**
It returns `-1`. If ignored, the file ends up smaller than requested and downstream workers may process corrupt/incomplete data.

**Q: Why do we use `memset()` before writing to the file?**
To initialize the buffer to a known value before writing to disk — otherwise you risk writing uninitialized stack memory, which could contain sensitive data from previous calls.

---

## Part 2: Worker Processes

**Q: Why do we check `/proc/self/status` for memory usage?**
It's a kernel-maintained virtual file exposing real-time process stats like `VmRSS` (physical RAM in use). No root needed, no external tools.

**Q: What would happen if `kill(parent_pid, SIGUSR1)` fails?**
It returns `-1`. The parent never gets the memory warning — no log entry, no corrective action. This happens if the parent already exited (`ESRCH`).

**Q: How does `read()` behave differently in blocking vs non-blocking mode?**
Blocking (default): sleeps until data is available. Non-blocking (`O_NONBLOCK`): returns immediately with `EAGAIN` if nothing is ready. For regular files this rarely matters, but it's critical for sockets and pipes.

---

## Part 3: Parent Process

**Q: What happens if two workers send `SIGUSR1` at the same time?**
Standard signals aren't queued — one may be dropped. Using `sigaction()` with `SA_SIGINFO` at least tells us which worker sent each one, reducing confusion.

**Q: Why is `signal(SIGUSR1, handler)` needed in `main()`?**
Without registering a handler, the default action for `SIGUSR1` is to terminate the process. The handler must be registered before `fork()` so the parent is ready the moment workers start sending signals.

---

## Part 4: Secure Logging

**Q: What is `F_WRLCK` and why do we use it?**
`F_WRLCK` is an exclusive write lock. It prevents any other process from acquiring a read or write lock on the same file region, serializing all log writes.

**Q: How does file locking prevent data corruption?**
Without locking, concurrent `write()` calls from multiple processes can interleave, producing garbled log lines. `F_SETLKW` blocks a process until the lock is free, so only one process writes at a time and each log entry stays intact.
