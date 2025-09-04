// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <pthread.h>

// #define MSG_LEN 1024
// #define MAX_QUEUE_SIZE 100
// #define NUM_WORKER_THREADS 4
// #define RR_TIME_SLICE 1 // 1 second

// // --- Message Struct (for original protocol) ---
// #pragma pack(1)
// struct Message {
//     int messageType;
//     int messageLength;
//     char message[MSG_LEN];
// };
// #pragma pack()

// // --- Unified Request Structure ---
// typedef struct {
//     int client_sock;
//     char task_type[MSG_LEN]; // "udp_protocol", "tcp_bulk_task", "long_task", etc.
//     int time_remaining;      // Only for scheduler tasks
// } client_request;

// // --- Shared Globals ---
// client_request request_queue[MAX_QUEUE_SIZE];
// int queue_count = 0;
// int queue_front = 0;
// int queue_rear = 0;
// char scheduling_policy[5];
// pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;


// // --- Task-specific Logic ---

// // Function to handle the original TCP/UDP protocol
// void handle_udp_protocol(int client_sock, const char* task_name) {
//     printf("SERVER: Worker handling original protocol request.\n");
//     int udp_socket;
//     struct sockaddr_in udp_server_addr;
//     int assigned_udp_port = 0;

//     if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { perror("UDP socket failed"); close(client_sock); return; }
    
//     memset(&udp_server_addr, 0, sizeof(udp_server_addr));
//     udp_server_addr.sin_family = AF_INET;
//     udp_server_addr.sin_addr.s_addr = INADDR_ANY;
//     udp_server_addr.sin_port = htons(0); // Bind to port 0

//     if (bind(udp_socket, (const struct sockaddr *)&udp_server_addr, sizeof(udp_server_addr)) < 0) { perror("UDP bind failed"); close(udp_socket); close(client_sock); return; }

//     socklen_t len = sizeof(udp_server_addr);
//     if (getsockname(udp_socket, (struct sockaddr *)&udp_server_addr, &len) != -1) {
//         assigned_udp_port = ntohs(udp_server_addr.sin_port);
//     } else { perror("getsockname failed"); close(udp_socket); close(client_sock); return; }

//     struct Message response_msg;
//     response_msg.messageType = 2;
//     sprintf(response_msg.message, "%d", assigned_udp_port);
//     response_msg.messageLength = strlen(response_msg.message);
//     send(client_sock, &response_msg, sizeof(response_msg), 0);
//     close(client_sock); // TCP part is done

//     // --- UDP Phase ---
//     struct Message udp_received_msg;
//     struct sockaddr_in udp_client_addr;
//     socklen_t client_len = sizeof(udp_client_addr);
//     recvfrom(udp_socket, &udp_received_msg, sizeof(udp_received_msg), 0, (struct sockaddr *) &udp_client_addr, &client_len);
//     printf("[Thread %lu]: Received UDP Message: '%s'\n", pthread_self(), udp_received_msg.message);
    
//     struct Message udp_response_msg;
//     udp_response_msg.messageType = 4;
//     strcpy(udp_response_msg.message, "Server acknowledges UDP data");
//     udp_response_msg.messageLength = strlen(udp_response_msg.message);
//     sendto(udp_socket, &udp_response_msg, sizeof(udp_response_msg), 0, (const struct sockaddr *) &udp_client_addr, client_len);
//     close(udp_socket);
//     printf("[Thread %lu]: FINISHED job '%s'.\n\n", pthread_self(), task_name);
// }

// // --- Main Worker Thread Function ---
// void *worker_thread_function(void *arg) {
//     while (1) {
//         pthread_mutex_lock(&queue_mutex);
//         while (queue_count == 0) {
//             pthread_cond_wait(&queue_cond, &queue_mutex);
//         }
//         client_request req = request_queue[queue_front];
//         queue_front = (queue_front + 1) % MAX_QUEUE_SIZE;
//         queue_count--;
//         pthread_mutex_unlock(&queue_mutex);

//         printf("[Thread %lu]: Picked up job '%s'.\n", pthread_self(), req.task_type);

//         // --- UNIFIED LOGIC ROUTER ---
//         if (strcmp(req.task_type, "udp_protocol") == 0) {
//             handle_udp_protocol(req.client_sock, req.task_type);
//         } else if (strcmp(req.task_type, "tcp_bulk_task") == 0) {
//             char buffer[4096];
//             while (read(req.client_sock, buffer, sizeof(buffer)) > 0) {} // Consume bulk data
//             printf("[Thread %lu]: FINISHED job '%s'.\n\n", pthread_self(), req.task_type);
//             close(req.client_sock);
//         } else if (strcmp(scheduling_policy, "FCFS") == 0) {
//             sleep(req.time_remaining);
//             printf("[Thread %lu]: FINISHED FCFS job '%s' after %d seconds.\n\n", pthread_self(), req.task_type, req.time_remaining);
//             close(req.client_sock);
//         } else if (strcmp(scheduling_policy, "RR") == 0) {
//             sleep(RR_TIME_SLICE);
//             req.time_remaining -= RR_TIME_SLICE;
//             if (req.time_remaining > 0) {
//                 printf("[Thread %lu]: PREEMPTED '%s'. %d secs left. Re-queuing.\n", pthread_self(), req.task_type, req.time_remaining);
//                 pthread_mutex_lock(&queue_mutex);
//                 request_queue[queue_rear] = req;
//                 queue_rear = (queue_rear + 1) % MAX_QUEUE_SIZE;
//                 queue_count++;
//                 pthread_cond_signal(&queue_cond);
//                 pthread_mutex_unlock(&queue_mutex);
//             } else {
//                 printf("[Thread %lu]: FINISHED RR job '%s'.\n\n", pthread_self(), req.task_type);
//                 close(req.client_sock);
//             }
//         }
//     }
//     return NULL;
// }

// // --- Main Server Function ---
// int main(int argc, char const *argv[]) {
//     if (argc != 3) {
//         fprintf(stderr, "Usage: %s <Server Port> <Policy: FCFS|RR>\n", argv[0]);
//         exit(EXIT_FAILURE);
//     }
//     int tcp_port = atoi(argv[1]);
//     strcpy(scheduling_policy, argv[2]);

//     int server_fd, new_socket;
//     struct sockaddr_in address;
//     int addrlen = sizeof(address);
//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { exit(EXIT_FAILURE); }
//     int opt = 1;
//     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) { exit(EXIT_FAILURE); }
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(tcp_port);
//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) { perror("bind"); exit(EXIT_FAILURE); }
//     if (listen(server_fd, 10) < 0) { exit(EXIT_FAILURE); }

//     pthread_t worker_threads[NUM_WORKER_THREADS];
//     for (int i = 0; i < NUM_WORKER_THREADS; i++) {
//         pthread_create(&worker_threads[i], NULL, worker_thread_function, NULL);
//     }
//     printf("Unified Server starting with policy: %s\n", scheduling_policy);
//     printf("%d worker threads created.\nServer is listening on port %d...\n\n", NUM_WORKER_THREADS, tcp_port);

//     // Main Producer Loop
//     while (1) {
//         new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        
//         char buffer[MSG_LEN] = {0};
//         // Use recv with PEEK to inspect the message without removing it from the socket buffer
//         int bytes_peeked = recv(new_socket, buffer, MSG_LEN - 1, MSG_PEEK);

//         client_request new_req;
//         new_req.client_sock = new_socket;
        
//         // Decide task type based on the peeked message
//         struct Message* msg_ptr = (struct Message*)buffer;
//         if (bytes_peeked > 0 && msg_ptr->messageType == 1) {
//             strcpy(new_req.task_type, "udp_protocol");
//         } else {
//              // For string-based messages, read the actual message
//             read(new_socket, buffer, MSG_LEN - 1);
//             strcpy(new_req.task_type, buffer);
//         }

//         if (strcmp(new_req.task_type, "long_task") == 0) {
//             new_req.time_remaining = 5;
//         } else if (strcmp(new_req.task_type, "short_task_A") == 0 ||
//                    strcmp(new_req.task_type, "short_task_B") == 0 ||
//                    strcmp(new_req.task_type, "short_task_C") == 0) {
//             new_req.time_remaining = 1;
//         } else {
//              new_req.time_remaining = 0; // Not a timed task
//         }

//         pthread_mutex_lock(&queue_mutex);
//         request_queue[queue_rear] = new_req;
//         queue_rear = (queue_rear + 1) % MAX_QUEUE_SIZE;
//         queue_count++;
//         printf("Main thread enqueued new job: '%s'.\n", new_req.task_type);
//         pthread_cond_signal(&queue_cond);
//         pthread_mutex_unlock(&queue_mutex);
//     }
//     return 0;
// }
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MSG_LEN 1024
#define MAX_QUEUE_SIZE 100
#define NUM_WORKER_THREADS 4
#define RR_TIME_SLICE 1 // 1 second

#pragma pack(1)
struct Message {
    int messageType;
    int messageLength;
    char message[MSG_LEN];
};
#pragma pack()

typedef struct {
    int client_sock;
    char task_type[MSG_LEN];
    int time_remaining;
} client_request;

client_request request_queue[MAX_QUEUE_SIZE];
int queue_count = 0;
int queue_front = 0;
int queue_rear = 0;
char scheduling_policy[5];
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

void handle_udp_protocol(int client_sock, const char* task_name) {
    // First, consume the Type 1 message from the socket buffer
    struct Message received_msg;
    read(client_sock, &received_msg, sizeof(received_msg));

    int udp_socket;
    struct sockaddr_in udp_server_addr;
    int assigned_udp_port = 0;

    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { perror("UDP socket failed"); close(client_sock); return; }
    
    memset(&udp_server_addr, 0, sizeof(udp_server_addr));
    udp_server_addr.sin_family = AF_INET;
    udp_server_addr.sin_addr.s_addr = INADDR_ANY;
    udp_server_addr.sin_port = htons(0);

    if (bind(udp_socket, (const struct sockaddr *)&udp_server_addr, sizeof(udp_server_addr)) < 0) { perror("UDP bind failed"); close(udp_socket); close(client_sock); return; }

    socklen_t len = sizeof(udp_server_addr);
    if (getsockname(udp_socket, (struct sockaddr *)&udp_server_addr, &len) != -1) {
        assigned_udp_port = ntohs(udp_server_addr.sin_port);
    } else { perror("getsockname failed"); close(udp_socket); close(client_sock); return; }

    struct Message response_msg;
    response_msg.messageType = 2;
    sprintf(response_msg.message, "%d", assigned_udp_port);
    response_msg.messageLength = strlen(response_msg.message);
    send(client_sock, &response_msg, sizeof(response_msg), 0);
    close(client_sock);

    struct Message udp_received_msg;
    struct sockaddr_in udp_client_addr;
    socklen_t client_len = sizeof(udp_client_addr);
    recvfrom(udp_socket, &udp_received_msg, sizeof(udp_received_msg), 0, (struct sockaddr *) &udp_client_addr, &client_len);
    printf("[Thread %lu]: Received UDP Message with payload of %d bytes.\n", pthread_self(), udp_received_msg.messageLength);
    
    struct Message udp_response_msg;
    udp_response_msg.messageType = 4;
    strcpy(udp_response_msg.message, "Server acknowledges UDP data");
    udp_response_msg.messageLength = strlen(udp_response_msg.message);
    sendto(udp_socket, &udp_response_msg, sizeof(udp_response_msg), 0, (const struct sockaddr *) &udp_client_addr, client_len);
    close(udp_socket);
    printf("[Thread %lu]: FINISHED job '%s'.\n\n", pthread_self(), task_name);
}

void *worker_thread_function(void *arg) {
    while (1) {
        pthread_mutex_lock(&queue_mutex);
        while (queue_count == 0) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        client_request req = request_queue[queue_front];
        queue_front = (queue_front + 1) % MAX_QUEUE_SIZE;
        queue_count--;
        pthread_mutex_unlock(&queue_mutex);

        printf("[Thread %lu]: Picked up job '%s'.\n", pthread_self(), req.task_type);

        if (strcmp(req.task_type, "udp_protocol") == 0) {
            handle_udp_protocol(req.client_sock, req.task_type);
        } else if (strcmp(req.task_type, "tcp_bulk_task") == 0) {
            char identifier_buffer[100];
            read(req.client_sock, identifier_buffer, sizeof(identifier_buffer)); // Consume the identifier string
            char data_buffer[4096];
            while (read(req.client_sock, data_buffer, sizeof(data_buffer)) > 0) {}
            printf("[Thread %lu]: FINISHED job '%s'.\n\n", pthread_self(), req.task_type);
            close(req.client_sock);
        } else if (strcmp(scheduling_policy, "FCFS") == 0) {
            read(req.client_sock, NULL, 0); // Consume data from socket buffer
            sleep(req.time_remaining);
            printf("[Thread %lu]: FINISHED FCFS job '%s' after %d seconds.\n\n", pthread_self(), req.task_type, req.time_remaining);
            close(req.client_sock);
        } else if (strcmp(scheduling_policy, "RR") == 0) {
            read(req.client_sock, NULL, 0); // Consume data from socket buffer
            sleep(RR_TIME_SLICE);
            req.time_remaining -= RR_TIME_SLICE;
            if (req.time_remaining > 0) {
                printf("[Thread %lu]: PREEMPTED '%s'. %d secs left. Re-queuing.\n", pthread_self(), req.task_type, req.time_remaining);
                pthread_mutex_lock(&queue_mutex);
                request_queue[queue_rear] = req;
                queue_rear = (queue_rear + 1) % MAX_QUEUE_SIZE;
                queue_count++;
                pthread_cond_signal(&queue_cond);
                pthread_mutex_unlock(&queue_mutex);
            } else {
                printf("[Thread %lu]: FINISHED RR job '%s'.\n\n", pthread_self(), req.task_type);
                close(req.client_sock);
            }
        }
    }
    return NULL;
}

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server Port> <Policy: FCFS|RR>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int tcp_port = atoi(argv[1]);
    strcpy(scheduling_policy, argv[2]);

    int server_fd;
    struct sockaddr_in address;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { exit(EXIT_FAILURE); }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) { exit(EXIT_FAILURE); }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(tcp_port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) { perror("bind"); exit(EXIT_FAILURE); }
    if (listen(server_fd, 10) < 0) { exit(EXIT_FAILURE); }

    pthread_t worker_threads[NUM_WORKER_THREADS];
    for (int i = 0; i < NUM_WORKER_THREADS; i++) {
        pthread_create(&worker_threads[i], NULL, worker_thread_function, NULL);
    }
    printf("Unified Server starting with policy: %s\n", scheduling_policy);
    printf("%d worker threads created.\nServer is listening on port %d...\n\n", NUM_WORKER_THREADS, tcp_port);

    while (1) {
        int new_socket = accept(server_fd, NULL, NULL);
        
        char buffer[MSG_LEN] = {0};
        recv(new_socket, buffer, MSG_LEN - 1, MSG_PEEK);

        client_request new_req;
        new_req.client_sock = new_socket;
        
        struct Message* msg_ptr = (struct Message*)buffer;
        if (msg_ptr->messageType == 1) {
            strcpy(new_req.task_type, "udp_protocol");
        } else if (strstr(buffer, "tcp_bulk_task") == buffer) {
             strcpy(new_req.task_type, "tcp_bulk_task");
        } else if (strstr(buffer, "long_task") == buffer) {
            strcpy(new_req.task_type, "long_task");
        } else if (strstr(buffer, "short_task") == buffer) {
            // Read the full short task name to differentiate them
            char temp_buffer[100];
            read(new_socket, temp_buffer, sizeof(temp_buffer)-1);
            strcpy(new_req.task_type, temp_buffer);
        } else {
             strcpy(new_req.task_type, "unknown_task");
        }

        if (strcmp(new_req.task_type, "long_task") == 0) {
            new_req.time_remaining = 5;
        } else if (strstr(new_req.task_type, "short_task") == new_req.task_type) {
            new_req.time_remaining = 1;
        } else {
             new_req.time_remaining = 0;
        }

        pthread_mutex_lock(&queue_mutex);
        request_queue[queue_rear] = new_req;
        queue_rear = (queue_rear + 1) % MAX_QUEUE_SIZE;
        queue_count++;
        printf("Main thread enqueued new job: '%s'.\n", new_req.task_type);
        pthread_cond_signal(&queue_cond);
        pthread_mutex_unlock(&queue_mutex);
    }
    return 0;
}

