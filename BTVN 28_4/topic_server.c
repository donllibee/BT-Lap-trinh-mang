#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <ctype.h>

#define MAX_CLIENTS 64
#define BUFFER_SIZE 1024
#define PORT 9000

typedef struct {
    int fd;
    char topics[5][50];
    int topic_count;
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;

// Tìm client theo file descriptor
int find_client(int fd) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i].fd == fd) return i;
    }
    return -1;
}

// Thêm topic cho client
int add_topic(int client_idx, const char *topic) {
    if (clients[client_idx].topic_count >= 5) return -1;
    
    // Kiểm tra topic đã tồn tại chưa
    for (int i = 0; i < clients[client_idx].topic_count; i++) {
        if (strcmp(clients[client_idx].topics[i], topic) == 0) {
            return 0;  // Đã subscribe rồi
        }
    }
    
    strcpy(clients[client_idx].topics[clients[client_idx].topic_count], topic);
    clients[client_idx].topic_count++;
    return 1;
}

// Kiểm tra client có subscribe topic không
int is_subscribed(int client_idx, const char *topic) {
    for (int i = 0; i < clients[client_idx].topic_count; i++) {
        if (strcmp(clients[client_idx].topics[i], topic) == 0) {
            return 1;
        }
    }
    return 0;
}

// Gửi tin nhắn đến tất cả client subscribe topic
void publish_message(const char *topic, const char *msg, int sender_fd) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "[%s] %s\n", topic, msg);
    
    for (int i = 0; i < client_count; i++) {
        // Không gửi lại cho chính client đã PUB
        if (clients[i].fd == sender_fd) continue;
        
        if (is_subscribed(i, topic)) {
            send(clients[i].fd, buffer, strlen(buffer), 0);
        }
    }
}

// Xóa client khỏi danh sách
void remove_client(struct pollfd fds[], int *nfds, int index) {
    close(clients[index].fd);
    
    // Xóa khỏi mảng clients
    for (int i = index; i < client_count - 1; i++) {
        clients[i] = clients[i + 1];
    }
    client_count--;
    
    // Xóa khỏi mảng pollfd
    for (int i = index + 1; i < *nfds; i++) {
        fds[i - 1] = fds[i];
    }
    (*nfds)--;
}

// Xử lý lệnh từ client
void process_command(int client_fd, const char *cmd) {
    char command[10], topic[50], msg[BUFFER_SIZE];
    int client_idx = find_client(client_fd);
    
    if (client_idx == -1) return;
    
    // Lệnh SUB
    if (sscanf(cmd, "SUB %49s", topic) == 1) {
        if (add_topic(client_idx, topic) == 1) {
            char reply[BUFFER_SIZE];
            snprintf(reply, sizeof(reply), "Da SUB topic '%s' thanh cong!\n", topic);
            send(client_fd, reply, strlen(reply), 0);
        } else {
            send(client_fd, "SUB that bai (da SUB topic nay hoac qua 5 topic)!\n", 52, 0);
        }
    }
    // Lệnh PUB
    else if (sscanf(cmd, "PUB %49s %[^\n]", topic, msg) == 2) {
        char reply[BUFFER_SIZE];
        snprintf(reply, sizeof(reply), "Da PUB tin nhan den topic '%s'\n", topic);
        send(client_fd, reply, strlen(reply), 0);
        
        // Chuyển tiếp tin nhắn cho các client đã SUB
        publish_message(topic, msg, client_fd);
    }
    else {
        send(client_fd, "Lenh khong hop le! Dung SUB <topic> hoac PUB <topic> <msg>\n", 58, 0);
    }
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0) {
        perror("socket() failed");
        return 1;
    }
    
    // Thiết lập tùy chọn reuse
    int reuse = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);
    
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind() failed");
        close(listener);
        return 1;
    }
    
    if (listen(listener, 5) < 0) {
        perror("listen() failed");
        close(listener);
        return 1;
    }
    
    printf("Server dang chay tren cong %d...\n", PORT);
    printf("Ho tro cac lenh:\n");
    printf("  SUB <topic>  - Dang ky nhan tin nhan cho topic\n");
    printf("  PUB <topic> <msg> - Gui tin nhan den topic\n\n");
    
    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;
    fds[0].fd = listener;
    fds[0].events = POLLIN;
    
    char buffer[BUFFER_SIZE];
    
    while (1) {
        int ret = poll(fds, nfds, -1);
        if (ret < 0) {
            perror("poll() failed");
            break;
        }
        
        // Chấp nhận kết nối mới
        if (fds[0].revents & POLLIN) {
            int new_client = accept(listener, NULL, NULL);
            if (new_client < 0) {
                perror("accept() failed");
                continue;
            }
            
            if (client_count >= MAX_CLIENTS) {
                char *msg = "Server da day! Khong the ket noi.\n";
                send(new_client, msg, strlen(msg), 0);
                close(new_client);
                continue;
            }
            
            // Thêm client mới
            clients[client_count].fd = new_client;
            clients[client_count].topic_count = 0;
            fds[nfds].fd = new_client;
            fds[nfds].events = POLLIN;
            nfds++;
            client_count++;
            
            char *welcome = "Ket noi thanh cong!\nCac lenh: SUB <topic>, PUB <topic> <msg>\n";
            send(new_client, welcome, strlen(welcome), 0);
            printf("Client moi ket noi! So client hien tai: %d\n", client_count);
        }
        
        // Xử lý dữ liệu từ các client
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                int bytes = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);
                
                if (bytes <= 0) {
                    // Client ngắt kết nối
                    printf("Client %d ngat ket noi\n", fds[i].fd);
                    int idx = find_client(fds[i].fd);
                    if (idx >= 0) {
                        remove_client(fds, &nfds, idx);
                        i--;  // Điều chỉnh chỉ số sau khi xóa
                    }
                } else {
                    buffer[bytes] = '\0';
                    // Xóa ký tự xuống dòng
                    buffer[strcspn(buffer, "\r\n")] = 0;
                    
                    printf("Nhan tu client %d: %s\n", fds[i].fd, buffer);
                    process_command(fds[i].fd, buffer);
                }
            }
        }
    }
    
    close(listener);
    return 0;
}