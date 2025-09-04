#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define SERVER_IP "127.0.0.1"

int main(int argc, char const *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <Port> <Total Size(MB)> <Chunk Size(KB)>\n", argv[0]);
        return -1;
    }
    int port = atoi(argv[1]);
    long total_size_mb = atol(argv[2]);
    int chunk_size_kb = atoi(argv[3]);

    long total_bytes = total_size_mb * 1024 * 1024;
    int chunk_bytes = chunk_size_kb * 1024;
    char *buffer = malloc(chunk_bytes);
    memset(buffer, 'A', chunk_bytes);

    int sock = 0;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { exit(EXIT_FAILURE); }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        free(buffer);
        close(sock);
        return -1;
    }

    const char* trigger_msg = "tcp_bulk_task";
    send(sock, trigger_msg, strlen(trigger_msg), 0);
    
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    
    long bytes_sent = 0;
    while (bytes_sent < total_bytes) {
        if (send(sock, buffer, chunk_bytes, 0) < 0) {
            perror("send failed");
            break;
        }
        bytes_sent += chunk_bytes;
    }

    gettimeofday(&end_time, NULL);
    close(sock);
    free(buffer);

    long elapsed_microseconds = (end_time.tv_sec - start_time.tv_sec) * 1000000L + (end_time.tv_usec - start_time.tv_usec);
    double elapsed_seconds = elapsed_microseconds / 1000000.0;
    double throughput_mbps = 0;
    if (elapsed_seconds > 0) {
        throughput_mbps = (double)(total_bytes * 8) / (elapsed_seconds * 1000000.0);
    }

    printf("TotalData(MB),ChunkSize(KB),Throughput(Mbps)\n");
    printf("%ld,%d,%.2f\n", total_size_mb, chunk_size_kb, throughput_mbps);

    return 0;
}
