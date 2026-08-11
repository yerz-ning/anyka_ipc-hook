#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc, char **argv) {
    int fd, steps, cmd;

    if (argc < 3) {
        printf("Usage: %s <0|1> <steps>\n", argv[0]);
        printf("  0 = clockwise, 1 = counter-clockwise\n");
        return 1;
    }

    fd = open("/dev/ak-motor0", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    steps = atoi(argv[2]);
    cmd = (atoi(argv[1]) == 0) ? 0x40046d0d : 0x40046d0e; // 0=顺时针，1=逆时针

    int ret = ioctl(fd, cmd, &steps);
    printf("ioctl returned %d, steps=%d\n", ret, steps);
    close(fd);
    return 0;
}
