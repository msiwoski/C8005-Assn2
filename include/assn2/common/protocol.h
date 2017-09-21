//
// Created by shane on 2/18/17.
//

#ifndef COMP8005_ASSN2_PROTOCOL_H
#define COMP8005_ASSN2_PROTOCOL_H
#include <sys/types.h>
#include <stdint.h>

/**
 * Attempts to send all bytes in buffer. If the send produces EWOULDBLOCK, returns the number of bytes
 * successfully sent. Otherwise, returns -1.
 *
 * @param sock          The socket on which to send the data.
 * @param buffer        The data to send.
 * @param bytes_to_send The size of the data to send.
 * @return -1 on error (except EWOULDBLOCK), or the number of bytes successfully sent.
 */
ssize_t send_data(int sock, const void* buffer, size_t bytes_to_send);

/**
 * Attempts to send all bytes in buffer. If the send produces EWOULDBLOCK, returns the number of bytes
 * successfully sent. Otherwise, returns -1.
 *
 * @param sock          The socket from which to read the data.
 * @param buffer        The buffer into which the data will be read.
 * @param bytes_to_send The number of bytes to read
 * @return -1 on error (except EWOULDBLOCK), or the number of bytes successfully read.
 */
ssize_t read_data(int sock, void *buffer, size_t bytes_to_read);

#endif //COMP8005_ASSN2_PROTOCOL_H
