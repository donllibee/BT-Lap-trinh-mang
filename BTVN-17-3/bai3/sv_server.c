#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

struct sinh_vien{
    int MSSV;
    char ten[100];
    char ngay_sinh[40];
    float diem_trung_binh;
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <Địa chỉ IP> <Cổng>\n", argv[0]);
        return 1;
    }
    char* sv_IP = argv[1];
    int port = atoi(argv[2]);

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(inet_addr(sv_IP));
    addr.sin_port = htons(port);

    int opt = 1; 
    // Thiết lập SO_REUSEADDR để giải phóng cổng ngay khi server tắt 
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { 
        perror("setsockopt failed"); 
        exit(EXIT_FAILURE); 
    }

    int ret = bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("bind() failed");
        exit(1);
    }

    ret = listen(listener, 5);
    if (ret < 0) {
        perror("listen() failed");
        exit(1);
    }

    printf("Waiting for client\n");
    
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    
    int client = accept(listener, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client < 0) {
        perror("accept() failed");
        exit(1);
    }

    printf("Client connected: %d\n", client);
    printf("IP: %s\n", inet_ntoa(client_addr.sin_addr));
    printf("Port: %d\n", ntohs(client_addr.sin_port));

    // Mở tệp tin lưu nội dung client gửi đến
    char write_path[] = "/mnt/c/Users/ACER/test/";
    strcat(write_path, argv[2]);
    FILE *log_file = fopen(argv[2], "w");
    if (log_file == NULL) {
        printf("Failed to open log file %s", argv[2]);
        return 1;
    }
    
    struct sinh_vien sv;

    while (1) {

        int n = recv(client, &sv, sizeof(sv), 0);
        
        if (n <= 0) {
            printf("Disconnected\n");
            break;
        }

        // fprintf(log_file, "%d %s %s %.2f\n", sv.MSSV, sv.ten, sv.ngay_sinh, sv.diem_trung_binh);
        printf("Thong tin sinh vien: %d %s %s %.2f\n", sv.MSSV, sv.ten, sv.ngay_sinh, sv.diem_trung_binh);
    }

    fclose(log_file);

    close(client);
    close(listener);
}