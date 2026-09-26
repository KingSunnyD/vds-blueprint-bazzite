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
#include <optional>

namespace vds {

static void setup_abstract_un(struct sockaddr_un &un_addr, const char *name) {
    std::memset(&un_addr, 0, sizeof(struct sockaddr_un));
    un_addr.sun_family = AF_UNIX;
    std::memcpy(un_addr.sun_path + 1, name, 3);
}

static UniqueFd create_ipc_listener(const char *name) {
    fprintf(stderr, "vDS-CORE: UNTERSTÜTZUNG FÜR ABSTRAKTE UNIX-SOCKETS AKTIV! Erstelle Pipeline: @%s\n", name);
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
        fprintf(stderr, "vDS-CORE: FATAL - Bind für @%s failed: %s\n", name, std::strerror(errno));
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

BtL2capAcceptor::BtL2capAcceptor()
    : control_listener_fd_(create_ipc_listener("v_c")),
      interrupt_listener_fd_(create_ipc_listener("v_i")) {}

std::optional<BtAcceptedChannel> BtL2capAcceptor::accept_control() {
} // namespace vds
