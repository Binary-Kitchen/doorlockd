/*
 * Doorlockd -- Binary Kitchen's smart door opener
 *
 * Copyright (c) Binary Kitchen e.V., 2018
 *
 * Author: Ralf Ramsauer <ralf@binary-kitchen.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the LICENSE file in the top-level directory.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <linux/gpio.h>

int main(int argc, const char **argv) {
	int gpiochip_fd, ret;
	struct gpioevent_request req;
	struct gpiohandle_data data;
	struct gpioevent_data event;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s gpiochip pin\n", argv[0]);
		return -1;
	}

	gpiochip_fd = open(argv[1], 0);
	if (gpiochip_fd == -1) {
		perror("open");
		return -1;
	}

	req.lineoffset = atoi(argv[2]);
	req.handleflags = GPIOHANDLE_REQUEST_INPUT;
	req.eventflags = GPIOEVENT_REQUEST_RISING_EDGE | GPIOEVENT_REQUEST_FALLING_EDGE;
	req.consumer_label[0] = 0;

	ret = ioctl(gpiochip_fd, GPIO_GET_LINEEVENT_IOCTL, &req);
	if (ret == -1) {
		perror("ioctl");
		goto gpiochip_out;
	}

	ret = ioctl(req.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
	if (ret == -1) {
		perror("ioctl");
		goto req_fd_out;
	}

	printf("Initial Value: %u\n", data.values[0]);

retry:
	ret = read(req.fd, &event, sizeof(event));
	if (ret == -1) {
		if (errno == -EAGAIN) {
			printf("Nothing available, trying again\n");
			goto retry;
		}
	}

	if (ret != sizeof(event)) {
		ret = -EIO;
		goto req_fd_out;
	}

	switch (event.id) {
		case GPIOEVENT_EVENT_RISING_EDGE:
			printf("Detected rising edge\n");
			ret = 1;
			break;
		case GPIOEVENT_EVENT_FALLING_EDGE:
			printf("Detected falling edge\n");
			ret = 0;
			break;
		default:
			printf("Unknown event\n");
			ret = -EIO;
			
			break;
	}

req_fd_out:
	close(req.fd);
gpiochip_out:
	close(gpiochip_fd);

	return ret;
}
