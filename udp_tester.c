#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define SERVER_IP "127.0.0.1"
#define MSG_LEN 32 * 1024 + 1

#pragma pack(1)
struct Message {
    int messageType;
    int messageLength;
    char message[MSG_LEN];
};
#pragma pack()

double perform_interaction(int tcp_port, int udp_message_size) {
    int received_udp_port = 0;
    
    // --- TCP Phase ---
    {
        int sock = 0;
        struct sockaddr_in serv_addr;
        
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { return -1; }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(tcp_port);
        inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { close(sock); return -1; }
        
        struct Message request_msg;
        request_msg.messageType = 1;
        strcpy(request_msg.message, "Requesting UDP port");
        request_msg.messageLength = strlen(request_msg.message);
        send(sock, &request_msg, sizeof(request_msg), 0);

        struct Message response_msg;
        read(sock, &response_msg, sizeof(response_msg));
        received_udp_port = atoi(response_msg.message);
        close(sock);
    }

    // --- UDP Phase ---
    if (received_udp_port > 0) {
        int udp_socket;
        struct sockaddr_in udp_server_addr;
        struct timeval start_time, end_time;

        if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { return -1; }

        memset(&udp_server_addr, 0, sizeof(udp_server_addr));
        udp_server_addr.sin_family = AF_INET;
        udp_server_addr.sin_port = htons(received_udp_port);
        udp_server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

        struct Message udp_data_msg;
        udp_data_msg.messageType = 3;
        memset(udp_data_msg.message, 'A', udp_message_size);
        udp_data_msg.message[udp_message_size] = '\0';
        udp_data_msg.messageLength = udp_message_size;

        gettimeofday(&start_time, NULL);
        sendto(udp_socket, &udp_data_msg, sizeof(udp_data_msg), 0, (const struct sockaddr *) &udp_server_addr, sizeof(udp_server_addr));
        struct Message udp_response_msg;
        recvfrom(udp_socket, &udp_response_msg, sizeof(udp_response_msg), 0, NULL, NULL);
        gettimeofday(&end_time, NULL);
        
        close(udp_socket);

        long elapsed_microseconds = (end_time.tv_sec - start_time.tv_sec) * 1000000L + (end_time.tv_usec - start_time.tv_usec);
        if (elapsed_microseconds == 0) return 0;
        double elapsed_seconds = elapsed_microseconds / 1000000.0;
        double throughput_mbps = (double)(udp_message_size * 8) / (elapsed_seconds * 1000000.0);
        return throughput_mbps;
    }
    return -1;
}

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server Port> <Mode: single|test>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int tcp_port = atoi(argv[1]);
    const char *mode = argv[2];

    if (strcmp(mode, "single") == 0) {
        perform_interaction(tcp_port, 1024);
    } else if (strcmp(mode, "test") == 0) {
        printf("Size(Bytes),Throughput(Mbps)\n");
        for (int size = 1024; size <= 32 * 1024; size += 1024) {
            double throughput = perform_interaction(tcp_port, size);
            if (throughput >= 0) {
                printf("%d,%.2f\n", size, throughput);
            }
        }
    }
    return 0;
}
