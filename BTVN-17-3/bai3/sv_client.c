#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

struct sinh_vien{
    int MSSV;
    char ten[100];
    char ngay_sinh[40];
    float diem_trung_binh;
};

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Usage: %s <dia_chi_IP> <cong>\n", argv[0]);
        printf("Example: %s 127.0.0.1 8080\n", argv[0]);
        return 1;
    }

    char* sv_IP = argv[1];
    int sv_port = atoi(argv[2]);

    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(sv_IP);
    addr.sin_port = htons(sv_port);
    
    if (inet_pton(AF_INET, sv_IP, &addr.sin_addr) <= 0) {
        printf("Dia chi IP khong hop le: %s\n", sv_IP);
        close(client);
        return 1;
    }

    printf("Dang ket noi den %s:%d...\n", sv_IP, sv_port);
    if (connect(client, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect() failed");
        close(client);
        return 1;
    }
    printf("Ket noi thanh cong den %s:%d\n", sv_IP, sv_port);
    printf("Nhap du lieu de gui:\n");

    struct sinh_vien sv;

    while(1){
        printf("Nhap MSSV: \n");
        fflush(stdin); 
        scanf("%d", &sv.MSSV);
        printf("Nhap ten: \n");
        fflush(stdin);
        while ((getchar()) != '\n'); // Xoa bo nho dem
        fgets(sv.ten, 100, stdin);
        sv.ten[strcspn(sv.ten, "\n")] = 0; // Xoa ky tu xuong dong
        printf("Nhap ngay sinh (DD-MM-YYYY): \n");
        fflush(stdin); 
        scanf("%s", sv.ngay_sinh);
        printf("Nhap diem trung binh: \n");
        fflush(stdin); 
        scanf("%f", &sv.diem_trung_binh);

        ssize_t sent_bytes = send(client, &sv, sizeof(sv), 0);
        if (sent_bytes < 0) {
            perror("send() failed");
            break;
        }
    }

    close(client);
    return 0;
}