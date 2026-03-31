#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

// Định nghĩa cấu trúc sinh viên theo yêu cầu [cite: 13]
struct sinh_vien {
    int MSSV;
    char ten[100];
    char ngay_sinh[40];
    float diem_trung_binh;
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Sử dụng: %s <Địa chỉ IP> <Cổng>\n", argv[0]);
        return 1;
    }

    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Kết nối thất bại");
        return 1;
    }

    struct sinh_vien sv;
    printf("Nhập MSSV: "); scanf("%d", &sv.MSSV);
    getchar(); // Xóa bộ nhớ đệm
    printf("Nhập họ tên: "); fgets(sv.ten, sizeof(sv.ten), stdin);
    sv.ten[strcspn(sv.ten, "\n")] = 0; // Xóa ký tự xuống dòng
    printf("Nhập ngày sinh (YYYY-MM-DD): "); scanf("%s", sv.ngay_sinh);
    printf("Nhập điểm trung bình: "); scanf("%f", &sv.diem_trung_binh);

    // Gửi cấu trúc dữ liệu sang server [cite: 14]
    send(client, &sv, sizeof(sv), 0);

    close(client);
    return 0;
}