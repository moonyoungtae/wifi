/*
 * =============================================================================
 *       Filename:  logcsi.h
 *
 *    Description:  header file for csi processing function
 *        Version:  1.1
 *
 *         Author:  Yaxiong Xie
 *          Email:  <xieyaxiongfly@gmail.com>
 *   Organization:  WANDS group @ Nanyang Technological University
 *
 *  Refactored by:  Jio Gim <jio@wldh.org>
 *
 *   Copyright (c)  WANDS group @ Nanyang Technological University
 * =============================================================================
 */

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

# define PORT 20162
# define BUFFER_SIZE 4096 // for server
# define BUFF_SIZE 100 // for client


typedef struct
{
  u_int64_t timestamp;   // h/w assigned time stamp

  u_int16_t channel;     // wireless channel (represented in Hz)
  u_int8_t  bandwidth;   // channel bandwidth (0->20MHz, 1->40MHz)

  u_int8_t  rate;        // transmission rate
  u_int8_t  nr;          // number of receiving antennas
  u_int8_t  nt;          // number of transmitting antennas
  u_int8_t  nc;          // number of tones (subcarriers)
  u_int8_t  noise;       // noise floor (to be updated)

  u_int8_t  phyerr;      // phy error code (set to 0 if correct)

  u_int8_t  rssi;        // rx frame RSSI
  u_int8_t  rssi_0;      // rx frame RSSI [ctl, chain 0]
  u_int8_t  rssi_1;      // rx frame RSSI [ctl, chain 1]
  u_int8_t  rssi_2;      // rx frame RSSI [ctl, chain 2]

  u_int16_t payload_len; // payload length (bytes)
  u_int16_t csi_len;     // csi data length (bytes)
  u_int16_t buf_len;     // data length in buffer

  u_int8_t  checker[4];  // destination MAC address
} CSISTAT;

inline bool is_big_endian()
{
  const unsigned int a = 0x1;
  const unsigned char b = *(unsigned char *)&a;
  if (b == 0)
  {
    return true;
  }
  return false;
}

inline int open_csi_device()
{
  return open("/dev/CSI_dev", O_RDWR);
}

inline void close_csi_device(int fd)
{
  close(fd);
}

inline ssize_t read_csi_buf(unsigned char *buf_addr, int fd, int BUFSIZE)
{
  /* listen to the port
   * read when  i) a csi is reported from kernel
   *           ii) timed out
   */
  ssize_t result = read(fd, buf_addr, BUFSIZE);
  if (result > 0) {
    return result;
  } else {
    return 0;
  }
}

void record_status_min(unsigned char *buf_addr, ssize_t read_size, CSISTAT *csi_status)
{
  if (is_big_endian())
  {
    csi_status->buf_len =
        ((buf_addr[read_size - 2] << 8) & 0xff00)
      | (buf_addr[read_size - 1]        & 0x00ff);
  }
  else
  {
    csi_status->buf_len =
        ((buf_addr[read_size - 1] << 8) & 0xff00)
      | (buf_addr[read_size - 2]        & 0x00ff);
  }

  csi_status->nt = buf_addr[18];

  csi_status->checker[0] = buf_addr[read_size - 10];
  csi_status->checker[1] = buf_addr[read_size - 9];
  csi_status->checker[2] = buf_addr[read_size - 8];
  csi_status->checker[3] = buf_addr[read_size - 7];
}

void record_status(unsigned char *buf_addr, ssize_t read_size, CSISTAT *csi_status)
{
  record_status_min(buf_addr, read_size, csi_status);

  if (is_big_endian())
  {
    csi_status->timestamp =
        (((u_int64_t)buf_addr[0] << 56) & 0xff00000000000000)
      | (((u_int64_t)buf_addr[1] << 48) & 0x00ff000000000000)
      | (((u_int64_t)buf_addr[2] << 40) & 0x0000ff0000000000)
      | (((u_int64_t)buf_addr[3] << 32) & 0x000000ff00000000)
      | (((u_int64_t)buf_addr[4] << 24) & 0x00000000ff000000)
      | (((u_int64_t)buf_addr[5] << 16) & 0x0000000000ff0000)
      | (((u_int64_t)buf_addr[6] << 8)  & 0x000000000000ff00)
      | (((u_int64_t)buf_addr[7])       & 0x00000000000000ff);
    csi_status->csi_len =
        ((buf_addr[8] << 8) & 0xff00)
      | (buf_addr[9]        & 0x00ff);
    csi_status->channel =
        ((buf_addr[10] << 8) & 0xff00)
      | (buf_addr[11]        & 0x00ff);
    csi_status->payload_len =
        ((buf_addr[23] << 8) & 0xff00)
      | ((buf_addr[24])      & 0x00ff);
  }
  else
  {
    csi_status->timestamp =
        (((u_int64_t)buf_addr[7] << 56) & 0xff00000000000000)
      | (((u_int64_t)buf_addr[6] << 48) & 0x00ff000000000000)
      | (((u_int64_t)buf_addr[5] << 40) & 0x0000ff0000000000)
      | (((u_int64_t)buf_addr[4] << 32) & 0x000000ff00000000)
      | (((u_int64_t)buf_addr[3] << 24) & 0x00000000ff000000)
      | (((u_int64_t)buf_addr[2] << 16) & 0x0000000000ff0000)
      | (((u_int64_t)buf_addr[1] << 8)  & 0x000000000000ff00)
      | (((u_int64_t)buf_addr[0])       & 0x00000000000000ff);
    csi_status->csi_len =
        ((buf_addr[9] << 8) & 0xff00)
      | (buf_addr[8]        & 0x00ff);
    csi_status->channel =
        ((buf_addr[11] << 8) & 0xff00)
      | (buf_addr[10]        & 0x00ff);
    csi_status->payload_len =
        ((buf_addr[24] << 8) & 0xff00)
      | (buf_addr[23]        & 0x00ff);
  }

  csi_status->phyerr = buf_addr[12];
  csi_status->noise = buf_addr[13];
  csi_status->rate = buf_addr[14] & 0x7f; // Remove HT flag to see MCS directly
  csi_status->bandwidth = buf_addr[15];
  csi_status->nc = buf_addr[16];
  csi_status->nr = buf_addr[17];

  csi_status->rssi = buf_addr[19];
  csi_status->rssi_0 = buf_addr[20];
  csi_status->rssi_1 = buf_addr[21];
  csi_status->rssi_2 = buf_addr[22];
}

