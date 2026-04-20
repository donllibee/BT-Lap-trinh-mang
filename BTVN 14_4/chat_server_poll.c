#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>

struct client_info {
    int fd;
    int registered;
    char id[32];
    char name[64];
};

void remove_client(struct pollfd fds[], struct client_info clients[], int *nfds, int index) {
    close(fds[index].fd);
    for (int i = index; i < *nfds - 1; i++) {
        fds[i] = fds[i + 1];
        clients[i] = clients[i + 1];
    }
    (*nfds)--;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Cu phap: %s <Cổng>\n", argv[0]);
        return 1;
    }

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(argv[1]));

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);
    printf("Poll Chat Server dang lang nghe o cong %s...\n", argv[1]);

    struct pollfd fds[64];
    int nfds = 1;
    fds[0].fd = listener;
    fds[0].events = POLLIN;
    struct client_info clients[64];
    char buf[1024];
    while (1) {
        int ret = poll(fds, nfds, -1);
        if (ret < 0) break;
        if (fds[0].revents & POLLIN) {
            int new_client = accept(listener, NULL, NULL);
            printf(">> Co client moi ket noi (ID: %d)\n", new_client);
            fds[nfds].fd = new_client;
            fds[nfds].events = POLLIN;
            clients[nfds].fd = new_client;
            clients[nfds].registered = 0;
            nfds++;

            char *msg = "Vui long nhap theo cu phap: client_id: client_name\n";
            send(new_client, msg, strlen(msg), 0);
        }
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & (POLLIN | POLLERR)) {
                int bytes = recv(fds[i].fd, buf, sizeof(buf) - 1, 0);
                if (bytes <= 0) {
                    printf(">> Client %d da thoat\n", fds[i].fd);
                    remove_client(fds, clients, &nfds, i);
                    i--;
                } else {
                    buf[bytes] = '\0';
                    buf[strcspn(buf, "\r\n")] = 0;
                    if (clients[i].registered == 0) {
                        // Xử lý đăng ký bằng sscanf
                        char id_tmp[32], name_tmp[64];
                        if (sscanf(buf, "%[^:]: %[^\n]", id_tmp, name_tmp) == 2) {
                            strcpy(clients[i].id, id_tmp);
                            strcpy(clients[i].name, name_tmp);
                            clients[i].registered = 1;
                            char *ok_msg = "Dang ky thanh cong! Ban da vao phong chat.\n";
                            send(fds[i].fd, ok_msg, strlen(ok_msg), 0);
                        } else {
                            char *err_msg = "Sai cu phap! Vui long nhap lai: client_id: client_name\n";
                            send(fds[i].fd, err_msg, strlen(err_msg), 0);
                        }
                    } else {
                        char out_msg[2048];
                        time_t t = time(NULL);
                        struct tm *tm_info = localtime(&t);
                        char time_str[32];
                        strftime(time_str, sizeof(time_str), "%Y/%m/%d %I:%M:%S%p", tm_info);
                        sprintf(out_msg, "%s %s: %s\n", time_str, clients[i].id, buf);
                        for (int j = 1; j < nfds; j++) {
                            if (clients[j].registered == 1 && j != i) {
                                send(fds[j].fd, out_msg, strlen(out_msg), 0);
                            }
                        }
                    }
                }
            }
        }
    }
    close(listener);
    return 0;
}