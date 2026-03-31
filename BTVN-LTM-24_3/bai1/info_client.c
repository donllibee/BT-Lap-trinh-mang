#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Sử dụng: %s <IP> <Port>\n", argv[0]);
        return 1;
    }

    int client = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(client, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Kết nối thất bại");
        return 1;
    }

    // 1. Lấy đường dẫn thư mục hiện tại
    char data[4096] = ""; 
    getcwd(data, sizeof(data));
    strcat(data, "\n"); // Dòng đầu tiên luôn là đường dẫn thư mục

    // 2. Đọc danh sách file
    DIR *dir = opendir(".");
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // Chỉ lấy file thông thường
            struct stat st;
            if (stat(entry->d_name, &st) == 0) {
                char temp[512];
                // Định dạng: tên_file_có_khoảng_trắng kích_thước\n
                sprintf(temp, "%s %ld\n", entry->d_name, st.st_size);
                strcat(data, temp);
            }
        }
    }
    closedir(dir);

    // 3. Gửi toàn bộ khối dữ liệu một lần duy nhất để tối ưu
    send(client, data, strlen(data), 0);
    printf("Đã gửi thông tin thư mục và danh sách file!\n");

    close(client);
    return 0;
}