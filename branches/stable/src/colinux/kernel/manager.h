/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_KERNEL_MANAGER_H__
#define __COLINUX_KERNEL_MANAGER_H__

#include "monitor.h"
#include <colinux/common/ioctl.h>

/*
 * The manager module manages the running coLinux's systems.
 */ 
typedef struct co_manager {
	co_manager_state_t state;

	struct co_monitor *monitors[CO_MAX_MONITORS];

	unsigned long host_memory_amount;
	unsigned long host_memory_pages;

	/*
	 * The next is a map between a real physical PFN and an 
	 * address of mapping in the host's kernel virtual memory.
	 */
	void **pa_to_host_va; 

	unsigned long pa_maps_size;  /* Size of these maps */
	unsigned long pa_maps_pages; /* Size of these maps in pages */

	int monitors_count;
} co_manager_t;


extern co_rc_t co_manager_load(co_manager_t *manager);
extern co_rc_t co_manager_ioctl(co_manager_t *manager, co_monitor_ioctl_op_t ioctl, 
				void *io_buffer, unsigned long in_size,
				unsigned long out_size, unsigned long *return_size,
				void **private_data);
extern co_rc_t co_manager_cleanup(co_manager_t *manager, void **private_data);
extern void co_manager_unload(co_manager_t *manager);

#endif
