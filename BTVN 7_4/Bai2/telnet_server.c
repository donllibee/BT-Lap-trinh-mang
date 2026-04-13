#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

struct client_info {
    int fd;
    int logged_in;
};

void remove_client(struct client_info clients[], int *num, int fd) {
    for (int i = 0; i < *num; i++) {
        if (clients[i].fd == fd) {
            for (int j = i; j < *num - 1; j++) {
                clients[j] = clients[j + 1];
            }
            (*num)--;
            break;
        }
    }
}

int check_login(char *user, char *pass) {
    FILE *f = fopen("database.txt", "r");
    if (!f) return 0;    
    char f_user[64], f_pass[64];
    while (fscanf(f, "%s %s", f_user, f_pass) == 2) {
        if (strcmp(user, f_user) == 0 && strcmp(pass, f_pass) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
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
    printf("Telnet Server dang lang nghe o cong %s...\n", argv[1]);
    fd_set fdread, fdtest;
    FD_ZERO(&fdread);
    FD_SET(listener, &fdread);
    struct client_info clients[64];
    int num_clients = 0;
    char buf[1024];
    while (1) {
        fdtest = fdread;
        int ret = select(FD_SETSIZE, &fdtest, NULL, NULL, NULL);
        if (ret < 0) break;
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &fdtest)) {
                if (i == listener) {
                    int new_client = accept(listener, NULL, NULL);
                    FD_SET(new_client, &fdread);
                    clients[num_clients].fd = new_client;
                    clients[num_clients].logged_in = 0;
                    num_clients++;
                    char *msg = "Vui long nhap tai khoan (Cu phap: user pass):\n";
                    send(new_client, msg, strlen(msg), 0);
                } 
                else {
                    int bytes = recv(i, buf, sizeof(buf) - 1, 0);
                    if (bytes <= 0) {
                        close(i);
                        FD_CLR(i, &fdread);
                        remove_client(clients, &num_clients, i);
                    } else {
                        buf[bytes] = '\0';
                        buf[strcspn(buf, "\r\n")] = 0;
                        int c_idx = -1;
                        for (int j = 0; j < num_clients; j++) {
                            if (clients[j].fd == i) {
                                c_idx = j; break;
                            }
                        }
                        if (clients[c_idx].logged_in == 0) {
                            char user[64], pass[64];
                            if (sscanf(buf, "%s %s", user, pass) == 2 && check_login(user, pass)) {
                                clients[c_idx].logged_in = 1;
                                char *ok_msg = "Dang nhap thanh cong! Ban co the nhap lenh:\n";
                                send(i, ok_msg, strlen(ok_msg), 0);
                            } else {
                                char *err_msg = "Loi dang nhap! Tai khoan hoac mat khau khong dung.\n";
                                send(i, err_msg, strlen(err_msg), 0);
                            }
                        } else {
                            char cmd[1100];
                            sprintf(cmd, "%s > out.txt", buf); 
                            system(cmd);
                            FILE *f = fopen("out.txt", "r");
                            if (f) {
                                char out_buf[1024];
                                int has_output = 0;
                                while (fgets(out_buf, sizeof(out_buf), f)) {
                                    send(i, out_buf, strlen(out_buf), 0);
                                    has_output = 1;
                                }
                                if (!has_output) {
                                    char *empty_msg = "(Khong co ket qua tra ve hoac lenh sai)\n";
                                    send(i, empty_msg, strlen(empty_msg), 0);
                                }
                                fclose(f);
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