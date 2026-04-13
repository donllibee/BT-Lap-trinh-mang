#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Sử dụng: %s <Port>\n", argv[0]);
        return 1;
    }

    int server = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 5);
    printf("Server đang lắng nghe trên cổng %s...\n", argv[1]);

    int client = accept(server, NULL, NULL);
    char buffer[4096];
    int n = recv(client, buffer, sizeof(buffer) - 1, 0);
    buffer[n] = '\0';

    // Tách dòng đầu tiên (Đường dẫn thư mục)
    char *line = strtok(buffer, "\n");
    if (line != NULL) {
        printf("Thư mục của Client: %s\n", line);
        printf("Danh sách tập tin:\n");

        // Tách các dòng tiếp theo (Thông tin file)
        while ((line = strtok(NULL, "\n")) != NULL) {
            // Thủ thuật: Tìm khoảng trắng CUỐI CÙNG (ngăn cách giữa tên và kích thước)
            char *last_space = strrchr(line, ' '); 
            if (last_space != NULL) {
                *last_space = '\0'; // Cắt chuỗi tại khoảng trắng cuối
                char *filename = line;
                long size = atol(last_space + 1); // Phần còn lại là kích thước
                printf("  + %s - %ld bytes\n", filename, size);
            }
        }
    }

    close(client);
    close(server);
    return 0;
}