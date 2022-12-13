/* simple program that uses ioctl to send a command to given file */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int fdcntl, fdin, count;
	unsigned long cmdwr, cmdioctl;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s ioctl-command\n", argv[0]);
		return -1;
	}

	cmdwr = atol(argv[1]);
	if (cmdwr < 0 || cmdwr > 100) {
		fprintf(stderr, "Usage: %s write-command ioctl-command\n", argv[0]);
		fprintf(stderr, "write-command must be a number from {0,100}\n");
		return -1;
	}
	cmdioctl = atol(argv[2]);
	if (cmdioctl < 1 || cmdioctl > 100) {
		fprintf(stderr, "Usage: %s write-command ioctl-command\n", argv[0]);
		fprintf(stderr, "ioctl-command must be a number from {1,100}\n");
		return -1;
	}

	fdcntl = open("/dev/shofer_control", O_RDONLY);
	if (fdcntl == -1) {
		perror("open failed");
		return -1;
	}

	fdin = open("/dev/shofer_in", O_WRONLY);
	if (fdin == -1) {
		perror("open failed");
		return -1;
	}

	char buf[cmdwr];
	int i;
	for (i = 0; i < cmdwr; i++) {
		buf[i] = '#';
	}

	ssize_t s = write(fdin, buf, sizeof(buf));
	if (s == -1) {
		perror("write error");
		return -1;
	}

	count = ioctl(fdcntl, cmdioctl);
	if (count == -1) {
		perror("ioctl error");
		return -1;
	}

	printf("ioctl returned %d\n", count);

	return 0;
}
