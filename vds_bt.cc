#include "vds_bt.hh"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <span>
#include <cstddef>
#include <stdio.h>
#include <cerrno>
namespace vds {
static void setup_abstract_un(struct sockaddr_un &un_addr, const char *name) {
    std::memset(&un_addr, 0, sizeof(struct sockaddr_un));
    un_addr.sun_family = AF_UNIX;
    std::memcpy(un_addr.sun_path + 1, name, 3);
}
static UniqueFd create_ipc_listener(const char *name) {
    fprintf(stderr, "vDS-CORE: UNTERSTUETZUNG FUER ABSTRAKTE UNIX-SOCKETS AKTIV! Erstelle Pipeline: @%s\n", name);
    fflush(stderr);
    int fd = ::socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        fprintf(stderr, "vDS-CORE: Fehler beim Erstellen des IPC-Sockets: %s\n", std::strerror(errno));
        fflush(stderr);
        throw std::runtime_error("IPC Socket Creation Failed");
    }

    int reuse = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_un un_addr;
    setup_abstract_un(un_addr, name);

    socklen_t actual_len = offsetof(struct sockaddr_un, sun_path) + 1 + 3;

    if (::bind(fd, reinterpret_cast<const struct sockaddr*>(&un_addr), actual_len) < 0) {
        fprintf(stderr, "vDS-CORE: FATAL - Bind fuer @%s failed: %s\n", name, std::strerror(errno));
        fflush(stderr);
        ::close(fd);
        throw std::runtime_error("IPC Bind Failed");
    }

    if (::listen(fd, SOMAXCONN) < 0) {
        fprintf(stderr, "vDS-CORE: Fehler beim Starten des IPC-Sockets: %s\n", std::strerror(errno));
        fflush(stderr);
        ::close(fd);
        throw std::runtime_error("IPC Listen Failed");
    }

    return UniqueFd(fd);
}

BtDaemon::BtDaemon(const std::string &device_path, const std::string &config_path)
    : device_path_(device_path), config_path_(config_path), device_(nullptr) {
    // Load configuration from the provided config_path
    std::ifstream config_file(config_path_);
    if (!config_file.is_open()) {
        throw std::runtime_error("Failed to open configuration file");
    }

    std::string line;
    while (std::getline(config_file, line)) {
        // Process each line of the configuration file
        // For simplicity, let's assume the configuration file contains key-value pairs
        // separated by a space
        size_t pos = line.find(' ');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            config_[key] = value;
        }
    }

    // Initialize the Bluetooth device
    device_ = std::make_unique<BtDevice>(device_path_, config_);
}

BtDaemon::~BtDaemon() {
    // Clean up resources
}

void BtDaemon::start() {
    // Start the Bluetooth daemon
    device_->start();
}

void BtDaemon::stop() {
    // Stop the Bluetooth daemon
    device_->stop();
}

// Additional methods and implementations for BtDaemon can be added here

} // namespace vds
