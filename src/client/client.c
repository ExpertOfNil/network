#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "proto.h"

void send_disconnect(int fd) {
    u8 buf[4096] = {0};
    frame_hdr_t* header = (frame_hdr_t*)&buf[0];
    header->type = PROTO_DISCONNECT;
    header->data_len = 0;
    header->version = 1;

    frame_t frame = {0};
    frame.header = *header;
    frame.data = &buf[sizeof(frame_hdr_t)];

    i64 bytes = sizeof(frame_hdr_t) + header->data_len;
    i64 bytes_sent = send_frame(fd, &frame);
    if (bytes != bytes_sent) {
        printf("ERROR: Packet size: %li, Sent: %li\n", bytes, bytes_sent);
    }
}

void send_data(int fd) {
    u8 write_data[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    u8 buf[4096] = {0};
    frame_hdr_t* header = (frame_hdr_t*)&buf[0];
    header->type = PROTO_BIN;
    header->data_len = sizeof(write_data);
    header->version = 1;

    frame_t frame = {0};
    frame.header = *header;
    frame.data = &buf[sizeof(frame_hdr_t)];
    memcpy(frame.data, &write_data, sizeof(write_data));

    i64 bytes = sizeof(frame_hdr_t) + header->data_len;
    i64 bytes_sent = send_frame(fd, &frame);
    if (bytes != bytes_sent) {
        printf("ERROR: Packet size: %li, Sent: %li\n", bytes, bytes_sent);
    }
}

int main(int argc, char* argv[]) {
    struct sockaddr_in server_info = {0};

    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_info.sin_port = htons(PORT);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    if (connect(fd, (struct sockaddr*)&server_info, sizeof(server_info)) ==
        -1) {
        perror("connect");
        close(fd);
        return EXIT_FAILURE;
    }
    frame_t* frame = recv_frame(fd);
    if (frame == NULL) {
        printf("Got NULL frame.\n");
        return EXIT_FAILURE;
    }
    switch (frame->header.type) {
        case PROTO_DISCONNECT:
            printf("Server disconnected.\n");
            return EXIT_SUCCESS;
        case PROTO_HELLO:
            printf("Connected to server using v%u\n", frame->header.version);
            break;
        default:
            printf("Unknown or unimplemented frame type.\n");
            return EXIT_FAILURE;
    }
    for (int i = 0; i < 50; ++i) {
        send_data(fd);
        sleep(1);
    }
    send_disconnect(fd);
    free(frame);
    close(fd);
    return EXIT_SUCCESS;
}
