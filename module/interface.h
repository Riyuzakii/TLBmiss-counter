#ifndef __INTERFACE_H_
#define __INTERFACE_H_

#define MAX_TOPPERS 5

enum read_cmd_t{
                 FAULT_START,
                 TLBMISS_TOPPERS,
                 READ_TOPPERS,
                 WRITE_TOPPERS,
                 MAX_READ_COMMANDS
};

struct topper_t{
                  unsigned long vaddr;
                  long count;                   
};

struct read_command{
                     long command;    // One of the commands in enum read_cmd_t
                     long valid_entries; //#of valid toppers
                     struct topper_t toppers[MAX_TOPPERS]; // Topper information
}; 

#endif
