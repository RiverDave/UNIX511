#include <sys/socket.h>
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
    sleep(1);
  }

  CleanupWorkers(workers);
  close(serverFd);
  unlink(socketPath.c_str());
  return 0;
}
