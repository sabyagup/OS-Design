/*
    File: vm_pool.C

    Author: Sabyasachi Gupta
            Texas A&M University
    Date  : 03/20/19

    Description: Creation of VMPool.

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "page_table.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {
    //assert(false);
	base_addr = _base_address;
	size = _size;
	frame_pool = _frame_pool;
	page_table = _page_table;
	//intializing the struct array
	reg_no = 0;
    //reg_info = (reg_info_ *)(Machine::PAGE_SIZE * (frame_pool->get_frames(1)));
    reg_info = (struct reg_info_ *) (base_addr);
    page_table->register_pool(this);
	
    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
    //assert(false);
	// checking valid size for allocation
	unsigned long strt_addr;
    if (size == 0){ 
        Console::puts("invalid to allocate");
        return 0;
    }
    assert(reg_no < MAX_REGIONS); //max region reached
	//no of frames needed 
	unsigned b = _size % (Machine::PAGE_SIZE) ;
    unsigned long frames = _size / (Machine::PAGE_SIZE) ;
    if (b > 0)
        frames++;
    //leaving the first frame in reg 0
    if (reg_no == 0) {
        strt_addr = base_addr;
        reg_info[reg_no].base_addr  = strt_addr + Machine::PAGE_SIZE ; //updating the struct array
        reg_info[reg_no].size = frames*(Machine::PAGE_SIZE) ; //updating the struct array
        reg_no++;
        return strt_addr + Machine::PAGE_SIZE;
    } else {
        strt_addr = reg_info[reg_no - 1].base_addr + reg_info[reg_no - 1].size ; //updating the struct array
    }

    reg_info[reg_no].base_addr  = strt_addr; //updating the struct array
    reg_info[reg_no].size = frames*(Machine::PAGE_SIZE); //updating the struct array

    reg_no++;

    return strt_addr;
	
    Console::puts("Allocated region of memory.\n");
}

void VMPool::release(unsigned long _start_address) {
    //assert(false);
	int cur_reg_no = -1;
    // finding which region the address is located
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (reg_info[i].base_addr == _start_address) {
            cur_reg_no = i;
            break;
        }
    }
    //number of pages need to be freed
    unsigned int alloc_pages = ( (reg_info[cur_reg_no].size) / (Machine::PAGE_SIZE) ) ;
    //calling the free_page function for each page
    for (int i = 0 ; i < alloc_pages ;i++) {
        page_table->free_page(_start_address);
        _start_address += Machine::PAGE_SIZE;
    }
    //updating the region info array
    for (int i = cur_reg_no; i < reg_no - 1; i++) {
        reg_info[i] = reg_info[i+1];
    }
    reg_no--;
    // refreshing the TLB
    page_table->load();
	
    Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
    //assert(false);
	//iteratating through every region
	for(unsigned long i = 0; i < this->reg_no; i++) {
		//checking the region limit
		int len = this->reg_info[i].base_addr + this->reg_info[i].size;
		int strt = this->reg_info[i].base_addr;
		if ((_address < len) && (_address >= strt)) {
			return true;
		}
	}
    return false;
	
    Console::puts("Checked whether address is part of an allocated region.\n");
}

