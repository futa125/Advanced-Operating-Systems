#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#define FIB_DEV "/dev/shofer"

void write_child(void *data) {
    int fd = open(FIB_DEV, O_WRONLY);

    if (fd < 0) {
        perror("failed to open");
        return;
    }

    sleep((rand() % 4) + 1);
    if (write(fd, (char*)data, 3) == -1) {
        perror("failed to write");
        return;
    }
}

void read_child(void *data) {
    int fd = open(FIB_DEV, O_RDONLY);

    if (fd < 0) {
        perror("failed to open");
        return;
    }

    sleep((rand() % 4) + 1);
    char buf[3];
    if (read(fd, buf, 3) == -1) {
        perror("failed to write");
        return;
    }
}

int main()
{
    pthread_t t[20];
    char strings[][3] = {"aaa", "bbb", "CCC", "ddd", "eee", "fff", "ggg", "hhh", "iii", "jjj",
                         "kkk", "lll", "mmm", "nnn", "ooo", "ppp", "qqq", "rrr", "sss", "ttt"};

    for (int i=0; i<10; ++i) {
        pthread_create(&(t[i]), NULL, (void*)read_child, strings[i]);
    }
    for (int i=10; i<20; ++i) {
        pthread_create(&(t[i]), NULL, (void*)write_child, strings[i]);
    }

    for (int i=0; i<20; ++i) {
        pthread_join(t[i], NULL);
    }

    return 0;
}