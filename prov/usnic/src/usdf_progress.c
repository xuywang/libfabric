/*
 * Copyright (c) 2014, Cisco Systems, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_prov.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_rma.h>
#include <rdma/fi_errno.h>
#include "fi.h"

#include "usnic_direct.h"
#include "usdf.h"

void
usdf_progress(struct usdf_domain *udp)
{
	usdf_av_progress(udp);
}

int
usdf_add_progression_item(struct usdf_domain *udp)
{
	int64_t val;
	int n;

	if (atomic_inc(&udp->dom_pending_items) == 1) {
		val = 1;
		n = write(udp->dom_eventfd, &val, sizeof(val));
		if (n != sizeof(val)) {
			return -FI_EIO;
		}
	}
	return 0;
}

void
usdf_progression_item_complete(struct usdf_domain *udp)
{
	atomic_dec(&udp->dom_pending_items);
}

void *
usdf_progression_thread(void *v)
{
	struct usdf_domain *udp;
	int64_t val;
	struct pollfd pfd;
	int sleep_time;
	int ret;
	int n;

	udp = v;

	pfd.fd = udp->dom_eventfd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	while (1) {

		/* sleep inifinitely if nothing to do */
		if (atomic_get(&udp->dom_pending_items) > 0) {
			sleep_time = 1;
		} else {
			sleep_time = -1;
		}

		ret = poll(&pfd, 1, sleep_time);
		if (ret == -1) {
			pthread_exit(NULL);
		}
		/* consume write if there was one */
		if (ret == 1) {
			n = read(udp->dom_eventfd, &val, sizeof(val));
			if (n != sizeof(val)) {
				pthread_exit(NULL);
			}
		}

		if (udp->dom_exit) {
			pthread_exit(NULL);
		}

		usdf_progress(udp);
	}
}
