#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <errno.h>
#include <sched.h>

#define BT_AF_BLUETOOTH   31
#define BT_SOCK_SEQPACKET 5
#define BT_BTPROTO_L2CAP  0

#define IDX_SRV_CTRL   0
#define IDX_SRV_INTR   1
#define IDX_CLI_CTRL   2
#define IDX_VDSD_CTRL  3
#define IDX_CLI_INTR   4
#define IDX_VDSD_INTR  5
#define TOTAL_FDS      6

int set_nonblocking_fd(int fd) {
    if (fd < 0) {
        fprintf(stderr, "vDS-Proxy: Fehler beim Setzen des non-blocking Modus für fd %d: %s\n", fd, strerror(errno));
        return -1;
    }
    int fl = fcntl(fd, F_GETFL, 0);
    if (fl == -1) {
        fprintf(stderr, "vDS-Proxy: Fehler beim Abrufen des File-Descriptor-Flags für fd %d: %s\n", fd, strerror(errno));
        return -1;
    }
    return fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

int open_bt_server_link(uint16_t psm) {
    int sock = socket(BT_AF_BLUETOOTH, BT_SOCK_SEQPACKET, BT_BTPROTO_L2CAP);
    if (sock < 0) {
        fprintf(stderr, "vDS-Proxy: Fehler beim Erstellen des Bluetooth-Server-Sockets: %s\n", strerror(errno));
        return -1;
    }
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (set_nonblocking_fd(sock) < 0) {
        close(sock);
        return -1;
    }

    uint8_t addr_bytes[16];
    memset(addr_bytes, 0, 16);
    addr_bytes[0] = BT_AF_BLUETOOTH & 0xFF;
    addr_bytes[1] = (BT_AF_BLUETOOTH >> 8) & 0xFF;
    addr_bytes[2] = psm & 0xFF;
    addr_bytes[3] = (psm >> 8) & 0xFF;
    if (bind(sock, (struct sockaddr *)addr_bytes, 16) < 0) {
        fprintf(stderr, "vDS-Proxy: Fehler beim Binden des Bluetooth-Server-Sockets: %s\n", strerror(errno));
        close(sock);
        return -1;
    }
    if (listen(sock, 5) < 0) {
        fprintf(stderr, "vDS-Proxy: Fehler beim Starten des Bluetooth-Server-Sockets: %s\n", strerror(errno));
        close(sock);
        return -1;
    }
    return sock;
}

int connect_unix_pipe(const char *name_three_bytes) {
    int sock = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
    if (sock < 0) {
        fprintf(stderr, "vDS-Proxy: Fehler beim Erstellen des UNIX-Sockets: %s\n", strerror(errno));
        return -1;
    }

    struct sockaddr_un addr;
}
