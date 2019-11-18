/*
    File: page_table.C

    Author: Sabyasachi Gupta
            Texas A&M University
    Date  : 02/21/19

    Description: Basic Paging.

*/

#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
   //assert(false);
   //Initialized all the required variables
   PageTable::kernel_mem_pool = _kernel_mem_pool;
   PageTable::process_mem_pool = _process_mem_pool;
   PageTable::shared_size = _shared_size;
   
   Console::puts("Initialized Paging System\n");
}

// bit 0 - page present/ absent
// bit 1 - rd / rd&wrt
// bit 2 - supervisor / user mode

PageTable::PageTable()
{
   //assert(false);
   page_directory = ( unsigned long*)(process_mem_pool->get_frames(1)*PAGE_SIZE);
   unsigned long* page_table= (unsigned long*)(process_mem_pool->get_frames(1)*PAGE_SIZE);
   unsigned long shrd_frms = ( PageTable::shared_size / PAGE_SIZE); //number of entries need to intialized in PT at begining
   unsigned long addr = 0; // holds the physical address of where a page is
   unsigned int i;
   
   // mapping the first 4MB of memory in page table
	for(i=0; i<shrd_frms; i++) {
		page_table[i] = addr | 3; // attribute set to: supervisor level, read/write, present(011 in binary)
		addr = addr + PAGE_SIZE; // page size = 4kb
	}
	
	// filling the first entry of the page directory
	page_directory[0] = (unsigned long)page_table | 3; // attribute set to: supervisor level, read/write, present(011 in binary)
	
	//setting rest of the entries in PD with zero address
	for(i=1; i<shrd_frms; i++) {
		page_directory[i] = 0 | 2; // attribute set to: supervisor level, read/write, not present(010 in binary)
	}
	page_directory[shrd_frms-1] = (unsigned long)( page_directory ) | 3 ;

	vm_pool_cnt = 0;
	for(int i = 0 ; i < VM_POOL_SIZE; i++) {
        vm_pool_arr[i] = NULL;
    }
	
   Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
   //assert(false);
   current_page_table = this; //setting the current page table object
   write_cr3((unsigned long)page_directory); //setting cr3 reg with page dir address
   
   Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
   //assert(false);
   paging_enabled = 1;
   write_cr0(read_cr0() | 0x80000000); //reading and setting cr0 reg to 1
   
   Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
  //assert(false);
  unsigned long page_addr = read_cr2();
  unsigned long PD_num = page_addr >> 22;
  unsigned long PT_num = page_addr >> 12;
  unsigned long error_code = _r->err_code;
  
  unsigned long * curr_pg_dir = (unsigned long *) 0xFFFFF000;
  unsigned long * new_page_table = NULL;
  
  //first 10 bits for PD, next 10 bits for PT and last 12 bit is offset  
  
  if ((error_code & 1) == 0 ) {
	  // checking for legitimate address
	  int idx = -1;
      VMPool ** vm_pool = current_page_table->vm_pool_arr;
      for (int i = 0; i < current_page_table->vm_pool_cnt; i++) { //iterating through every vm pool
          if (vm_pool[i] != NULL) {
              if (vm_pool[i]->is_legitimate(page_addr)) {
                  idx = i;
                  break;
              }
          }
      }
	  
	  if ((curr_pg_dir[PD_num] & 1 ) == 1) { //fault in page table
		  //new_page_table = (unsigned long *)(curr_pg_dir[PD_num] & 0xFFFFF000); //traversing to the given page
		  new_page_table = (unsigned long *)(0xFFC00000 | (PD_num << 12)); //setting first 10 bit as 1023
		  new_page_table[PT_num & 0x03FF] =  PageTable::process_mem_pool->get_frames(1)*PAGE_SIZE | 3; // setting the page with 011 config
		  
	  } else {
		  curr_pg_dir[PD_num] = (unsigned long)(process_mem_pool->get_frames(1)*PAGE_SIZE | 3); //creating a directory entry
		  //new_page_table = (unsigned long *)(curr_pg_dir[PD_num] & 0xFFFFF000);
		  new_page_table = (unsigned long *)(0xFFC00000 | (PD_num << 12)); //setting first 10 bit as 1023
		  
		  for (int i = 0; i<1024; i++) {
			  new_page_table[i] = 0 | 4 ; // marking pages as user mode
			}
		  new_page_table[PT_num & 0x03FF] =  PageTable::process_mem_pool->get_frames(1)*PAGE_SIZE | 3; //marking the specified page with 011
	  }
	}

  Console::puts("handled page fault\n");
}

void PageTable::register_pool(VMPool * _vm_pool)
{
    //assert(false);
	//checking if VM Pool limit is reached or not
	if (vm_pool_cnt < VM_POOL_SIZE) {
		vm_pool_cnt++;
        vm_pool_arr[vm_pool_cnt] = _vm_pool;
		Console::puts("registered VM pool\n");
    }
    Console::puts("No space in VM POOL"); 
}

void PageTable::free_page(unsigned long _page_no) {
    //assert(false);
	//getting the first 10 and 20 bits
	unsigned long PD_num   = _page_no >> 22;
    unsigned long PT_num   = _page_no >> 12;

    unsigned long * page_table = (unsigned long *) (0xFFC00000 | (PD_num << 12));
    //calling release_frames for the given page number
    unsigned long frm_no  = page_table[PT_num & 0x03FF] / (Machine::PAGE_SIZE);   
    process_mem_pool->release_frames(frm_no);
    //updating the table
    page_table[PT_num & 0x03FF] = 0 | 2 ;
	
    Console::puts("freed page\n");
}
  