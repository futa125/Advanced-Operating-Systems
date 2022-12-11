#include <poll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main() {
    int nfds, num_open_fds;
    struct pollfd *pfds;
    char shofer[12 + 1];

    num_open_fds = nfds = 6;
    pfds = calloc(nfds, sizeof(struct pollfd));
    if (pfds == NULL)
        errExit("malloc");

    /* Open each file on command line, and add it 'pfds' array. */

    for (int j = 0; j < nfds; j++) {
        sprintf(shofer, "/dev/shofer%d", j);
        pfds[j].fd = open(shofer, O_WRONLY);
        if (pfds[j].fd == -1)
            errExit("open");

        printf("Opened \"%s\" on fd %d\n", shofer, pfds[j].fd);

        pfds[j].events = POLLOUT;
    }

    /* Keep calling poll() as long as at least one file descriptor is
        open. */
    while (num_open_fds > 0) {
        int ready;

        printf("About to poll()\n");
        ready = poll(pfds, nfds, -1);
        if (ready == -1)
            errExit("poll");

        printf("Ready: %d\n", ready);

        /* Deal with array returned by poll(). */


        char buf[24 + 1];
        while(1) {
            int j = rand() % (nfds);
            sprintf(buf, "random data for shofer %d", j);

            if (pfds[j].revents != 0) {
                printf("  fd=%d; events: %s%s%s\n", pfds[j].fd,
                        (pfds[j].revents & POLLOUT) ? "POLLOUT "  : "",
                        (pfds[j].revents & POLLHUP) ? "POLLHUP " : "",
                        (pfds[j].revents & POLLERR) ? "POLLERR " : "");

                if (pfds[j].revents & POLLOUT) {
                        ssize_t s = write(pfds[j].fd, buf, sizeof(buf));
                    if (s == -1)
                        errExit("write");
                    printf("    write %zd bytes: %.*s\n",
                            s, (int) s, buf);
                } else {                /* POLLERR | POLLHUP */
                    printf("    closing fd %d\n", pfds[j].fd);
                    if (close(pfds[j].fd) == -1)
                        errExit("close");
                    num_open_fds--;
                }

                sleep(5);
                break;
            }
        }
    }

    printf("All file descriptors closed; bye\n");
    exit(EXIT_SUCCESS);
}