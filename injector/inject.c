/*
 * =============================================================================
 *       Filename:  inject.c
 *
 *    Description:  send packets with milisecond interval
 *        Version:  1.1
 *
 *         Author:  Yaxiong Xie
 *          Email:  <xieyaxiongfly@gmail.com>
 *   Organization:  WANDS group @ Nanyang Technological University
 *
 *  Refactored by:  Jio Gim <jio@wldh.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * =============================================================================
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_DEST_MAC0 0x12
#define DEFAULT_DEST_MAC1 0x34
#define DEFAULT_DEST_MAC2 0x56
#define DEFAULT_DEST_MAC3 0x78
#define DEFAULT_DEST_MAC4 0x90
#define DEFAULT_DEST_MAC5 0xFF

#define DEFAULT_INTERFACE_NAME "wlp1s0"
#define DEFAULT_PACKET_SIZE 100
#define DEFAULT_SEND_DELAY 10000

int main(int argc, char *argv[]) {

  /* Check is got privileged */
  if (geteuid() != 0) {
    printf("Run this with the root privilege for correct socket descriptor.\n");
    exit(0);
  }

  /* Check is help message time */
  if (
    argc == 1
    || (argc >= 2 && strncmp("help", argv[1], 4) == 0)
    || (argc >= 2 && strncmp("-h", argv[1], 2) == 0)
    || (argc >= 2 && strncmp("--help", argv[1], 6) == 0)
  ) {
    printf("Usage  : %s [...OPTIONS] PACKETS_TO_SEND\n\n", argv[0]);
    printf("OPTIONS\n");
    printf("  -d SEND_DELAY    Injection interval in microseconds. Set this zero for no-\n");
    printf("                   delay injection. (Default: %d)\n", DEFAULT_SEND_DELAY);
    printf("  -i INTERFACE     The name of network interface that the injector will use.\n");
    printf("                   (Default: %s)\n", DEFAULT_INTERFACE_NAME);
    printf("  -s PACKET_SIZE   The size of packets in bytes (18 <= PACKET_SIZE <= 2048).\n");
    printf("                   (Default: %d)\n", DEFAULT_PACKET_SIZE);
    printf("  -a ADDRESS       The MAC address of the injection target client station.\n");
    printf("                   (Default: %02x:%02x:%02x:%02x:%02x:%02x)\n",
      DEFAULT_DEST_MAC0, DEFAULT_DEST_MAC1, DEFAULT_DEST_MAC2,
      DEFAULT_DEST_MAC3, DEFAULT_DEST_MAC4, DEFAULT_DEST_MAC5);
    printf("  -c               0xCC mode. If set, all payloads are filled with 0xC.\n");
    printf("                   (Default: random)\n");
    printf("  -v               View ethernet packet contents before injection.\n");
    printf("  -h               This help message.\n\n");
    printf("PACKETS_TO_SEND\n  The number of packets to inject. Set this zero for infinite injection.\n");
    exit(0);
  }

  char* readStrEnd;
  unsigned long long cnt = 0;
  bool infCnt = cnt == 0;
  unsigned long delay = DEFAULT_SEND_DELAY;
  bool delayExist = delay > 0;
  char* interface = DEFAULT_INTERFACE_NAME;
  unsigned short pktSize = DEFAULT_PACKET_SIZE;
  char dstAddr[6] = {
    DEFAULT_DEST_MAC0, DEFAULT_DEST_MAC1, DEFAULT_DEST_MAC2,
    DEFAULT_DEST_MAC3, DEFAULT_DEST_MAC4, DEFAULT_DEST_MAC5
  };
  bool cMode = false;
  bool vMode = false;

  /* Get arguments */
  unsigned int argi;
  char opti = 0;
  bool cntSet = false;
  for (argi = 1; argi < argc; argi += 1) {
    if (opti != 0) {
      switch (opti) {
        case 'd':
          delay = strtoul(argv[argi], &readStrEnd, 10);
          break;
        case 'i':
          interface = argv[argi];
          break;
        case 's':
          pktSize = (unsigned short) strtoul(argv[argi], &readStrEnd, 10);
          break;
        case 'a':
          sscanf(argv[argi], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
            &dstAddr[0], &dstAddr[1], &dstAddr[2],
            &dstAddr[3], &dstAddr[4], &dstAddr[5]);
          break;
        default:
          printf("Unknown option error: -%c\n", opti);
          exit(1);
          break;
      }
      opti = 0;
    } else if (argv[argi][0] == '-') {
      if (strlen(argv[argi]) < 2) {
        printf("Invalid option prefix: -\n");
        exit(1);
      } else {
        switch (argv[argi][1]) {
          case 'd':
          case 'i':
          case 's':
          case 'a':
            opti = argv[argi][1];
            break;
          case 'c':
            cMode = true;
            break;
          case 'v':
            vMode = true;
            break;
          default:
            printf("Invalid option prefix: -%c\n", argv[argi][1]);
            exit(1);
            break;
        }
      }
    } else {
      cnt = strtoull(argv[argi], &readStrEnd, 10);
      cntSet = true;
    }
  }

  if (opti != 0) {
    printf("Option value was not given: -%c\n", opti);
    exit(1);
  } else if (cntSet == false) {
    printf("Packets to send are not set.\n");
    exit(1);
  } else if (pktSize > 2048 || pktSize < 18) {
    printf("Packet size should be more than 17 (>=18), less than 2049 (<=2048).\n");
    exit(1);
  } else {
    infCnt = cnt == 0;
    delayExist = delay > 0;
  }

  /* Open RAW socket to send on */
  int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (sock == -1) {
    perror("socket");
  }

  /* Get the index & mac of the interface to send on */
  struct ifreq ifIndex, ifMAC;
  memset(&ifIndex, 0, sizeof(struct ifreq));
  strncpy(ifIndex.ifr_name, interface, IFNAMSIZ - 1);
  if (ioctl(sock, SIOCGIFINDEX, &ifIndex) < 0) perror("SIOCGIFINDEX");
  memset(&ifMAC, 0, sizeof(struct ifreq));
  strncpy(ifMAC.ifr_name, interface, IFNAMSIZ - 1);
  if (ioctl(sock, SIOCGIFHWADDR, &ifMAC) < 0) perror("SIOCGIFHWADDR");

  /* Initialize buffer */
  char sendBuf[pktSize];
  memset(sendBuf, 0, pktSize);

  /* Construct the Ethernet header */
  struct ether_header *eh = (struct ether_header *) sendBuf;
  eh->ether_shost[0] = ((uint8_t *)&ifMAC.ifr_hwaddr.sa_data)[0];
  eh->ether_shost[1] = ((uint8_t *)&ifMAC.ifr_hwaddr.sa_data)[1];
  eh->ether_shost[2] = ((uint8_t *)&ifMAC.ifr_hwaddr.sa_data)[2];
  eh->ether_shost[3] = ((uint8_t *)&ifMAC.ifr_hwaddr.sa_data)[3];
  eh->ether_shost[4] = ((uint8_t *)&ifMAC.ifr_hwaddr.sa_data)[4];
  eh->ether_shost[5] = ((uint8_t *)&ifMAC.ifr_hwaddr.sa_data)[5];
  eh->ether_dhost[0] = dstAddr[0];
  eh->ether_dhost[1] = dstAddr[1];
  eh->ether_dhost[2] = dstAddr[2];
  eh->ether_dhost[3] = dstAddr[3];
  eh->ether_dhost[4] = dstAddr[4];
  eh->ether_dhost[5] = dstAddr[5];
  eh->ether_type = htons(ETH_P_IP);

  /* Set packet payload */
  const size_t ehSize = sizeof(struct ether_header);
  const size_t plSize = pktSize - ehSize;
  unsigned int rbufSize = plSize - 4;
  if (!cMode) {
    rbufSize = getrandom(&sendBuf[ehSize], rbufSize, 0);
    if (rbufSize < 0) {
      perror("getrandom");
      exit(1);
    }
  } else {
    memset(&sendBuf[ehSize], (char) 0xCC, rbufSize);
  }
  sendBuf[pktSize - 4] = 0x23;
  sendBuf[pktSize - 3] = 0x50;
  sendBuf[pktSize - 2] = 0xde;
  sendBuf[pktSize - 1] = 0xe3;

  /* Display Injection Information */
  const double injectionRate = delay == 0 ? 0 : ((double) 1000000 / delay);
  printf("[Injection Information]\n");
  printf("Interface         : %s\n", interface);
  printf("Payload Generator : %s\n", cMode ? "0xCC" : "random");
  printf("Payload Length    : %u bytes\n", plSize);
  printf("Packet Length     : %zu bytes\n", pktSize);
  if (infCnt) {
    printf("Packet Count      : infinite pkts\n");
  } else {
    printf("Packet Count      : %llu pkts\n", cnt);
  }
  printf("Injection Delay   : %ld us\n", delay);
  if (delay == 0) {
    printf("Injection Rate    : unknown\n");
  } else {
    printf("Injection Rate    : %.5f pkts/s\n", injectionRate);
  }
  printf("Target            : %02x:%02x:%02x:%02x:%02x:%02x\n",
    dstAddr[0], dstAddr[1], dstAddr[2], dstAddr[3], dstAddr[4], dstAddr[5]);
  if (vMode) {
    printf("Packet Content    :");
    short bufi;
    for (bufi = 0; bufi < pktSize; bufi += 1) {
      printf(" %02X", sendBuf[bufi]);
    }
    printf("\n");
  }
  printf("\n");
  fflush(stdout);

  /* Set socket address */
  struct sockaddr_ll sockAddr;
  sockAddr.sll_ifindex = ifIndex.ifr_ifindex;
  sockAddr.sll_family = PF_PACKET;
  sockAddr.sll_protocol = htons(ETH_P_IP);
  sockAddr.sll_hatype = ARPHRD_ETHER;
  sockAddr.sll_pkttype = PACKET_OTHERHOST;
  sockAddr.sll_halen = ETH_ALEN;
  sockAddr.sll_addr[0] = dstAddr[0];
  sockAddr.sll_addr[1] = dstAddr[1];
  sockAddr.sll_addr[2] = dstAddr[2];
  sockAddr.sll_addr[3] = dstAddr[3];
  sockAddr.sll_addr[4] = dstAddr[4];
  sockAddr.sll_addr[5] = dstAddr[5];

  /* Set clock variables */
  struct timespec start, now;
  int32_t diff;
  if (delayExist) clock_gettime(CLOCK_MONOTONIC, &start);

  /* Send packets */
  unsigned long long i;
  const size_t saSize = sizeof(struct sockaddr_ll);
  const struct sockaddr* saPointer = (struct sockaddr*) &sockAddr;
  for (i = 0; infCnt || i < cnt; i += 1)
  {
    if (delayExist) {
      clock_gettime(CLOCK_MONOTONIC, &now);
      diff = delay * i - (
              (now.tv_sec - start.tv_sec) * 1000000
               + (now.tv_nsec - start.tv_nsec + 500) / 1000
             );
      if (diff > 0 && diff < delay) usleep(diff);
    }
    if (sendto(sock, sendBuf, pktSize, 0, saPointer, saSize) < 0)
    {
      printf("\n[%d] %s\n", errno, strerror(errno));
      printf("X");
    } else {
      printf(".");
    }
    if ((i + 1) % 1000 == 0) {
      printf(" %lluk\n", (i + 1) / 1000);
    }
    fflush(stdout);
  }

  printf("\n\nInjection Completed.\n");

  return 0;

}
