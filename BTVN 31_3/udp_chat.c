#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    // Xử lý tham số dòng lệnh
    if (argc != 4) {
        printf("Usage: %s <Cổng nhận dữ liệu> <Địa chỉ IP cổng cần kết nối> <Cổng cần kết nối>\n", argv[0]);
        return 1;
    }

    char* recv_port_str = argv[1];
    char* send_ip = argv[2];
    char* send_port_str = argv[3];

    // Tạo socket UDP để nhận tin nhắn
    int recv_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    unsigned long ul = 1;
    ioctl(recv_socket, FIONBIO, &ul);
    
    struct sockaddr_in recv_addr;
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    recv_addr.sin_port = htons(atoi(recv_port_str));

    bind(recv_socket, (struct sockaddr *)&recv_addr, sizeof(recv_addr));
    printf("UDP server listening on port %s...\n", recv_port_str);

    // Cấu hình địa chỉ người nhận
    struct sockaddr_in send_addr;
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = inet_addr(send_ip);
    send_addr.sin_port = htons(atoi(send_port_str));


    // Cấu hình địa chỉ người gửi
    struct sockaddr_in addr;
    int addr_len = sizeof(addr);

    //----------------------------------------------------------------------------------
    char buf[1024], buf2[1024];

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    while(1){
        // Nhận tin nhắn từ người gửi (any)
        int ret = recvfrom(recv_socket, buf, sizeof(buf) - 1, 0, 
                       (struct sockaddr *)&addr, &addr_len);
        if (ret > 0) {
            buf[ret] = '\0';
            // Xử lý nếu tin nhận được là lệnh exit (họ không còn kết nối)
            if (strcmp(buf, "exit") == 0) {
                printf("Client %s:%d disconnected\n", 
                       inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
                continue;
            }
            printf("Received from %s:%d: %s\n", 
                   inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), buf);
        }
        else if (ret < 0 && errno != EWOULDBLOCK) {
            perror("recvfrom() failed");
            break;
        }
        
        // Gửi tin nhắn đến người nhận
        ssize_t n = read(STDIN_FILENO, buf2, sizeof(buf2) - 1);

        if (n > 0){
            buf2[strcspn(buf2, "\n")] = 0; // Xóa newline

            if (strcmp(buf2, "exit") == 0) break;

            ssize_t sent = sendto(recv_socket, buf2, strlen(buf2), 0, 
                (struct sockaddr *)&send_addr, sizeof(send_addr));
            if (sent < 0) {
                perror("sendto() failed");
                break;
            }else printf("Đã gửi!\n");
        }

        usleep(10000); // Nghỉ 10ms mỗi vòng lặp
    }
    return 0;
}