/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_COMMON_IOCTL_H__
#define __COLINUX_COMMON_IOCTL_H__

#include "common.h"

#include <colinux/common/import.h>
#include <colinux/common/config.h>


typedef enum {
	CO_MANAGER_IOCTL_BASE=0x10,

	CO_MANAGER_IOCTL_CREATE,
	CO_MANAGER_IOCTL_MONITOR,
	CO_MANAGER_IOCTL_STATUS,
	CO_MANAGER_IOCTL_DEBUG,
	CO_MANAGER_IOCTL_DEBUG_READER,
	CO_MANAGER_IOCTL_DEBUG_LEVELS,
	CO_MANAGER_IOCTL_INFO,
} co_manager_ioctl_t;

/*
 * This struct is mapped both in kernel space and userspace.
 */
typedef struct {
	unsigned long userspace_msgwait_count;
} co_monitor_user_kernel_shared_t;

/* interface for CO_MANAGER_IOCTL_CREATE: */
typedef struct {
	co_rc_t rc;
	co_symbols_import_t import;
	co_config_t config;
	co_info_t info;
	co_arch_info_t arch_info;
	unsigned long actual_memsize_used;
	void *shared_user_address;
} co_manager_ioctl_create_t;

/* 
 * ioctls()s under CO_MANAGER_IOCTL_MONITOR: 
 */
typedef enum {
	CO_MONITOR_IOCTL_DESTROY,
	CO_MONITOR_IOCTL_LOAD_SECTION, 
	CO_MONITOR_IOCTL_START,
	CO_MONITOR_IOCTL_RUN,
	CO_MONITOR_IOCTL_STATUS,
	CO_MONITOR_IOCTL_LOAD_INITRD, 
} co_monitor_ioctl_op_t;

/* interface for CO_MANAGER_IOCTL_MONITOR: */
typedef struct {
	co_rc_t rc;
	co_monitor_ioctl_op_t op;
	char extra_data[];
} co_manager_ioctl_monitor_t;

/* interface for CO_MANAGER_IOCTL_STATUS: */
typedef struct {
	unsigned long state; /* co_manager_state_t */
	unsigned long reserved;
	int monitors_count;
	int periphery_api_version;
	int linux_api_version;
} co_manager_ioctl_status_t;

/* interface for CO_MANAGER_IOCTL_INFO: */
typedef struct {
	unsigned long hostmem_usage_limit;
	unsigned long hostmem_used;
} co_manager_ioctl_info_t;

/* interface for CO_MANAGER_IOCTL_DEBUG_READER: */
typedef struct {
	co_rc_t rc;
	void *user_buffer;
	unsigned long user_buffer_size;
	unsigned long filled;
} co_manager_ioctl_debug_reader_t;

/* interface for CO_MANAGER_IOCTL_DEBUG_LEVELS: */
typedef struct {
	co_debug_levels_t levels;
	bool_t modify;
} co_manager_ioctl_debug_levels_t;

/*
 * Monitor ioctl()s
 */

/* interface for CO_MONITOR_IOCTL_LOAD_SECTION: */
typedef struct {
	co_manager_ioctl_monitor_t pc;
	char *user_ptr;
	unsigned long address;
	unsigned long size;
	unsigned long index;
	unsigned char buf[0];
} co_monitor_ioctl_load_section_t;

/* interface for CO_MONITOR_IOCTL_LOAD_INITRD: */
typedef struct {
	co_manager_ioctl_monitor_t pc;
	unsigned long size;
	unsigned char buf[0];
} co_monitor_ioctl_load_initrd_t;

/* interface for CO_MONITOR_IOCTL_RUN: */
typedef struct {
	co_manager_ioctl_monitor_t pc;
	unsigned long num_messages;
	char data[];
} co_monitor_ioctl_run_t;

typedef enum {
	CO_MONITOR_MESSAGE_TYPE_TERMINATED,
	CO_MONITOR_MESSAGE_TYPE_DEBUG_LINE,
	CO_MONITOR_MESSAGE_TYPE_TRACE_POINT,
} co_monitor_message_type_t;

typedef struct {
	co_monitor_message_type_t type;
	union {
		struct {
			co_termination_reason_t reason;
		} terminated;
	};
} co_daemon_message_t;

/* interface for CO_MONITOR_IOCTL_STATUS */
typedef struct co_monitor_ioctl_status {
	co_manager_ioctl_monitor_t pc;
} co_monitor_ioctl_status_t;

#endif
