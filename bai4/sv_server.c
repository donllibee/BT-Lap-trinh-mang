#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

struct sinh_vien {
    int MSSV;
    char ten[100];
    char ngay_sinh[40];
    float diem_trung_binh;
};

int main(int argc, char *argv[]) {
    // Tham số: <Cổng> <Tên file log> 
    if (argc != 3) {
        printf("Sử dụng: %s <Cổng> <Tên file log>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    char *log_filename = argv[2];

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Lỗi bind()");
        return 1;
    }

    listen(listener, 5);
    printf("Server đang đợi kết nối ở cổng %d...\n", port);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client = accept(listener, (struct sockaddr *)&client_addr, &client_addr_len);

    // Mở file log để ghi tiếp (append) [cite: 16]
    FILE *f = fopen(log_filename, "a");
    if (f == NULL) {
        perror("Không thể mở file log");
        return 1;
    }

    struct sinh_vien sv;
    if (recv(client, &sv, sizeof(sv), 0) > 0) {
        // Lấy thời gian hiện tại [cite: 17]
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        char s_time[32];
        strftime(s_time, sizeof(s_time), "%Y-%m-%d %H:%M:%S", &tm);

        // Định dạng ghi log: IP Thời-gian MSSV Họ-tên Ngày-sinh Điểm 
        char log_line[512];
        sprintf(log_line, "%s %s %d %s %s %.2f\n", 
                inet_ntoa(client_addr.sin_addr), s_time, 
                sv.MSSV, sv.ten, sv.ngay_sinh, sv.diem_trung_binh);

        // In ra màn hình và ghi vào file [cite: 16]
        printf("%s", log_line);
        fputs(log_line, f);
    }

    fclose(f);
    close(client);
    close(listener);
    return 0;
}