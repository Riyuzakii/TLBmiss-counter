/*
Name: Aditya Rohan
Roll Number: 160053
*/

#include "mem_tracker.h"
#include "interface.h"

static int command;
static unsigned long tlb_misses, readwss, writewss, unused;

//My Global Variables 
static unsigned long gptr, total_pgs;
#define WRITE_MASK 2 
static pte_t *gpte;
extern int (*rsvd_fault_hook)(struct mm_struct *mm, struct pt_regs *regs, unsigned long error_code, unsigned long address);
extern int page_fault_pid;
static unsigned long *tlbmisses_cnt_pgs, *writes_cnt_pgs, *reads_cnt_pgs, * unused_cnt_pgs; 
static unsigned long *tlb_toppers, *write_toppers, *read_toppers;
static struct read_command cmd;

static ssize_t memtrack_command_show(struct kobject *kobj,
															struct kobj_attribute *attr, char *buf)
{
				return sprintf(buf, "%d\n", command);
}

static ssize_t memtrack_command_set(struct kobject *kobj,
															 struct kobj_attribute *attr,
															 const char *buf, size_t count)
{
	/*TODO		Part of assignment, needed to be implemented by you*/
	int int_buff;
	kstrtoint(buf, 0, &int_buff);

	if(int_buff == 0){
		command = 0;
	}
	else if(int_buff == 1){
		command = 1;
	}
	else if(int_buff == 2){
		command = 2;
	}
	return count;
}

static struct kobj_attribute memtrack_command_attribute = __ATTR(command,0644,memtrack_command_show, memtrack_command_set);

static ssize_t memtrack_tlb_misses_show(struct kobject *kobj,
															struct kobj_attribute *attr, char *buf)
{
				return sprintf(buf, "%lu\n", tlb_misses);

}
static struct kobj_attribute memtrack_tlb_misses_attribute = __ATTR(tlb_misses, 0444,memtrack_tlb_misses_show, NULL);

static ssize_t memtrack_readwss_show(struct kobject *kobj,
															struct kobj_attribute *attr, char *buf)
{
				return sprintf(buf, "%lu\n", readwss);

}
static struct kobj_attribute memtrack_readwss_attribute = __ATTR(readwss, 0444,memtrack_readwss_show, NULL);

static ssize_t memtrack_writewss_show(struct kobject *kobj,

															struct kobj_attribute *attr, char *buf)

{
				return sprintf(buf, "%lu\n", writewss);
}
static struct kobj_attribute memtrack_writewss_attribute = __ATTR(writewss, 0444,memtrack_writewss_show, NULL);


static ssize_t memtrack_unused_show(struct kobject *kobj,
															struct kobj_attribute *attr, char *buf)
{
				return sprintf(buf, "%lu\n", unused);

}
static struct kobj_attribute memtrack_unused_attribute = __ATTR(unused, 0444,memtrack_unused_show, NULL);
static struct attribute *memtrack_attrs[] = {
				&memtrack_command_attribute.attr,
				&memtrack_tlb_misses_attribute.attr,
				&memtrack_readwss_attribute.attr,
				&memtrack_writewss_attribute.attr,
				&memtrack_unused_attribute.attr,
				NULL,

};
struct attribute_group memtrack_attr_group = {
				.attrs = memtrack_attrs,
				.name = "memtrack",

};


static pte_t* get_pte(unsigned long address, unsigned long *addr_vma)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep;
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma = find_vma(mm, address);
	if(!vma){
					 // printk(KERN_INFO "No vma yet\n");
					 goto nul_ret;
	}
 
	*addr_vma = (unsigned long) vma;

	pgd = pgd_offset(mm, address);
	if (pgd_none(*pgd) || unlikely(pgd_bad(*pgd)))
					goto nul_ret;
	// printk(KERN_INFO "pgd(va) [%lx] pgd (pa) [%lx] *pgd [%lx]\n", (unsigned long)pgd, __pa(pgd), pgd->pgd); 
	p4d = p4d_offset(pgd, address);
	if (p4d_none(*p4d))
					goto nul_ret;
	if (unlikely(p4d_bad(*p4d)))
					goto nul_ret;
	pud = pud_offset(p4d, address);
	if (pud_none(*pud))
					goto nul_ret;
	if (unlikely(pud_bad(*pud)))
					goto nul_ret;
	// printk(KERN_INFO "pud(va) [%lx] pud (pa) [%lx] *pud [%lx]\n", (unsigned long)pud, __pa(pud), pud->pud); 

	pmd = pmd_offset(pud, address);
	if (pmd_none(*pmd))
					goto nul_ret;
	if (unlikely(pmd_trans_huge(*pmd))){
					// printk(KERN_INFO "I am huge\n");
					goto nul_ret;
	}
	// printk(KERN_INFO "pmd(va) [%lx] pmd (pa) [%lx] *pmd [%lx]\n", (unsigned long)pmd, __pa(pmd), pmd->pmd); 
	ptep = pte_offset_map(pmd, address);
	if(!ptep){
					// printk(KERN_INFO "pte_p is null\n\n");
					goto nul_ret;
	}
	// printk(KERN_INFO "pte(va) [%lx] pte (pa) [%lx] *pte [%lx]\n", (unsigned long)ptep, __pa(ptep), ptep->pte); 
	return ptep;

	nul_ret:
				 // printk(KERN_INFO "Address could not be translated\n");
				 return NULL;

}

static int fault_hook(struct mm_struct *mm, struct pt_regs *regs, unsigned long error_code, unsigned long address)
{
    unsigned long ind, cr3;
    volatile char temp;
    pte_t *pte;
    if (address >= start_ptr && address < end_ptr) {
        ind = (address - start_ptr) >> 12;
        page_infos[ind].tlb_misses += 1;
        tlb_misses += 1;
        // For command 2
        if (page_infos[ind].unused == 1) {
            page_infos[ind].unused = 0;
            unused -= 1;
        }
        if (error_code & (0x1UL << 1)) {
            // Write operation
            if (page_infos[ind].writewss == 0) {
                // First time write.
                if (page_infos[ind].readwss > 0) {
                    // A read was made before.
                    readwss -= 1;
                }
                writewss += 1;
            }
            page_infos[ind].writewss += 1;
        } else {
            // Read operation
            if (page_infos[ind].writewss == 0 && page_infos[ind].readwss == 0) {
                // First read access to page and no write before.
                readwss += 1;
            }
            page_infos[ind].readwss += 1;
        }
        pte = page_infos[ind].pte;
        pte->pte &= ~(0x1UL << 50);
        // Access the page table entry now so it is in the TLB.
        if (command == 1) {
            // PTI enabled. change the ASID to that of user.
            __asm__ __volatile__(
                    "mov %%cr3, %0;"
                    :"=r" (cr3)
                    :
                    :"memory"
                    );
            cr3 |= (1 << X86_CR3_PTI_PCID_USER_BIT);
            __asm__ __volatile__(
                    "mov %0, %%cr3;"
                    :
                    :"r" (cr3)
                    :"memory"
                    );
            temp = *((char*)address);
            *((char*)address) = temp;
            cr3 &= ~(1 << X86_CR3_PTI_PCID_USER_BIT);
            __asm__ __volatile__(
                    "mov %0, %%cr3;"
                    :
                    :"r" (cr3)
                    :"memory"
                    );
        } else if (command == 2) {
            __native_flush_tlb();
            if (error_code & (0x1UL << 1)) {
                // In case of a write, do write access
                // so that dirty bit is set.
                temp = *((char*)address);
                *((char*)address) = temp;
            } else {
                // In case of read do read access.
                temp = *((char*)address);
            }
        } else {
            // command is 0.
            // Do a read and then write with same value
            // so that dirty bit is set and we do not get wrong
            // miss count.
            temp = *((char*)address);
            *((char*)address) = temp;
        }
        // Repoison the page table entry
        pte->pte |= (0x1UL << 50);
        return 0;
    }
    return -1;
}
ssize_t handle_read(char *buff, size_t length)

{
	 /*TODO Read handler*/
	unsigned long cr3;
	unsigned long i,j,max;
	unsigned long vma;
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma_st = find_vma(mm, gptr);
	unsigned long start_addr = vma_st->vm_start;
	unsigned long last_addr = vma_st->vm_end;
	// printk(KERN_INFO "Entered handle_read");
	__asm__ __volatile__("mov %%cr3, %0;"
														:"=r" (cr3)
														:
														:"memory");
	struct read_command *cmd;
	cmd = (struct read_command*)buff;
	//==========================================================================================
	// printk(KERN_INFO "Process pid [%d] cr3 [%lx]\n", current->pid, cr3);
	if(cmd.command == FAULT_START){
		for(i=start_addr; i < last_addr; i +=4096){
			total_pgs++; //counting the total number of pages
		}		
	}	
	nr_pages = count_vma_pages_range(mm, start_addr, last_addr - start_addr);
	//Memory allocation for all required arrays
	tlbmisses_cnt_pgs = kmalloc(total_pgs*sizeof(unsigned long), GFP_KERNEL);
	writes_cnt_pgs = kmalloc(total_pgs*sizeof(unsigned long), GFP_KERNEL);
	reads_cnt_pgs = kmalloc(total_pgs*sizeof(unsigned long), GFP_KERNEL);
	unused_cnt_pgs = kmalloc(total_pgs*sizeof(unsigned long), GFP_KERNEL);
	tlb_toppers = kmalloc(5*sizeof(unsigned long), GFP_KERNEL);
	write_toppers = kmalloc(5*sizeof(unsigned long), GFP_KERNEL);
	read_toppers = kmalloc(5*sizeof(unsigned long), GFP_KERNEL);



	if(cmd.command == FAULT_START){
		for(i=0; i< total_pgs; i++){
			tlbmisses_cnt_pgs[i]=-1;
			writes_cnt_pgs[i] = 0;
			reads_cnt_pgs[i] = 0;
			unused_cnt_pgs[i] = 0;
		}
		for (i = 0; i < 5; ++i)
		{
			tlb_toppers[i] = -1;
			write_toppers[i] = -1;
			read_toppers[i] = -1;
		}
		for(i=start_addr; i < last_addr; i +=4096){
			// total_pgs++; //counting the total number of pages
			gpte = get_pte(i, &vma);
			gpte->pte |= 0x1UL << 50;
		}
		for(i=0; i< total_pgs; i++){
			printk(KERN_INFO "[FAULT_START]tlbmisses for page %ld is %ld\n", i, tlbmisses_cnt_pgs[i]);
		}		
		return 0;
	}
	else if(cmd.command == TLBMISS_TOPPERS){
		// printk(KERN_INFO "MISS_TOPPERS\n");
		// if(command == 0){// PART 1A & 1B
			for(i=0; i< total_pgs; i++){
				printk(KERN_INFO "[TLBMISS_TOPPERS]tlbmisses for page %ld is %ld\n", i, tlbmisses_cnt_pgs[i]);
				if(tlbmisses_cnt_pgs[i] > -1)
					tlb_misses += tlbmisses_cnt_pgs[i];
			}
			//Find 5 TLB_miss toppers
			for(j=0; j < 5; j++){	
				max=0;
				for (i = 0; i < total_pgs; ++i){
					if(j>=1 && i == tlb_toppers[0])
						continue;
					if(j>=2 && i == tlb_toppers[1])
						continue;
					if(j>=3 && i == tlb_toppers[2])
						continue;
					if(j>=4 && i == tlb_toppers[3])
						continue;
					if(tlbmisses_cnt_pgs[i]>=max){
						max = tlbmisses_cnt_pgs[i];
						tlb_toppers[j] = i;
					}
				}
				if(tlbmisses_cnt_pgs[tlb_toppers[j]] == -1){
					cmd.valid_entries = j;
					break;
				}
				cmd.toppers[j].count = tlbmisses_cnt_pgs[tlb_toppers[j]];
				cmd.toppers[j].vaddr = gptr + 4096*tlb_toppers[j];
			}	
		// }
			// stac();
			// memcpy(&cmd, buff, sizeof(cmd));
			// clac();
			return 5;
	}


	return 0;
}


ssize_t handle_write(const char *buff, size_t lenth)
{
	 /*TODO Write handler*/
	stac();
	gptr = *((unsigned long *)buff);
	clac();
	// printk(KERN_INFO "In write %lx\n", gptr);
	return 8;

}

int handle_open(void)

{
	/*TODO open handler*/
	//Resetting global values to 0 for every new user-program
	// unsigned long vma,i;
	// struct mm_struct *mm = current->mm;
	// struct vm_area_struct *vma_st = find_vma(mm, gptr);
	// unsigned long start_addr = vma_st->vm_start;
	// unsigned long last_addr = vma_st->vm_end;
	tlb_misses=0;writewss=0;readwss=0;unused=0;total_pgs=0;
	
	// for(i=start_addr; i < last_addr; i +=4096){
	// 	printk(KERN_INFO "Total number of pages [%ld], from [%lx] to [%lx]\n", total_pgs, start_addr, last_addr);
	// 	total_pgs++; //counting the total number of pages
	// }
	
	page_fault_pid = current->pid;
	rsvd_fault_hook = &fault_hook;

	return 0;
}

int handle_close(void)

{
	/*TODO open handler*/
	struct mm_struct *mm2 = current->mm;
	struct vm_area_struct *vma_st = find_vma(mm2, gptr);
	unsigned long start_addr = vma_st->vm_start;
	unsigned long last_addr = vma_st->vm_end;
	unsigned long i,j,max, vma;
	printk(KERN_INFO "Total number of pages [%ld], from [%lx] to [%lx]\n", total_pgs, start_addr, last_addr);
	page_fault_pid = -1;
	rsvd_fault_hook = NULL;
	for(i=start_addr; i < last_addr; i +=4096){
		gpte = get_pte(i, &vma);
		gpte->pte &= ~(0x1UL << 50); 
	}

	/*Update all info before exiting*/
if(command == 2){// PART 3A 
		for(i=0; i< total_pgs; i++){
			if(writes_cnt_pgs[i] != 0)
				writewss ++;

			if(reads_cnt_pgs[i] != 0)
				readwss++;

			if(writes_cnt_pgs[i] == 0 && reads_cnt_pgs[i] == 0){
				unused++;
				unused_cnt_pgs[i]=1;
			}
		}
		for(j=0; j < 5; j++){	//5 write toppers
			max=0;
			for (i = 0; i < total_pgs; ++i){
				if(j>=1 && i == write_toppers[0])
					continue;
				if(j>=2 && i == write_toppers[1])
					continue;
				if(j>=3 && i == write_toppers[2])
					continue;
				if(j>=4 && i == write_toppers[3])
					continue;
				if(writes_cnt_pgs[i]>=max){
					max = writes_cnt_pgs[i];
					write_toppers[j] = i;
				}
			}
		}	
		for(j=0; j < 5; j++){	// 5 read toppers
			max=0;
			for (i = 0; i < total_pgs; ++i){
				if(j>=1 && i == read_toppers[0])
					continue;
				if(j>=2 && i == read_toppers[1])
					continue;
				if(j>=3 && i == read_toppers[2])
					continue;
				if(j>=4 && i == read_toppers[3])
					continue;
				if(reads_cnt_pgs[i]>=max){
					max = reads_cnt_pgs[i];
					read_toppers[j] = i;
				}
			}
		}	
		printk(KERN_INFO "reads [%ld] writes [%ld] unused [%ld]\n", readwss, writewss, unused);
	}
	kfree(tlbmisses_cnt_pgs);
	kfree(writes_cnt_pgs);
	kfree(reads_cnt_pgs);
	kfree(unused_cnt_pgs);
	kfree(tlb_toppers);
	kfree(write_toppers);
	kfree(read_toppers);
	return 0;
}

