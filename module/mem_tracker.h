#ifndef __MEM_TRACKER_H_
#define __MEM_TRACKER_H_


#include<linux/mm.h>
#include<linux/mm_types.h>
#include<linux/file.h>
#include<linux/fs.h>
#include<linux/path.h>
#include<linux/slab.h>
#include<linux/dcache.h>
#include<linux/sched.h>
#include<linux/uaccess.h>
#include<linux/fs_struct.h>
#include <asm/tlbflush.h>
#include<linux/uaccess.h>
#include<linux/device.h>


#define DEVNAME "memtrack"

enum command_t{
                  COUNT_TLB_MISS,
                  COUNT_TLB_MISS_PTI,
                  COUNT_WSS,
                  MAX_COMMANDS
};


extern struct attribute_group memtrack_attr_group;

extern int (*rsvd_fault_hook)(struct mm_struct *mm, struct pt_regs *regs, unsigned long error_code, unsigned long address);
extern int page_fault_pid;

extern ssize_t handle_read(char *buff, size_t length);
extern ssize_t handle_write(const char *buff, size_t lenth);
extern int handle_open(void);
extern int handle_close(void);

#endif
