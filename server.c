#include "common.h"

#define MAXLEN 1024

int sockfd;

static int64_t nowMicros() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static void putU64(uint8_t* p, uint64_t v) {
  for (int i = 7; i >= 0; i--) {
    p[i] = (uint8_t)(v & 0xFF);
    v >>= 8;
  }
}

void runClkServer() {
  char buf[MAXLEN];
  struct sockaddr_in clientAddr;
  socklen_t clientLen = sizeof(clientAddr);

  while(1) {
    size_t n = recvfrom(sockfd, buf, sizeof(buf), 0,
        (struct sockaddr*) &clientAddr, &clientLen);

    if (n < 0) {
      perror("recvfrom");
      continue;
    }

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
    printf("request from %s: %d", ip, ntohs(clientAddr.sin_port));

    uint8_t reply[8];
    putU64(reply, (uint64_t)nowMicros());
    if (sendto(sockfd, reply, sizeof(reply), 0,
          (struct sockaddr*) &clientAddr, clientLen) < 0) {
      perror("sendto");
      exit(EXIT_FAILURE);
    }
  }
}

void initClkServer() {
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("sockfd");
    exit(EXIT_FAILURE);
  }
  printf("socket created successfully\n");
  
  int opt = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(PORT);
  if (bind(sockfd, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  runClkServer();
}

int main() {
  initClkServer();
  return 0;
}
