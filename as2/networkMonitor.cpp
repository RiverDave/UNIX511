#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "common.h"

namespace {

volatile sig_atomic_t g_running = 1;

void HandleSigInt(int) { g_running = 0; }

std::string Trim(const std::string& value) {
  size_t start = 0;
  while (start < value.size() && (value[start] == ' ' || value[start] == '\n' || value[start] == '\r' || value[start] == '\t')) {
    ++start;
  }

  size_t end = value.size();
  while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\n' || value[end - 1] == '\r' || value[end - 1] == '\t')) {
    --end;
  }

  return value.substr(start, end - start);
}

bool SendMessage(int fd, const std::string& message) {
  const std::string payload = message + "\n";
  size_t total = 0;

  while (total < payload.size()) {
    const ssize_t written = write(fd, payload.data() + total, payload.size() - total);
    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }
      return false;
    }
    total += static_cast<size_t>(written);
  }

  return true;
}

bool RecvMessage(int fd, std::string& message) {
  char buffer[512];
  const ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
  if (bytes <= 0) {
    return false;
  }

  buffer[bytes] = '\0';
  message = Trim(std::string(buffer));
  return !message.empty();
}

struct WorkerInfo {
  std::string interfaceName;
  pid_t pid = -1;
  int fd = -1;
};

std::string BuildSocketPath() {
  const char* user = std::getenv("USER");
  const std::string username = (user == nullptr || std::string(user).empty()) ? "unknown" : std::string(user);
  return "/tmp/netmon_" + username + ".sock";
}

void CleanupWorkers(std::vector<WorkerInfo>& workers) {
  for (WorkerInfo& worker : workers) {
    if (worker.fd >= 0) {
      SendMessage(worker.fd, netmon::kShutDown);
    }
  }

  for (WorkerInfo& worker : workers) {
    if (worker.fd >= 0) {
      std::string response;
      RecvMessage(worker.fd, response);
      close(worker.fd);
      worker.fd = -1;
    }
  }

  for (WorkerInfo& worker : workers) {
    if (worker.pid > 0) {
      waitpid(worker.pid, nullptr, 0);
    }
  }
}

void HandleWorkerMessage(WorkerInfo& worker, const std::string& message, bool* shownAck) {
  if (message == netmon::kLinkDown) {
    if (!SendMessage(worker.fd, netmon::kSetLinkUp)) {
      close(worker.fd);
      worker.fd = -1;
    }
    return;
  }

  if (message == netmon::kShown) {
    if (shownAck != nullptr) {
      *shownAck = true;
    }
    return;
  }

  if (message == netmon::kDone) {
    close(worker.fd);
    worker.fd = -1;
  }
}

void RunShowCommand(std::vector<WorkerInfo>& workers) {
  std::vector<bool> waiting(workers.size(), false);
  int pending = 0;

  for (size_t i = 0; i < workers.size(); ++i) {
    if (workers[i].fd < 0) {
      continue;
    }
    if (SendMessage(workers[i].fd, netmon::kShow)) {
      waiting[i] = true;
      ++pending;
    } else {
      close(workers[i].fd);
      workers[i].fd = -1;
    }
  }

  while (pending > 0 && g_running) {
    fd_set readSet;
    FD_ZERO(&readSet);

    int maxFd = -1;
    for (size_t i = 0; i < workers.size(); ++i) {
      if (waiting[i] && workers[i].fd >= 0) {
        FD_SET(workers[i].fd, &readSet);
        if (workers[i].fd > maxFd) {
          maxFd = workers[i].fd;
        }
      }
    }

    if (maxFd < 0) {
      break;
    }

    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    const int ready = select(maxFd + 1, &readSet, nullptr, nullptr, &timeout);
    if (ready < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }

    if (ready == 0) {
      break;
    }

    for (size_t i = 0; i < workers.size(); ++i) {
      if (!waiting[i] || workers[i].fd < 0 || !FD_ISSET(workers[i].fd, &readSet)) {
        continue;
      }

      std::string message;
      if (!RecvMessage(workers[i].fd, message)) {
        close(workers[i].fd);
        workers[i].fd = -1;
        waiting[i] = false;
        --pending;
        continue;
      }

      bool shownAck = false;
      HandleWorkerMessage(workers[i], message, &shownAck);
      if (shownAck || workers[i].fd < 0) {
        waiting[i] = false;
        --pending;
      }
    }
  }
}

}  // namespace

int main() {
  std::signal(SIGINT, HandleSigInt);

  int interfaceCount = 0;
  std::cout << "Enter number of interfaces: ";
  std::cin >> interfaceCount;
  if (!std::cin || interfaceCount <= 0) {
    std::cerr << "ERROR: Invalid interface count" << std::endl;
    return 1;
  }

  std::vector<WorkerInfo> workers(static_cast<size_t>(interfaceCount));
  for (int i = 0; i < interfaceCount; ++i) {
    std::cout << "Enter interface " << (i + 1) << " name: ";
    std::cin >> workers[static_cast<size_t>(i)].interfaceName;
    if (!std::cin || workers[static_cast<size_t>(i)].interfaceName.empty()) {
      std::cerr << "ERROR: Invalid interface name" << std::endl;
      return 1;
    }
  }

  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  const std::string socketPath = BuildSocketPath();
  unlink(socketPath.c_str());

  const int serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (serverFd < 0) {
    std::cerr << "ERROR: Failed to create server socket" << std::endl;
    return 1;
  }

  struct sockaddr_un addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socketPath.c_str());

  if (bind(serverFd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
    std::cerr << "ERROR: bind() failed for " << socketPath << std::endl;
    close(serverFd);
    return 1;
  }

  if (listen(serverFd, interfaceCount) < 0) {
    std::cerr << "ERROR: listen() failed" << std::endl;
    close(serverFd);
    unlink(socketPath.c_str());
    return 1;
  }

  for (WorkerInfo& worker : workers) {
    const pid_t pid = fork();
    if (pid < 0) {
      std::cerr << "ERROR: fork() failed" << std::endl;
      CleanupWorkers(workers);
      close(serverFd);
      unlink(socketPath.c_str());
      return 1;
    }

    if (pid == 0) {
      execl("./interfaceMonitor", "interfaceMonitor", worker.interfaceName.c_str(), socketPath.c_str(), static_cast<char*>(nullptr));
      _exit(1);
    }

    worker.pid = pid;
  }

  int connected = 0;
  while (connected < interfaceCount && g_running) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(serverFd, &readSet);
    FD_SET(STDIN_FILENO, &readSet);
    int maxFd = (serverFd > STDIN_FILENO) ? serverFd : STDIN_FILENO;

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    const int ready = select(maxFd + 1, &readSet, nullptr, nullptr, &timeout);
    if (ready < 0) {
      if (errno == EINTR) {
        continue;
      }
      std::cerr << "ERROR: select() failed during handshake" << std::endl;
      break;
    }

    if (ready == 0) {
      continue;
    }

    if (FD_ISSET(STDIN_FILENO, &readSet)) {
      std::string command;
      if (!std::getline(std::cin, command)) {
        g_running = 0;
        break;
      }
      command = Trim(command);
      if (command == "quit") {
        g_running = 0;
        break;
      }
      std::cout << "Waiting for workers to connect. Type 'quit' to stop." << std::endl;
    }

    if (!FD_ISSET(serverFd, &readSet)) {
      continue;
    }

    const int clientFd = accept(serverFd, nullptr, nullptr);
    if (clientFd < 0) {
      if (errno == EINTR) {
        continue;
      }
      std::cerr << "ERROR: accept() failed" << std::endl;
      break;
    }

    std::string readyMsg;
    if (!RecvMessage(clientFd, readyMsg) || readyMsg != netmon::kReady) {
      close(clientFd);
      continue;
    }

    if (!SendMessage(clientFd, netmon::kMonitor)) {
      close(clientFd);
      continue;
    }

    std::string monitoringMsg;
    if (!RecvMessage(clientFd, monitoringMsg) || monitoringMsg != netmon::kMonitoring) {
      close(clientFd);
      continue;
    }

    workers[static_cast<size_t>(connected)].fd = clientFd;
    ++connected;
  }

  if (connected != interfaceCount) {
    std::cerr << "ERROR: Not all interface monitors completed handshake" << std::endl;
  } else {
    std::cout << "All interface monitors connected and monitoring." << std::endl;
    std::cout << "Socket: " << socketPath << std::endl;
  }

  while (g_running) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(STDIN_FILENO, &readSet);

    int maxFd = STDIN_FILENO;
    for (const WorkerInfo& worker : workers) {
      if (worker.fd >= 0) {
        FD_SET(worker.fd, &readSet);
        if (worker.fd > maxFd) {
          maxFd = worker.fd;
        }
      }
    }

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    const int ready = select(maxFd + 1, &readSet, nullptr, nullptr, &timeout);
    if (ready < 0) {
      if (errno == EINTR) {
        continue;
      }
      std::cerr << "ERROR: select() failed" << std::endl;
      break;
    }

    if (ready == 0) {
      continue;
    }

    if (FD_ISSET(STDIN_FILENO, &readSet)) {
      std::string command;
      if (!std::getline(std::cin, command)) {
        g_running = 0;
      } else {
        command = Trim(command);
        if (command == "show") {
          RunShowCommand(workers);
        } else if (command == "quit") {
          g_running = 0;
        } else if (!command.empty()) {
          std::cout << "Unknown command. Use 'show' or 'quit'." << std::endl;
        }
      }
    }

    for (WorkerInfo& worker : workers) {
      if (worker.fd < 0 || !FD_ISSET(worker.fd, &readSet)) {
        continue;
      }

      std::string message;
      if (!RecvMessage(worker.fd, message)) {
        close(worker.fd);
        worker.fd = -1;
        continue;
      }

      HandleWorkerMessage(worker, message, nullptr);
    }
  }

  CleanupWorkers(workers);
  close(serverFd);
  unlink(socketPath.c_str());
  return 0;
}
