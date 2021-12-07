/*
 * =============================================================================
 *       Filename:  logcsi.c
 *
 *    Description:  Here is an example for receiving CSI matrix
 *                  Basic CSI processing function is also implemented and called
 *                  Check logcsi.h for the detail of the processing function
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

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <termios.h>

#include "logcsi.h"

#define BUFSIZE 65536

bool recording = true;
int log_recv_count = 0;
int log_write_count = 0;

void sigHandler(int signo)
{
  recording = false;
}

int main(int argc, char *argv[])
{
  /* check is got privileged */
  if (geteuid() != 0) {
    printf("Run this with the root privilege for correct socket descriptor.\n");
    exit(0);
  }

  /* check usage and open file */
  bool file_flag = true;
  bool verbose_flag = false;
  char *file_name = NULL;
  FILE *log = NULL;
  if (1 == argc)
  {
    file_flag = false;
    printf("NOTE:\n");
    printf("  To log CSI values in file, use below.\n");
    printf("  => %s [FILE_NAME]\n\n", argv[0]);
  } else if (
    (strncmp("--help", argv[1], 6) == 0)
    || (strncmp("help", argv[1], 4) == 0)
    || (strncmp("-h", argv[1], 2) == 0)
  ) {
    printf("Usage  : %s [-v|--verbose] [FILE_NAME]\n", argv[0]);
    printf("Example: %s\n", argv[0]);
    printf("         Just see CSI log without saving them\n");
    printf("Example: %s test.dat\n", argv[0]);
    printf("         Write CSI log to test.dat\n");
    printf("Example: %s -v test.dat\n", argv[0]);
    printf("         Write CSI log to test.dat with printing details\n");
    exit(0);
  } else {
    file_name = argv[1];
    if (
      (strncmp("-v", argv[1], 2) == 0)
      || (strncmp("--verbose", argv[1], 9) == 0)
    ) {
      verbose_flag = true;
      if (argv[2] == NULL) {
        file_flag = false;
      } else {
        file_name = argv[2];
      }
    }
    if (file_flag) {
      log = fopen(file_name, "w");
      if (!log) {
        printf("Failed to open %s for write!\n", file_name);
        exit(errno);
      }
    }
  }

  /* open CSI_dev */
  int csi_device = open_csi_device();
  if (csi_device < 0)
  {
    if (log) fclose(log);
    perror("Failed to open CSI device!");
    exit(errno);
  }

  /* get CSI values */
  ssize_t read_size;
  unsigned char buf_addr[BUFSIZE + 2];
  CSISTAT *csi_status = (CSISTAT *)malloc(sizeof(CSISTAT));
  size_t write_size;
  const bool disp_info = !file_flag || verbose_flag;
  const char *okay_sign = disp_info ? " -> OK" : ".";
  const char *csi_broken_sign = disp_info ? " -> CSI Broken = Throw" : "B";
  const char *bw_mismatch_sign = disp_info ? " -> Bandwidth Mismatch = Throw" : "W";
  const char *write_fail_sign = disp_info ? " -> Write Fail" : "F";
  const char *not_intended_sign = disp_info ? " -> Not Intended = Throw" : "I";

  /* check is connected to AP */
  if (system("iw dev wlp1s0 info | grep 'ssid' > /dev/null") != 0) {
    printf("The injector is not connected!");
    exit(1);
  }

  /* get bandwidth */
  bool bandwidth = 0;
  if (system("iw dev wlp1s0 info | grep '20 MHz' > /dev/null") != 0) {
    bandwidth = 1;
  }

  /* print iw dev wlp1s0 info */
  system("iw dev wlp1s0 info");

  //revised for udp

  char readBuff[BUFFER_SIZE];
  char sendBuff[BUFFER_SIZE];
  struct sockaddr_in serverAddress, clientAddress;
  int server_fd, client_fd;
  int client_addr_size;
  ssize_t receivedBytes;
  ssize_t sentBytes;

  /*
  if (argc != 2)
  {
  printf("사용법 : ./filename 포트번호 \n");
  exit(0);
  }
  */

  socklen_t clientAddressLength = 0;

  memset(&serverAddress, 0, sizeof(serverAddress));
  memset(&clientAddress, 0, sizeof(clientAddress));

  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  serverAddress.sin_port = htons(20162);


  // 서버 소켓 생성 및 서버 주소와 bind

  // 서버 소켓 생성(UDP니 SOCK_DGRAM이용)
  if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) // SOCK_DGRAM : UDP
  {
      printf("Sever : can not Open Socket\n");
      exit(0);
  }

  // bind 과정
  if (bind(server_fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
  {
      printf("Server : can not bind local address");
      exit(0);
  }
  printf("Server: waiting connection request.\n");

  struct sockaddr_in connectSocket;
  socklen_t connectSocketLength = sizeof(connectSocket);
  getpeername(client_fd, (struct sockaddr*)&clientAddress, &connectSocketLength);
  char clientIP[sizeof(clientAddress.sin_addr) + 1] = { 0 };
  sprintf(clientIP, "%s", inet_ntoa(clientAddress.sin_addr));
  // 접속이 안되었을 때는 while에서 출력 x
  if (strcmp(clientIP, "0.0.0.0") != 0)
      printf("Client : %s\n", clientIP);


  //채팅 프로그램 제작
  client_addr_size = sizeof(clientAddress);

  receivedBytes = recvfrom(server_fd, readBuff, BUFF_SIZE, 0, (struct sockaddr*)&clientAddress, &client_addr_size);
  printf("%lu bytes read\n", receivedBytes);
  readBuff[receivedBytes] = '\0';
  fputs(readBuff, stdout);
  fflush(stdout);

  /* listen CSI */
  printf("\nReceiving data... Press Ctrl+C to quit.\n\n");
  signal(SIGINT, sigHandler);
  setbuf(stdout, NULL);

  while (recording)
  {

    /* keep listening to the kernel and waiting for the csi report */
    read_size = read_csi_buf(&buf_addr[2], csi_device, BUFSIZE);

    if (read_size)
    {
      log_recv_count += 1;

      /* fill the status struct with information about the rx packet */
      if (disp_info) {
        record_status(&buf_addr[2], read_size, csi_status);
        fprintf(
          stdout,
          "%d (%d): phyerr(%d) payload(%d) csi(%d) rate(0x%d) nt(%d) nr(%d) nc(%d) timestamp(%lld)",
          log_write_count + 1, csi_status->buf_len, csi_status->phyerr,
          csi_status->payload_len, csi_status->csi_len, csi_status->rate,
          csi_status->nt, csi_status->nr, csi_status->nc,
          csi_status->timestamp
        );
      } else {
        record_status_min(&buf_addr[2], read_size, csi_status);
      }

      /* check is really intended packet */
      if (
          (csi_status->checker[0] != 0x23)
          || (csi_status->checker[1] != 0x50)
          || (csi_status->checker[2] != 0xde)
          || (csi_status->checker[3] != 0xe3)
          ) {
          fprintf(stdout, not_intended_sign);
      }

      /* log the received data */
      else if (file_flag)
      {
        if (csi_status->nt == 0) {
          fprintf(stdout, csi_broken_sign);
        } else if (csi_status->bandwidth != bandwidth) {
          fprintf(stdout, bw_mismatch_sign);
        } else {
          buf_addr[0] = csi_status->buf_len & 0xFF;
          buf_addr[1] = csi_status->buf_len >> 8;
          write_size = fwrite(buf_addr, 1, csi_status->buf_len + 2, log);
          send(clint_sock, buf_addr, csi_status->buf_len + 2,0);
          sentBytes = sendto(server_fd, buf_addr, csi_status->buf_len + 2, 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));

          if (1 > write_size) {
            fprintf(stdout, write_fail_sign);
            perror("fwrite");
            recording = 0;
          } else {
            fprintf(stdout, okay_sign);
            log_write_count += 1;
          }
        }
      }

      if (disp_info) fprintf(stdout, "\n");
    }
  }

  /* clean */
  printf("\n\nReceived %d packets.\n", log_recv_count);
  if (file_flag) {
    printf("Wrote %d packets to \"%s\".\n", log_write_count, file_name);
    fclose(log);
  }
  close_csi_device(csi_device);
  free(csi_status);

  close(serv_sock);
  close(clint_sock);

  return 0;
}
