#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int config_livewire_socket(char *multicastAddr)
{
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  int reuse = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) == -1) {
      fprintf(stderr, "setsockopt: %d\n", errno);
      return -1;
  }
  
  struct sockaddr_in addr; 
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(5004);
  addr.sin_addr.s_addr = inet_addr(multicastAddr);

  struct ip_mreq mreq;
  mreq.imr_multiaddr.s_addr = inet_addr(multicastAddr);         
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);

  if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) == -1) {
      fprintf(stderr, "setsockopt: %d\n", errno);
      return -1;
  }

  if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
      fprintf(stderr, "bind: %d\n", errno);
      return -1;
  }
  return sock;
}

char* lw_mc_addr(int channelNumber)
{
  struct in_addr addr;
  addr.s_addr = htonl(0xEFC00000 + channelNumber);
  char *s = inet_ntoa(addr);
  return s;
}

int main(int argc, const char *argv[]) {
  if (argc != 4) {
    printf("Usage: axiadump <LW channel> <file.out> <Duration sec.>\n");
    return 1;
  }

  int axiaChannel = atoi(argv[1]);
  const char *outputFile = argv[2];
  int32_t duration = 48000 * atoi(argv[3]);
  uint32_t frameCounter = 0;
  int packetLength;
  uint8_t packet[1452];
  int sock = config_livewire_socket(lw_mc_addr(axiaChannel));
  FILE *file = fopen(outputFile, "wb");
  
  printf("Writing Livewire source %i to %s\nDuration: %i sec.\n", axiaChannel, outputFile, duration / 48000);

  while (frameCounter < duration) {
    packetLength = recv(sock, packet, sizeof(packet), 0);
    fwrite(packet + 12, sizeof(packet[0]), packetLength - 12, file);
    frameCounter += ((packetLength - 12) / 6);
  }

  printf("Done!\n");
  fclose(file);
  return 0;
}
