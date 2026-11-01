#include "common.h"
#include <sys/time.h>

#define PORT 8000
#define MAX_RETRIES 3

static int64_t nowMicros() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static uint64_t getU64(const uint8_t* p) {
  uint64_t v = 0;
  for (int i = 0; i < 8; i++) {
    v = (v << 8) | p[i];
  }
  return v;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <Server_IP>\n", argv[0]);
    return 1;
  }

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    return 1;
  }

  struct timeval tv;
  tv.tv_sec = 2;
  tv.tv_usec = 0;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  struct sockaddr_in servAddr = {0};
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(PORT);
  inet_pton(AF_INET, argv[1], &servAddr.sin_addr);

  uint8_t req = 0x01;
  uint8_t buf[8];
  socklen_t addrLen = sizeof(servAddr);
  
  int attempts = 0;
  int success = 0;
  int64_t t1, t4;

  printf("Attempting to synchronize time with %s...\n", argv[1]);

  while (attempts < MAX_RETRIES) {
    attempts++;
    t1 = nowMicros(); 
    
    if (sendto(sockfd, &req, sizeof(req), 0, (struct sockaddr*)&servAddr, addrLen) < 0) {
      perror("sendto error");
      continue; 
    }

    if (recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&servAddr, &addrLen) >= 0) {
      t4 = nowMicros(); 
      success = 1;
      break; 
    } else {
      printf("[Warning] Packet lost or timeout. Retrying (%d/%d)...\n", attempts, MAX_RETRIES);
    }
  }

  if (!success) {
    printf("[Error] Failed to synchronize after %d attempts. Server unreachable.\n", MAX_RETRIES);
    close(sockfd);
    return 1;
  }

  uint64_t t_server = getU64(buf);
  int64_t rtt = t4 - t1;
  int64_t offset = (int64_t)t_server + (rtt / 2) - t4;

  int64_t localTime = nowMicros();
  int64_t syncTime = localTime + offset;

  struct timeval newTime;
  newTime.tv_sec = syncTime / 1000000;
  newTime.tv_usec = syncTime % 1000000;
  
  if (settimeofday(&newTime, NULL) < 0)
    perror("failed to set time (run as root)");
  else
    printf("Time synchronized perfectly:PP\n");

  printf("[Client] Server Time: %lu us\n", t_server);
  printf("[Client] RTT: %ld us\n", rtt);
  printf("[Client] Calculated Offset: %ld us\n", offset);
  printf("[Client] Unsync'd time %ld us\n", localTime); 
  printf("[Client] Sync'd time: %ld us\n", syncTime);

  close(sockfd);
  return 0;
}
