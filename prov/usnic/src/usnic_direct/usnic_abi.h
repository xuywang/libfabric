/*
 * Copyright (c) 2013, Cisco Systems, Inc. All rights reserved.
 *
 * LICENSE_BEGIN
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
 *
 * LICENSE_END
 *
 *
 */


#ifndef USNIC_ABI_H
#define USNIC_ABI_H

/* ABI between userspace and kernel */
#define USNIC_UVERBS_ABI_VERSION	4

#define USNIC_QP_GRP_MAX_WQS		8
#define USNIC_QP_GRP_MAX_RQS		8
#define USNIC_QP_GRP_MAX_CQS		16

#define USNIC_DECODE_PGOFF_VFID(pgoff)	((pgoff) & ((1ULL << 32) - 1))
#define USNIC_DECODE_PGOFF_TYPE(pgoff)	((pgoff) >> 48)
#define USNIC_DECODE_PGOFF_RES(pgoff)	(((pgoff) >> 32) & ((1ULL << 16) - 1))
#define USNIC_DECODE_PGOFF_BAR(pgoff)	(((pgoff) >> 32) & ((1ULL << 16) - 1))

#define USNIC_ENCODE_PGOFF(vfid, map_type, res_type_bar_id) \
	(((((uint64_t)map_type & 0xffff) << 48) | \
	  (((uint64_t)res_type_bar_id & 0xffff) << 32) | \
	  ((uint64_t)vfid & ((1ULL << 32) - 1))) * sysconf(_SC_PAGE_SIZE))

#define USNIC_UDEVCMD_NARGS 		15   /* VNIC_DEVCMD_NARGS */

enum usnic_mmap_type {
	USNIC_MMAP_BAR			= 0,
	USNIC_MMAP_RES			= 1,
	USNIC_MMAP_IOVA			= 2,
};

enum usnic_transport_type {
	USNIC_TRANSPORT_UNKNOWN		= 0,
	USNIC_TRANSPORT_ROCE_CUSTOM	= 1,
	USNIC_TRANSPORT_IPV4_UDP	= 2,
	USNIC_TRANSPORT_MAX		= 3,
};

enum usnic_ucmd_type {
	USNIC_USER_CMD_DEVCMD,
	USNIC_USER_CMD_MAX,
};

struct usnic_user_cmd {
	u32			ucmd;
	u32			pad_to_8byte;
	u64			inbuf;
	u64			outbuf;
	u32			inlen;
	u32			outlen;
};

struct usnic_udevcmd_cmd {
	u32			vnic_idx;
	u32			devcmd;
	u32			wait;
	u32			num_args;
	u64			args[USNIC_UDEVCMD_NARGS];
};

struct usnic_udevcmd_resp {
	u32			num_args;
	u64			args[USNIC_UDEVCMD_NARGS];
};

struct usnic_transport_spec {
	enum usnic_transport_type	trans_type;
	union {
		struct {
			uint16_t	port_num;
		} usnic_roce;
		struct {
			uint32_t	sock_fd;
		} udp;
	};
};

#define USNIC_IB_CREATE_QP_VERSION 2

struct usnic_ib_create_qp_cmd_v0 {
	struct usnic_transport_spec	spec;
};

struct usnic_ib_create_qp_cmd {
	struct usnic_transport_spec	spec;
	u32				cmd_version;
	union {
		struct {
			/* length in bytes of resources array */
			u32		resources_len;

			/* ptr to array of struct usnic_vnic_barres_info */
			u64		resources;
		} cur; /* v1 and v2 cmd */
	} u;
};


/*
 * infomation of vnic bar resource
 */
struct usnic_vnic_barres_info {
	int32_t	 	type;
	uint32_t	padding;
	uint64_t	bus_addr;
	uint64_t	len;
};

/*
 * All create_qp responses must start with this for backwards compatability
 */
#define USNIC_IB_CREATE_QP_RESP_V0_FIELDS                               \
	u32				vfid;                           \
	u32				qp_grp_id;                      \
	u64				bar_bus_addr;                   \
	u32				bar_len;                        \
	u32				wq_cnt;                         \
	u32				rq_cnt;                         \
	u32				cq_cnt;                         \
	u32				wq_idx[USNIC_QP_GRP_MAX_WQS];   \
	u32				rq_idx[USNIC_QP_GRP_MAX_RQS];   \
	u32				cq_idx[USNIC_QP_GRP_MAX_CQS];   \
	u32				transport;

struct usnic_ib_create_qp_resp_v0 {
	USNIC_IB_CREATE_QP_RESP_V0_FIELDS
	u32				reserved[9];
};

struct usnic_ib_create_qp_resp {
	USNIC_IB_CREATE_QP_RESP_V0_FIELDS
	/* the above fields end on 4-byte alignment boundary */
	u32 cmd_version;
	union {
		struct {
			u32 num_barres;
			u32 pad_to_8byte;
		} v1;
		struct {
			u32 num_barres;
			u32 wq_err_intr_offset;
			u32 rq_err_intr_offset;
			u32 wcq_intr_offset;
			u32 rcq_intr_offset;
			u32 pad_to_8byte;
		} cur; /* v2 */
	} u;

	/* v0 had a "reserved[9]" field, must not shrink the response or we can
	 * corrupt newer clients running on older kernels */
	u32				reserved[2];
};

#define USNIC_CTX_RESP_VERSION 2

/*
 * Make this structure packed in order to make sure v1.num_caps not aligned
 * at 8 byte boundary, hence still being able to support user libary
 * requesting version 1 response.
 */
struct __attribute__((__packed__)) usnic_ib_get_context_cmd {
	u32 resp_version;	/* response version requested */
	union {
		struct {
			u32 num_caps;	/* number of capabilities requested */
		} v1;
		struct {
			u32 encap_subcmd;	/* whether encapsulate subcmd */
			union {
				u32 num_caps;
				struct usnic_user_cmd usnic_ucmd;
			};
		} v2;
	};
};

/*
 * Note that this enum must never have members removed or re-ordered in order
 * to retain backwards compatability
 */
enum usnic_capability {
	USNIC_CAP_CQ_SHARING,	/* CQ sharing version */
	USNIC_CAP_MAP_PER_RES,	/* Map individual RES */
	USNIC_CAP_PIO,		/* PIO send */
	USNIC_CAP_CQ_INTR,	/* CQ interrupts (via comp channels) */
	USNIC_CAP_SHARE_PD,	/* share PD */
	USNIC_CAP_CNT
};

/*
 * If and when there become multiple versions of this struct, it will
 * become a union for cross-version compatability to make sure there is always
 * space for older and larger versions of the contents.
 */
struct usnic_ib_get_context_resp {
	u32 resp_version;	/* response version returned */
	u32 num_caps;		/* number of capabilities returned */
	u32 cap_info[USNIC_CAP_CNT];
};

#define USNIC_IB_CREATE_CQ_VERSION 1

enum usnic_cq_intr_arm_mode {
	USNIC_CQ_INTR_ARM_MODE_CONTINUOUS,   /* continuously armed after
						CQ(+QP) creation */
	USNIC_CQ_INTR_ARM_MODE_CNT
};

struct usnic_ib_create_cq_v0 {
	u64 reserved;
};

struct usnic_ib_create_cq {
	u32 resp_version; /* response version requested */
	u32 intr_arm_mode;
};

struct usnic_ib_create_cq_resp_v0 {
	u64 reserved;
};

struct usnic_ib_create_cq_resp {
	u32 resp_version; /* response version returned */
	u32 pad_to_8byte;
};

#define USNIC_IB_REG_MR_CMD_VERSION 	1
#define USNIC_IB_REG_MR_RESP_VERSION 	1

struct usnic_ib_reg_mr_cmd {
	u32 cmd_version;
	union {
		struct {
			u32 hw_addr_type;	/* Address space for addresses to be
						 * programmed to HW and IOMMU */
#define USNIC_REGMR_HWADDR_VA		0	/* address programmed to HW is same as
						 * process va */
#define USNIC_REGMR_HWADDR_IOVA		1	/* address programmed to HW is obtained
						 * from a specially allocated address
						 * (called io va) */
			u32 vfid;
			u32 mr_type;
#define USNIC_MR_WQ_RING		0
#define USNIC_MR_RQ_RING		1
#define USNIC_MR_CQ_RING		2
#define USNIC_MR_WQ_COPYBUF		3
#define USNIC_MR_RQ_HDRBUF		4	/* Created in usdf DGRAM EP */
#define USNIC_MR_WQ_INJECTBUF		5	/* Created in usdf RDM and MSG EP */
#define USNIC_MR_RQ_RXBUF		6	/* created in usdf RDM and MSG EP */
#define USNIC_MR_MAX			7
			u32 queue_index;
		} v1;
		/* Add future version fields here */
	};
};

struct usnic_ib_reg_mr_resp {
	u32 resp_version;
	union {
		struct {
			u64 iova;	/* translated virtual address to programmed to HW */
		} v1;
	};
};

#define USNIC_IB_ALLOC_SHPD_CMD_VERSION		1

struct usnic_ib_alloc_shpd_cmd {
	u32 cmd_version;
	union {
		struct {
			u64 iova_start;
			u64 iova_length;
		} v1;
	};
};

#endif /* USNIC_ABI_H */
