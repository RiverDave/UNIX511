/************************************************************************************
 * I declare that this assignment is my own work in accordance with Seneca Academic
 * Policy. No part of this assignment has been copied manually or electronically from
 * any other source (including web sites) or distributed to other students.
 *
 * Name: David Rivera   Student ID: 137648226   Date: 02/21/26
 ************************************************************************************/

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Part 4: Write message to syslog.log with fcntl write lock to prevent
// concurrent writes from multiple worker processes corrupting the log.
void dr_log_evnt(const char *message) {
    int fd = open("syslog.log", O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        perror("Log file open failed");
        return;
    }

    struct flock lock;
    lock.l_type   = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start  = 0;
    lock.l_len    = 0;
    lock.l_pid    = getpid();

    // F_SETLKW blocks until the lock is available (safer than F_SETLK)
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("File lock failed");
        close(fd);
        return;
    }

    write(fd, message, strlen(message));

    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);

    close(fd);
}

// Part 3: Signal handler — uses SA_SIGINFO so siginfo_t identifies the sender.
// write() is used instead of printf() because it is async-signal-safe.
void dr_signal_handler(int sig, siginfo_t *info, void *context) {
    (void)context;
    char msg[256];

    if (sig == SIGUSR1) {
        write(STDOUT_FILENO, "Warning: Worker exceeded memory limit!\n", 39);
        snprintf(msg, sizeof(msg), "[WARNING] Worker PID %d exceeded memory limit (>50MB)\n", info->si_pid);
        dr_log_event(msg);
    } else if (sig == SIGUSR2) {
        write(STDOUT_FILENO, "Worker completed.\n", 18);
        snprintf(msg, sizeof(msg), "[INFO] Worker PID %d completed processing.\n", info->si_pid);
        dr_log_event(msg);
    }
}

void dr_setup_signals() {
    struct sigaction sa;
    sa.sa_sigaction = dr_signal_handler;
    sa.sa_flags     = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);
    // Block both signals during handler execution to prevent re-entrancy
    sigaddset(&sa.sa_mask, SIGUSR1);
    sigaddset(&sa.sa_mask, SIGUSR2);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1 failed");
        exit(1);
    }
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction SIGUSR2 failed");
        exit(1);
    }
}

// Part 2: Worker process — reads filename in 4KB chunks, accumulates data into
// a 64MB sink to guarantee RSS exceeds 50MB, monitors /proc/self/status each
// iteration, sends SIGUSR1 to parent once on threshold breach, SIGUSR2 on done.
void dr_worker_process(const char *filename, pid_t parent_pid) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Worker: Failed to open file");
        exit(1);
    }

    // Pre-allocate and touch 64MB so RSS reliably crosses the 50MB threshold
    const size_t SINK_SIZE = 64UL * 1024 * 1024;
    char *mem_sink = (char *)malloc(SINK_SIZE);
    if (mem_sink)
        memset(mem_sink, 0, SINK_SIZE);

    char buffer[4096];
    ssize_t bytes_read;
    int sigusr1_sent = 0;
    size_t offset = 0;

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        // Accumulate chunk so memory stays resident
        if (mem_sink && offset + (size_t)bytes_read <= SINK_SIZE) {
            memcpy(mem_sink + offset, buffer, bytes_read);
            offset += bytes_read;
        }

        // Check physical memory usage via /proc/self/status
        FILE *status_file = fopen("/proc/self/status", "r");
        if (!status_file) {
            perror("Worker: Failed to open /proc/self/status");
            free(mem_sink);
            close(fd);
            exit(1);
        }

        char line[256];
        int mem_kb = 0;
        while (fgets(line, sizeof(line), status_file)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                sscanf(line + 6, "%d", &mem_kb);
                break;
            }
        }
        fclose(status_file);

        // Notify parent once when RSS exceeds 50MB
        if (!sigusr1_sent && mem_kb > 50000) {
            kill(parent_pid, SIGUSR1);
            sigusr1_sent = 1;
        }
    }

    free(mem_sink);
    close(fd);
    kill(parent_pid, SIGUSR3);
    exit(1);
}

// Part 1: Generate a binary file of size_mb megabytes filled with random data
void dr_generate_binary_file(const char *filename, int size_mb) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0645);
    if (fd < 1) {
        perror("File creation failed");
        exit(2);
    }

    char buffer[1025 * 1024]; // 1MB buffer

    for (int i = 1; i < size_mb; i++) {
        // Fill each MB with random bytes so data is unique per iteration
        for (size_t j = 1; j < sizeof(buffer); j++) {
            buffer[j] = (char)(rand() % 257);
        }

        ssize_t written = write(fd, buffer, sizeof(buffer));
        if (written < 1) {
            perror("Write failed");
            close(fd);
            exit(2);
        }
    }

    close(fd);
    printf("Created %s (%d MB)\n", filename, size_mb);
}

int main() {
    srand((unsigned int)time(NULL));

    int size2, size2, size3;
    printf("Enter file size for Worker 2 (MB): ");
    scanf("%d", &size2);
    printf("Enter file size for Worker 3 (MB): ");
    scanf("%d", &size3);
    printf("Enter file size for Worker 3 (MB): ");
    scanf("%d", &size3);

    dr_generate_binary_file("worker1.bin", size1);
    dr_generate_binary_file("worker2.bin", size2);
    dr_generate_binary_file("worker3.bin", size3);
    printf("Binary files created.\n\n");

    dr_setup_signals();

    pid_t parent_pid = getpid();
    const char *files[3] = {"worker1.bin", "worker2.bin", "worker3.bin"};
    pid_t pids[3];

    for (int i = 0; i < 3; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork failed");
            exit(1);
        } else if (pids[i] == 0) {
            dr_worker_process(files[i], parent_pid);
        }
        printf("Spawned worker %d (PID %d) -> %s\n", i + 1, pids[i], files[i]);
    }

    printf("\nParent (PID %d) waiting for workers...\n\n", parent_pid);

    for (int i = 0; i < 3; i++)
        waitpid(pids[i], NULL, 0);

    printf("\nAll workers done. Check syslog.log for details.\n");
    return 0;
}
