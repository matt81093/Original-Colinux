/*
 * This source code is a part of coLinux source package.
 *
 * Nuno Lucas <lucas@xpto.ath.cx> 2005 (c)
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 
#ifndef __COLINUX_USER_CONSOLE_MAIN_H__
#define __COLINUX_USER_CONSOLE_MAIN_H__

extern "C" {
    #include <colinux/common/common.h>
}


/**
 * Console startup parameters
 *
 */
struct console_parameters_t
{
    co_id_t     instance_id;    // colinux instance id to attach to
    char  *     motd;           // Message of the day
};


#endif  // __COLINUX_USER_CONSOLE_MAIN_H__
