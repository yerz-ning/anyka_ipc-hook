#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

// 原始函数声明（根据反编译结果）
int yi_p2p_send_frame_data(int conn_id, void *header, void *data, int len);

// 钩子函数
int yi_p2p_send_frame_data(int conn_id, void *header, void *data, int len) {
    static int (*orig)(int, void*, void*, int) = NULL;
    if (!orig) {
        orig = (int (*)(int, void*, void*, int))dlsym(RTLD_NEXT, "yi_p2p_send_frame_data");
        if (!orig) {
            fprintf(stderr, "Failed to find yi_p2p_send_frame_data\n");
            return -1;
        }
    }
    
    // 调用原始函数，先发送数据（不影响P2P功能）
    int ret = orig(conn_id, header, data, len);
    
    // 如果数据非空，复制一份发送到本地 UDP 端口 8080
    if (data && len > 0) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock >= 0) {
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(8080);
            addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            sendto(sock, data, len, 0, (struct sockaddr*)&addr, sizeof(addr));
            close(sock);
        }
    }
    return ret;
}
