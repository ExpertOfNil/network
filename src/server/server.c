#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "proto.h"

#define MAX_CLIENTS 256

typedef enum {
    STATE_NEW,
    STATE_CONNECTED,
    STATE_DISCONNECTED,
} state_e;

typedef struct {
    int fd;
    state_e state;
    u8 buffer[MAX_FRAME_SIZE];
} client_state_t;

client_state_t client_states[MAX_CLIENTS];

void init_clients() {
    for (size_t i = 0; i < MAX_CLIENTS; ++i) {
        client_states[i].fd = -1;
        client_states[i].state = STATE_NEW;
        memset(&client_states[i].buffer, 0, MAX_FRAME_SIZE);
    }
}

int find_free_slot() {
    for (size_t i = 0; i < MAX_CLIENTS; ++i) {
        if (client_states[i].fd == -1) {
            return i;
        }
    }
    return -1;
}

int find_slot_by_fd(int fd) {
    for (size_t i = 0; i < MAX_CLIENTS; ++i) {
        if (client_states[i].fd == fd) {
            return i;
        }
    }
    return -1;
}

void send_hello(int fd) {
    u8 buf[4096] = {0};
    frame_t* frame = (frame_t*)&buf[0];
    frame->header.type = PROTO_HELLO;
    frame->header.data_len = 0;
    frame->header.version = 1;
    i64 bytes = sizeof(frame_hdr_t) + frame->header.data_len;
    i64 bytes_sent = send_frame(fd, frame);
    if (bytes != bytes_sent) {
        printf("ERROR: Packet size: %li, Sent: %li\n", bytes, bytes_sent);
    }
}

int handle_frame(u8* buffer) {
    if (buffer == NULL) {
        printf("Found NULL buffer.");
        return STATE_CONNECTED;
    };
    frame_hdr_t* header = (frame_hdr_t*)buffer;
    header->type = (frame_type_e)ntohl(header->type);
    printf("Message Type: %d\n", header->type);
    header->data_len = ntohs(header->data_len);
    printf("Data Len: %d\n", header->data_len);
    header->version = ntohs(header->version);
    printf("Version: %d\n", header->version);

    switch (header->type) {
        case PROTO_HELLO:
            printf("Got HELLO frame.\n");
            break;
        case PROTO_DISCONNECT:
            printf("Disconnecting.\n");
            return STATE_DISCONNECTED;
        case PROTO_BIN: {
            if (fwrite(buffer, header->data_len, 1, stdout) != 1) {
                perror("fwrite");
                return EXIT_FAILURE;
            }
            printf("Got binary data: %u bytes\n", header->data_len);
            size_t offset = sizeof(frame_hdr_t);
            for (size_t i = 0; i < header->data_len; ++i) {
                printf("%u ", buffer[offset + i]);
            }
            printf("\nEOF\n\n");
            break;
        }
        default:
            printf("Unknown frame type: %u\n", header->type);
    }
    return STATE_CONNECTED;
}

int main() {
    int listen_fd = -1;
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(listen_fd);
        return EXIT_FAILURE;
    }
    if (setsockopt(listen_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = 0;
    server_addr.sin_port = htons(PORT);

    // Bind
    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        perror("bind");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    // Listen
    if (listen(listen_fd, 0)) {
        perror("listen");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    printf("Server listening on port %d\n", PORT);

    init_clients();
    struct pollfd fds[MAX_CLIENTS + 1];

    memset(fds, 0, sizeof(fds));
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    int n_fds = 1;

    int conn_fd = -1;
    int free_slot = -1;
    struct sockaddr_in client_addr = {0};
    socklen_t client_len = sizeof(client_addr);

    while (1) {
        size_t ii = 1;
        for (size_t i = 0; i < MAX_CLIENTS; ++i) {
            if (client_states[i].fd != -1) {
                fds[ii].fd = client_states[i].fd;
                fds[ii].events = POLLIN;
                ii++;
            }
        }

        // Wait for an event on one of the sockets
        int n_events = poll(fds, n_fds, -1);
        if (n_events == -1) {
            perror("poll");
            return EXIT_FAILURE;
        }

        if (fds[0].revents & POLLIN) {
            if ((conn_fd = accept(
                     listen_fd, (struct sockaddr*)&client_addr, &client_len)) ==
                -1) {
                perror("accept");
                return EXIT_FAILURE;
            }

            printf(
                "New connection from %s:%d\n",
                inet_ntoa(client_addr.sin_addr),
                ntohs(client_addr.sin_port));
            free_slot = find_free_slot();
            if (free_slot == -1) {
                printf("Server full: closing new connection.\n");
                close(conn_fd);
            } else {
                client_states[free_slot].fd = conn_fd;
                client_states[free_slot].state = STATE_CONNECTED;
                n_fds += 1;
                printf(
                    "Slot %d has fd %d\n",
                    free_slot,
                    client_states[free_slot].fd);
                send_hello(client_states[free_slot].fd);
            }
            n_events--;
        }

        // Check each client for read/write activity
        for (size_t i = 1; i <= n_fds && n_events > 0; ++i) {
            if (fds[i].revents & POLLIN) {
                n_events--;

                int fd = fds[i].fd;
                int slot = find_slot_by_fd(fd);
                ssize_t bytes_read =
                    read(fd, &client_states[slot].buffer, MAX_FRAME_SIZE);
                int bytes_available;
                if (bytes_read <= 0) {
                    close(fd);
                    if (slot == -1) {
                        printf("Tried to close fd that doesn't exist?");
                    } else {
                        // Free up the slot
                        client_states[slot].fd = -1;
                        client_states[slot].state = STATE_DISCONNECTED;
                        // Clear client data
                        memset(client_states[slot].buffer, 0, MAX_FRAME_SIZE);
                        printf("Client disconnected or error\n");
                        n_fds--;
                    }
                } else {
                    printf("---- RECV ----\n");
                    printf(
                        "received message from client: %li bytes\n",
                        bytes_read);
                    printf("|");
                    for (size_t i = 0; i < bytes_read; ++i) {
                        printf("%i ", client_states[slot].buffer[i]);
                    }
                    printf("|\n");

                    state_e state = handle_frame(client_states[slot].buffer);
                    // Clear client data
                    memset(client_states[slot].buffer, 0, MAX_FRAME_SIZE);
                    if (state == STATE_DISCONNECTED) {
                        // Free up the slot
                        client_states[slot].fd = -1;
                        client_states[slot].state = STATE_DISCONNECTED;
                        printf("Client disconnected.\n");
                        n_fds--;
                    }
                    printf("--------------\n");
                }
            }
        }
    }
    return EXIT_SUCCESS;
}
