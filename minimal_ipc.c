#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// ============================================================
// 1. 结构体定义（从反编译代码中提取）
// ============================================================

struct venc_param {
    int width;
    int height;
    int qp_min;
    int qp_max;
    int fps;
    int gop;
    int bitrate_kbps;
    int profile;
    int video_mode;
    int method;
    int type;
    int group;
};

// ============================================================
// 2. 外部函数声明（直接从反编译代码中抄出）
// ============================================================

// 视频输入
void* ak_vi_open(int dev_type);
int ak_vi_match_sensor(const char* path);
int ak_vi_set_channel_attr(void* handle, void* attr);
int ak_vi_capture_on(void* handle);
int ak_vi_close(void* handle);

// 视频编码
int* ak_venc_open(struct venc_param* param);
int ak_venc_get_stream(int* stream_handle, int* output_struct);

// 内存管理
void akuio_pmem_init(void);
void* akuio_alloc_pmem(void* param);
void akuio_free_pmem(void* ptr);

// 系统辅助
void ak_sleep_ms(int ms);
void ak_print(int level, const char* fmt, ...);
void* calloc(size_t nmemb, size_t size);
void free(void* ptr);

// ============================================================
// 3. UDP 发送函数
// ============================================================

int udp_send(unsigned char* data, int len) {
    static int sock = -1;
    static struct sockaddr_in addr;
    if (sock < 0) {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) return -1;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(8080);
        inet_aton("127.0.0.1", &addr.sin_addr);
    }
    return sendto(sock, data, len, 0, (struct sockaddr*)&addr, sizeof(addr));
}

// ============================================================
// 4. 主程序
// ============================================================

int main() {
    void *vi_handle;
    int *venc_handle;
    int output[6];
    int ret;

    printf("[minimal_ipc] Starting...\n");

    // 1. 初始化内存管理
    akuio_pmem_init();

    // 2. 匹配传感器（与 anyka_ipc 一致）
    ret = ak_vi_match_sensor("/etc/jffs2/");
    if (ret != 0) {
        printf("Sensor match failed\n");
        return -1;
    }

    // 3. 打开视频输入
    vi_handle = ak_vi_open(0);
    if (vi_handle == NULL) {
        printf("ak_vi_open failed\n");
        return -1;
    }

    // 4. 设置编码参数（1280x720, 25fps, 2048kbps）
    struct venc_param param = {
        .width = 1280,
        .height = 720,
        .qp_min = 10,
        .qp_max = 40,
        .fps = 25,
        .gop = 30,
        .bitrate_kbps = 2048,
        .profile = 1,
        .video_mode = 0,
        .method = 0,
        .type = 0,
        .group = 0
    };

    // 5. 打开编码器
    venc_handle = ak_venc_open(&param);
    if (venc_handle == NULL) {
        printf("ak_venc_open failed\n");
        ak_vi_close(vi_handle);
        return -1;
    }

    printf("Encoder opened, capturing frames...\n");

    // 6. 循环获取 H.264 帧并发送到 UDP 8080
    while (1) {
        ret = ak_venc_get_stream(venc_handle, output);
        if (ret == 0 && output[0] != 0) {
            // output[0] = 数据指针, output[1] = 长度
            udp_send((unsigned char*)output[0], output[1]);
            printf("[FRAME] len=%d, ts=%d\n", output[1], output[2]);
        }
        ak_sleep_ms(10);
    }

    return 0;
}
