#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <errno.h>

#define MAX_CLIENTS 64

// ================= CLIENT STRUCT =================
typedef struct {
    int sock;
    int state; // 0: nhap ten, 1: nhap MSSV, 2: done
    char name[100];
    char mssv[20];
} Client;

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    // Non-blocking listener
    unsigned long ul = 1;
    ioctl(listener, FIONBIO, &ul);

    // Reuse port
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }

    // Bind
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        return 1;
    }

    if (listen(listener, 5)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }

    printf("Server is listening on port 8080...\n");

    // ================= CLIENT LIST =================
    Client clients[MAX_CLIENTS];
    int nclients = 0;

    char buf[256];
    int len;

    while (1) {
        // ================= ACCEPT =================
        int client = accept(listener, NULL, NULL);
        if (client != -1) {
            printf("New client: %d\n", client);

            if (nclients < MAX_CLIENTS) {
                clients[nclients].sock = client;
                clients[nclients].state = 0;

                // set non-blocking
                ul = 1;
                ioctl(client, FIONBIO, &ul);

                send(client, "Nhap ho ten:\n", 13, 0);

                nclients++;
            } else {
                close(client);
            }
        }

        // ================= HANDLE CLIENTS =================
        for (int i = 0; i < nclients; i++) {
            len = recv(clients[i].sock, buf, sizeof(buf) - 1, 0);

            if (len == -1) {
                if (errno != EWOULDBLOCK) {
                    printf("Client %d disconnected\n", clients[i].sock);
                    close(clients[i].sock);

                    for (int j = i; j < nclients - 1; j++) {
                        clients[j] = clients[j + 1];
                    }
                    nclients--;
                    i--;
                }
                continue;
            }

            if (len == 0) continue;

            buf[len] = 0;

            buf[strcspn(buf, "\r\n")] = 0;

            // ================= STATE MACHINE =================
            if (clients[i].state == 0) {
                // nhap ten
                strcpy(clients[i].name, buf);

                send(clients[i].sock, "Nhap MSSV:\n", 13, 0);
                clients[i].state = 1;
            }
            else if (clients[i].state == 1) {
                strcpy(clients[i].mssv, buf);

                // ====== TAO EMAIL ======
                char email[200];

                // Lay ten (tu cuoi)
                char *last = strrchr(clients[i].name, ' ');
                char ten[50];
                if (last) strcpy(ten, last + 1);
                else strcpy(ten, clients[i].name);

                // Lay chu cai dau ho + dem
                char initials[50] = "";
                char temp[100];
                strcpy(temp, clients[i].name);

                char *token = strtok(temp, " ");
                while (token != NULL) {
                    if (strcmp(token, ten) != 0) {
                        char c[2] = {token[0], 0};
                        strcat(initials, c);
                    }
                    token = strtok(NULL, " ");
                }

                // Bo 2 so dau MSSV
                char *mssv_cut = clients[i].mssv + 2;

                sprintf(email, "%s%s%s@sis.hust.edu.vn\n",
                        ten, initials, mssv_cut);

                send(clients[i].sock, email, strlen(email), 0);

                clients[i].state = 2;
            }
        }
    }

    close(listener);
    return 0;
}