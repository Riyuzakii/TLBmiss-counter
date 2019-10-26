### TLBmiss Counter
This linux kernel module was created as part of the course Topics in Operating Systems
**(CS730A)** Spring Semester - 2019, instructed by [Prof. Debadatta Mishra](https://www.cse.iitk.ac.in/users/deba/).



A linux kernel module to count number of TLB misses encountered during program execution.
The module is also extended to handle Page Table Isolation (a Meltdown-attack mitigation) and count the number of unused, read-only and written pages.
The base code (fault_hook.patch) was provided.

#### Build Instruction
* Patch your kernel with `fault_hook.patch`.

``` bash
 cd module && make       # build the module
```

``` bash
 sudo insmod memtrack.ko		#insert the module into the kernel
```

* The module has the following sysfs variables (**/sys/kernel/memtrack**):
    * **command**: `0` to count TLB misses w/o PTI. `1` to count TLB misses w PTI. `2` to count read-only, unused and write pages.
    * **tlb_misses**: Total number of TLB misses
    * **unused**: Number of unused pages
    * **readwss**: Number of read-only pages
    * **writewss**: Number of pages that were written to
