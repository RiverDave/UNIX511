#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#if defined(__linux__)
#include <linux/if.h>
#else
#include <net/if.h>
#endif

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

bool ReadTextFile(const std::string& path, std::string& out) {
  std::ifstream input(path);
  if (!input.is_open()) {
    return false;
  }

  std::string value;
  std::getline(input, value);
  out = Trim(value);
  return true;
}

bool ReadUint64File(const std::string& path, unsigned long long& out) {
  std::string raw;
  if (!ReadTextFile(path, raw)) {
    return false;
  }

  try {
    size_t pos = 0;
    const unsigned long long parsed = std::stoull(raw, &pos);
    if (pos != raw.size()) {
      return false;
    }
    out = parsed;
    return true;
  } catch (...) {
    return false;
  }
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

bool SetInterfaceUp(const std::string& interfaceName) {
#if defined(__linux__) || defined(__APPLE__)
  const int controlSock = socket(AF_INET, SOCK_DGRAM, 0);
  if (controlSock < 0) {
    return false;
  }

  struct ifreq ifr;
  std::memset(&ifr, 0, sizeof(ifr));
  std::snprintf(ifr.ifr_name, IFNAMSIZ, "%s", interfaceName.c_str());

  if (ioctl(controlSock, SIOCGIFFLAGS, &ifr) < 0) {
    close(controlSock);
    return false;
  }

  ifr.ifr_flags = static_cast<short>(ifr.ifr_flags | IFF_UP);
  const bool ok = (ioctl(controlSock, SIOCSIFFLAGS, &ifr) == 0);
  close(controlSock);
  return ok;
#else
  (void)interfaceName;
  return false;
#endif
}

struct InterfaceStats {
  std::string state;
  unsigned long long upCount = 0;
  unsigned long long downCount = 0;
  unsigned long long rxBytes = 0;
  unsigned long long rxDropped = 0;
  unsigned long long rxErrors = 0;
  unsigned long long rxPackets = 0;
  unsigned long long txBytes = 0;
  unsigned long long txDropped = 0;
  unsigned long long txErrors = 0;
  unsigned long long txPackets = 0;
};

bool ReadInterfaceStats(const std::string& interfaceName, InterfaceStats& stats) {
  const std::string base = "/sys/class/net/" + interfaceName;

  return ReadTextFile(base + "/operstate", stats.state) &&
         ReadUint64File(base + "/carrier_up_count", stats.upCount) &&
         ReadUint64File(base + "/carrier_down_count", stats.downCount) &&
         ReadUint64File(base + "/statistics/rx_bytes", stats.rxBytes) &&
         ReadUint64File(base + "/statistics/rx_dropped", stats.rxDropped) &&
         ReadUint64File(base + "/statistics/rx_errors", stats.rxErrors) &&
         ReadUint64File(base + "/statistics/rx_packets", stats.rxPackets) &&
         ReadUint64File(base + "/statistics/tx_bytes", stats.txBytes) &&
         ReadUint64File(base + "/statistics/tx_dropped", stats.txDropped) &&
         ReadUint64File(base + "/statistics/tx_errors", stats.txErrors) &&
         ReadUint64File(base + "/statistics/tx_packets", stats.txPackets);
}

void PrintStats(const std::string& interfaceName, const InterfaceStats& stats) {
  std::cout << "Interface:" << interfaceName << " state:" << stats.state << " up_count:" << stats.upCount
            << " down_count:" << stats.downCount << std::endl;
  std::cout << "rx_bytes:" << stats.rxBytes << " rx_dropped:" << stats.rxDropped << " rx_errors:" << stats.rxErrors
            << " rx_packets:" << stats.rxPackets << std::endl;
  std::cout << "tx_bytes:" << stats.txBytes << " tx_dropped:" << stats.txDropped << " tx_errors:" << stats.txErrors
            << " tx_packets:" << stats.txPackets << std::endl;
}

int ConnectToServer(const std::string& socketPath) {
  const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    return -1;
  }

  struct sockaddr_un addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socketPath.c_str());

  if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
    close(fd);
    return -1;
  }

  return fd;
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <interface-name> <socket-path>" << std::endl;
    return 1;
  }

  const std::string interfaceName = argv[1];
  const std::string socketPath = argv[2];

  std::signal(SIGINT, HandleSigInt);

  const int serverFd = ConnectToServer(socketPath);
  if (serverFd < 0) {
    std::cerr << "ERROR: Could not connect to network monitor at " << socketPath << std::endl;
    return 1;
  }

  if (!SendMessage(serverFd, netmon::kReady)) {
    std::cerr << "ERROR: Failed to send Ready message" << std::endl;
    close(serverFd);
    return 1;
  }

  bool monitoringEnabled = false;

  while (g_running) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(serverFd, &readSet);

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    const int ready = select(serverFd + 1, &readSet, nullptr, nullptr, &timeout);
    if (ready < 0) {
      if (errno == EINTR) {
        continue;
      }
      std::cerr << "ERROR: select() failed" << std::endl;
      break;
    }

    if (ready == 0) {
      if (!monitoringEnabled) {
        continue;
      }

      InterfaceStats stats;
      if (!ReadInterfaceStats(interfaceName, stats)) {
        std::cerr << "ERROR: Could not read interface stats for " << interfaceName << std::endl;
        continue;
      }

      PrintStats(interfaceName, stats);

      if (stats.state != "up") {
        if (!SendMessage(serverFd, netmon::kLinkDown)) {
          std::cerr << "ERROR: Failed to report link-down" << std::endl;
          break;
        }

        std::string command;
        if (!RecvMessage(serverFd, command)) {
          std::cerr << "ERROR: Failed to receive command after link-down" << std::endl;
          break;
        }

        if (command == netmon::kSetLinkUp) {
          if (!SetInterfaceUp(interfaceName)) {
            std::cerr << "ERROR: Failed to set link up for " << interfaceName << std::endl;
          }
        }
      }

      continue;
    }

    if (FD_ISSET(serverFd, &readSet)) {
      std::string command;
      if (!RecvMessage(serverFd, command)) {
        break;
      }

      if (command == netmon::kMonitor) {
        monitoringEnabled = true;
        if (!SendMessage(serverFd, netmon::kMonitoring)) {
          std::cerr << "ERROR: Failed to send Monitoring message" << std::endl;
          break;
        }
      } else if (command == netmon::kShow) {
        InterfaceStats stats;
        if (!ReadInterfaceStats(interfaceName, stats)) {
          std::cerr << "ERROR: Could not read interface stats for " << interfaceName << std::endl;
        } else {
          PrintStats(interfaceName, stats);
        }

        if (!SendMessage(serverFd, netmon::kShown)) {
          std::cerr << "ERROR: Failed to send Shown message" << std::endl;
          break;
        }
      } else if (command == netmon::kSetLinkUp) {
        if (!SetInterfaceUp(interfaceName)) {
          std::cerr << "ERROR: Failed to set link up for " << interfaceName << std::endl;
        }
      } else if (command == netmon::kShutDown) {
        SendMessage(serverFd, netmon::kDone);
        break;
      }
    }
  }

  close(serverFd);
  return 0;
}
