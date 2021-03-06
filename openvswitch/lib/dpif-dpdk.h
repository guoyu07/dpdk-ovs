/*
 * Copyright 2012-2014 Intel Corporation All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DPIF_DPDK_H
#define DPIF_DPDK_H 1

#include <stdbool.h>
#include <rte_ether.h>
#include <linux/openvswitch.h>

#include "ofpbuf.h"
#include "dpif.h"
#include "common.h"

#define DPIF_DPDK_VPORT_FAMILY     0xE
#define DPIF_DPDK_FLOW_FAMILY      0xF
#define DPIF_DPDK_PACKET_FAMILY    0x1F

#define DPDK_PORT_MAX_STRING_LEN   32

struct dpif_dpdk_vport_stats {
	uint64_t rx;        /* Rx packet count */
	uint64_t tx;        /* Tx packet count */
	uint64_t rx_bytes;  /* Rx packet count */
	uint64_t tx_bytes;  /* Tx bytes count */
	uint64_t rx_drop;   /* Rx dropped packet count */
	uint64_t tx_drop;   /* Tx dropped packet count */
	uint64_t rx_error;  /* Rx error packet count */
	uint64_t tx_error;  /* Tx error packet count */
};

struct dpif_dpdk_flow_key {
	odp_port_t in_port;
	struct ether_addr ether_dst;
	struct ether_addr ether_src;
	uint16_t ether_type;
	uint16_t vlan_id;
	uint8_t vlan_prio;
	uint32_t ip_src;
	uint32_t ip_dst;
	uint8_t ip_proto;
	uint8_t ip_tos;
	uint8_t ip_ttl;
	uint8_t ip_frag;
	uint16_t tran_src_port;
	uint16_t tran_dst_port;
} __attribute__((__packed__));

struct dpif_dpdk_flow_stats {
	uint64_t packet_count;
	uint64_t byte_count;
	uint64_t used;
	uint8_t tcp_flags;
};

struct dpif_dpdk_upcall {
	uint8_t cmd;
	struct dpif_dpdk_flow_key key;
};

/* TODO: same value as VPORTS increase if required */
/* Increasing this value to the same as MAX_VPORTS
 * introduces packet corruption
 */
#define MAX_ACTIONS    (48)

enum dpif_dpdk_vport_type {
	VPORT_TYPE_DISABLED = 0,
	VPORT_TYPE_VSWITCHD,
	VPORT_TYPE_BRIDGE,
	VPORT_TYPE_PHY,
	VPORT_TYPE_CLIENT,
	VPORT_TYPE_KNI,
	VPORT_TYPE_VETH,
	VPORT_TYPE_VHOST
};

enum dpif_dpdk_action_type {
	ACTION_NULL,     /* Empty action */
	ACTION_OUTPUT,   /* Output packet to port */
	ACTION_POP_VLAN, /* Remove 802.1Q header */
	ACTION_PUSH_VLAN,/* Add 802.1Q VLAN header to packet */
	ACTION_SET_ETHERNET, /* Modify Ethernet header */
	ACTION_SET_IPV4, /* Modify IPV4 header */
	ACTION_SET_TCP, /* Modify TCP header */
	ACTION_SET_UDP, /* Modify UDP header */
	ACTION_MAX       /* Maximum number of supported actions */
};

struct dpif_dpdk_action_output {
	uint32_t port;    /* Output port */
};

struct dpif_action_push_vlan {
	uint16_t tpid; /* Tag Protocol ID (always 0x8100) */
	uint16_t tci;  /* Tag Control Information */
};

struct dpif_dpdk_action {
	enum dpif_dpdk_action_type type;
	union { /* union of different action types */
		struct dpif_dpdk_action_output output;
		struct dpif_action_push_vlan vlan;
		struct ovs_key_ethernet ethernet;
		struct ovs_key_ipv4 ipv4;
		struct ovs_key_tcp tcp;
		struct ovs_key_udp udp;
		/* add other action structs here */
	} data;
};

/* A 'vport managment' message between vswitchd <-> datapath. */
struct dpif_dpdk_vport_message {
	uint32_t id;              /* Thread ID of sending thread */
	uint8_t cmd;              /* Command to execute on vport. */
	uint32_t flags;           /* Additional flags, if any, or null. */
	uint32_t port_no;         /* Number of the vport. */
	char port_name[DPDK_PORT_MAX_STRING_LEN];    /* Name of the vport. */
	enum dpif_dpdk_vport_type type;      /* Type of the vport */
	char reserved[16];           /* Padding - currently reserved for future use */
	struct dpif_dpdk_vport_stats stats;  /* Current statistics for the given vport. */
};

struct dpif_dpdk_flow_message {
	uint32_t id;
	uint8_t cmd;
	uint32_t flags;
	struct dpif_dpdk_flow_key key;
	struct dpif_dpdk_flow_stats stats;
	struct dpif_dpdk_action actions[MAX_ACTIONS];
	bool clear;
};

struct dpif_dpdk_packet_message {
	struct dpif_dpdk_action actions[MAX_ACTIONS];
};

/* A message between vswitchd <-> datapath. */
struct dpif_dpdk_message {
	int16_t type;        /* Message type, if a request, or return code */
	union {              /* Actual message */
		struct dpif_dpdk_vport_message vport_msg;
		struct dpif_dpdk_flow_message flow_msg;
		struct dpif_dpdk_packet_message packet_msg;
	};
};

/* Semantic wrapping of a vport_message as a state struct. This is used in
 * by the state machine found in the DPIF's 'dump' command. */
struct dpif_dpdk_port_state {
	struct dpif_dpdk_vport_message vport;
};

struct dpif_dpdk_flow_state {
	struct dpif_dpdk_flow_message flow;
	struct dpif_flow_stats stats;
	struct ofpbuf actions_buf;
	struct ofpbuf key_buf;
};

int dpif_dpdk_port_get_stats(const char *name, struct dpif_dpdk_vport_stats *stats);

#endif /* dpif-dpdk.h */
