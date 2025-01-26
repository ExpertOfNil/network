#ifndef PROTO_H
#define PROTO_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/* Maintain a strict maximum packet size */
#define MAX_FRAME_SIZE 4096
#define ADDR "127.0.0.1"
#define PORT 6969

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef size_t usize;
typedef ssize_t isize;

/* Frame types */
typedef enum {
    PROTO_HELLO,
    PROTO_DISCONNECT,
    PROTO_BIN,
} frame_type_e;

/* Frame header common to all frames */
typedef struct {
    frame_type_e type;
    u16 data_len;
    u16 version;
} frame_hdr_t;

/* All information needed to fully characterize a frame */
typedef struct {
    frame_hdr_t header;
    u8* data;
} frame_t;

/* Read a frame from the socket */
int recv_frame(int fd, frame_t* frameOut) {
    u8 buf[MAX_FRAME_SIZE] = {0};
    frame_hdr_t* header = (frame_hdr_t*)&buf[0];
    ssize_t bytes = read(fd, buf, sizeof(buf));
    if (bytes == -1) {
        perror("read");
        return EXIT_FAILURE;
    }
    printf("Bytes read: %li\n", bytes);
    if (bytes > MAX_FRAME_SIZE) {
        printf("Frame too large!");
        return EXIT_FAILURE;
    }

    if (frameOut == NULL) return EXIT_FAILURE;
    frameOut->header.type = (frame_type_e)ntohl(header->type);
    frameOut->header.data_len = ntohs(header->data_len);
    frameOut->header.version = ntohs(header->version);
    memcpy(
        frameOut->data, &buf[sizeof(frame_hdr_t)], frameOut->header.data_len);

    return EXIT_SUCCESS;
}

/* Write a frame to the socket */
i64 send_frame(int fd, frame_t* frame) {
    size_t bytes = sizeof(frame_hdr_t) + frame->header.data_len;

    if (bytes > MAX_FRAME_SIZE) {
        printf("Frame too large!");
        return EXIT_FAILURE;
    }

    u8 buf[MAX_FRAME_SIZE] = {0};
    frame_hdr_t* hdr = (frame_hdr_t*)&buf[0];
    hdr->type = (frame_type_e)htonl(frame->header.type);
    hdr->data_len = htons(frame->header.data_len);
    hdr->version = htons(frame->header.version);
    memcpy(&buf[sizeof(frame_hdr_t)], frame->data, frame->header.data_len);

    i64 bytes_written =
        write(fd, &buf, sizeof(frame_hdr_t) + frame->header.data_len);
    if (bytes_written == -1) {
        perror("write");
    }
    printf("Bytes written: %li\n", bytes_written);
    return bytes_written;
}

#endif /* PROTO_H */
