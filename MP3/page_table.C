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
   page_directory = ( unsigned long*)(kernel_mem_pool->get_frames(1)*PAGE_SIZE);
   unsigned long* page_table= (unsigned long*)(kernel_mem_pool->get_frames(1)*PAGE_SIZE);
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
  
  unsigned long * curr_pg_dir = (unsigned long *) read_cr3();
  unsigned long * new_page_table = NULL;
  
  //first 10 bits for PD, next 10 bits for PT and last 12 bit is offset  
  
  if ((error_code & 1) == 0 ) {
	  
	  if ((curr_pg_dir[PD_num] & 1 ) == 1) { //fault in page table
		  new_page_table = (unsigned long *)(curr_pg_dir[PD_num] & 0xFFFFF000); //traversing to the given page
		  new_page_table[PT_num & 0x03FF] =  PageTable::process_mem_pool->get_frames(1)*PAGE_SIZE | 3; // setting the page with 011 config
		  
	  } else {
		  curr_pg_dir[PD_num] = (unsigned long)(kernel_mem_pool->get_frames(1)*PAGE_SIZE | 3); //creating a directory entry
		  new_page_table = (unsigned long *)(curr_pg_dir[PD_num] & 0xFFFFF000);
		  
		  for (int i = 0; i<1024; i++) {
			  new_page_table[i] = 0 | 4 ; // marking pages as user mode
			}
		  new_page_table[PT_num & 0x03FF] =  PageTable::process_mem_pool->get_frames(1)*PAGE_SIZE | 3; //marking the specified page with 011
	  }
	}

  Console::puts("handled page fault\n");
}

