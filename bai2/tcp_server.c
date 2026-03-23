#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <Port> <Greeting_File> <Log_File>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5) < 0) {
        perror("listen() failed");
        return 1;
    }

    printf("Waiting for client on port %d...\n", port);
    
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client = accept(listener, (struct sockaddr *)&client_addr, &client_addr_len);
    
    if (client < 0) {
        perror("accept() failed");
        return 1;
    }

    printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    FILE *greeting_file = fopen(argv[2], "r");
    if (greeting_file == NULL) {
        perror("Failed to open greeting file");
        close(client); close(listener);
        return 1;
    }
    
    char greeting[1024];
    // Đọc toàn bộ nội dung file chào
    size_t n_read = fread(greeting, 1, sizeof(greeting) - 1, greeting_file);
    greeting[n_read] = '\0';
    fclose(greeting_file);

    send(client, greeting, strlen(greeting), 0);

    FILE *log_file = fopen(argv[3], "w");
    if (log_file == NULL) {
        perror("Failed to open log file");
        close(client); close(listener);
        return 1;
    }

    char buf[256];
    while (1) {
        int n = recv(client, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            printf("Client disconnected.\n");
            break;
        }

        buf[n] = '\0';
        fputs(buf, log_file);
        fflush(log_file);
        printf("Received: %s", buf);
    }

    fclose(log_file);
    close(client);
    close(listener);
    return 0;
}