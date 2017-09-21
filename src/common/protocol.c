/*********************************************************************************************
Name:			protocol.c

    Required:	protocol.h	

    Developer:	Mat Siwoski/Shane Spoor

    Created On: 2017-02-17

    Description:
    Handles the protocol send and receive from the sockets. Sends back the total read count 
    to the client

    Revisions:
    (none)

*********************************************************************************************/
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include "protocol.h"

/*********************************************************************************************
FUNCTION

    Name:		send_data

    Prototype:	ssize_t send_data(int sock, void *buffer, size_t bytes_to_send)

    Developer:	Mat Siwoski/Shane Spoor

    Created On: 2017-02-17

    Parameters:
    sock - socket that is having the data sent on.
	buffer - the buffer
    bytes_to_send - the bytes to send from the socket.

    Return Values:
	
    Description:
    Send the data going out on the socket. 

    Revisions:
	(none)

*********************************************************************************************/
ssize_t send_data(int sock, void const *buffer, size_t bytes_to_send)
{
    ssize_t bytes_sent = 0;
    ssize_t sent_total = 0;
    size_t bytes_left = bytes_to_send;

    unsigned char const* raw = (unsigned char const*)buffer;

    while (sent_total < bytes_to_send)
    {
        bytes_sent = send(sock, raw + sent_total, bytes_left, 0);
        if (bytes_sent == -1)
        {
            if (errno == EWOULDBLOCK)
            {
                return sent_total;
            }
            else
            {
                return -1;
            }
        }
        sent_total += bytes_sent;
        bytes_left = bytes_to_send - sent_total;
    }

    return sent_total;
}

/*********************************************************************************************
FUNCTION

    Name:		read_data

    Prototype:	ssize_t read_data(int sock, void *buffer, size_t bytes_to_read)

    Developer:	Mat Siwoski/Shane Spoor

    Created On: 2017-02-17

    Parameters:
    sock - socket that is having the data read on.
	buffer - the buffer
    bytes_to_read - the bytes to read from the socket.

    Return Values:
	
    Description:
    Read the data coming in from the socket. 

    Revisions:
	(none)

*********************************************************************************************/
ssize_t read_data(int sock, void *buffer, size_t bytes_to_read)
{
    ssize_t bytes_read = 0;
    ssize_t read_total = 0;
    size_t bytes_left = bytes_to_read;

    unsigned char* raw = (unsigned char*)buffer;

    while (read_total < bytes_to_read)
    {
        bytes_read = recv(sock, raw + read_total, bytes_left, 0);
        if (bytes_read == -1)
        {
            if (errno == EWOULDBLOCK)
            {
                return read_total;
            }
            return -1;
        }
        else if (bytes_read == 0)
        {
            // Should really propagate this back up to the client
            break;
        }
        read_total += bytes_read;
        bytes_left = bytes_to_read - read_total;
    }

    return read_total;
}