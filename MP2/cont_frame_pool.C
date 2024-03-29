/*
 File: ContFramePool.C
 
 Author: Sabyasachi Gupta
 Date  : 2/7/19
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define MB * (0x1 << 20)
#define KB * (0x1 << 10)

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

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
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

ContFramePool* ContFramePool::pool_start = NULL;
ContFramePool* ContFramePool::pool_end = NULL;

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    // Checking to see if Bitmap fit in a single frame
	
    assert(_n_frames * 2 <= FRAME_SIZE * 8);
    
    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    nFreeFrames = _n_frames;
    info_frame_no = _info_frame_no;
	ninfoframes = _n_info_frames;
    
    // If _info_frame_no is zero then we keep management info in the first
    //frame, else we use the provided frame to keep management info
    if(info_frame_no == 0) {
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
        assert(((_n_frames*2)/(8*4 KB) + ((_n_frames*2) % (8*4 KB) > 0 ? 1 : 0)) == ninfoframes);
    }
    
    // Number of frames must be "fill" the bitmap!
    assert ((nframes % 8 ) == 0);
    
    
    // Everything ok. Proceed to mark all bits in the bitmap
    for(int i=0; i*8 < _n_frames*2; i++) {
        bitmap[i] = 0x00;
    }
    
    // Mark the first frame as being used if it is being used
    if(_info_frame_no == 0) {
        bitmap[0] = 0x80;
        nFreeFrames--;
    }
    
	//Keeping track of contiguous frame allocation
	if (ContFramePool::pool_start == NULL) {
		ContFramePool::pool_start = this;
		ContFramePool::pool_end = this;
	} else {
		ContFramePool::pool_end->pool_next = this;
		ContFramePool::pool_end = this;
	}
	pool_next = NULL;
	
	
    Console::puts("Frame Pool initialized\n");
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    unsigned int ttl_frames = _n_frames;
    unsigned int frame_no = base_frame_no;
    int fr_srch = 0;
    int seq_fnd = 0;
    int a_idx = 0;
    int f_idx = 0;

	//check to see if sufficient frames are available or not
    assert(_n_frames < nFreeFrames);
    if(_n_frames > nFreeFrames) {
        Console::puts("Enough frames are not available");
    }

	//look for appropiate place
    for (unsigned int i = 0; i<nframes/4; i++) {
        unsigned char mask = 0xC0;
        for (int j = 0; j < 4; j++) {
            if((bitmap[i] & mask) == 0) {
                if(fr_srch == 1) {
					ttl_frames--;
                } else {
                    fr_srch = 1;
                    frame_no += i*4 + j;
                    a_idx = i;
                    f_idx = j;
                    ttl_frames--;
                }
            } else {
                if(fr_srch == 1) {
                    frame_no = base_frame_no;
                    ttl_frames = _n_frames;
                    a_idx = 0;
                    f_idx = 0;
                    fr_srch = 0;
                }
            }
            mask = mask>>2;
			
			//check if sequence is found?
            if (ttl_frames == 0) {
                seq_fnd = 1;
                break;
            }
        }
        if (ttl_frames == 0) {
            seq_fnd = 1;
            break;
        }
    }

    //if seq not found inform
    if (seq_fnd == 0 ) {
        Console::puts("Seq not found for length: ");Console::puti(_n_frames);Console::puts("\n");
        return 0;
    }

    //update bitmap sequence with head frame
    unsigned char hd_fr_msk = 0x80;
    unsigned char inv_mask = 0xC0;
	int updt_bm_frms = _n_frames;
    hd_fr_msk = hd_fr_msk>>(f_idx*2);
    inv_mask = inv_mask>>(f_idx*2);
    bitmap[a_idx] = (bitmap[a_idx] & ~inv_mask)| hd_fr_msk ;
    updt_bm_frms--;
	f_idx++;
    
	//update rest of the frames in bitmap
    unsigned char a_mask = 0xC0;
    a_mask = a_mask>>(f_idx*2);
    while(updt_bm_frms > 0 && f_idx < 4) {
        bitmap[a_idx] = bitmap[a_idx] | a_mask;
        a_mask = a_mask>>2;
		updt_bm_frms--;
		f_idx++;
    }
    
    for(int i = a_idx + 1; i< nframes/4; i++) {
        a_mask = 0xC0;
        for (int j = 0; j< 4 ; j++) {
            if (updt_bm_frms == 0) {
                break;
            }
            bitmap[i] = bitmap[i] | a_mask;
            a_mask = a_mask>>2;
            updt_bm_frms--;
        }
        if (updt_bm_frms ==0){
            break;
        }
    }

	//return head frame number
    if (fr_srch == 1) {
        nFreeFrames -= _n_frames;
		Console::puts("frame allocation complete");Console::puts("\n");
        return frame_no;
    } else {
        Console::puts("free frame not found ");Console::puts("\n");
        return 0;
    }
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    if (_base_frame_no < base_frame_no || base_frame_no + nframes < _base_frame_no + _n_frames) {
        Console::puts("out of range \n");
    } else {
        //remove it from free frames 
        nFreeFrames -= _n_frames;
		
		//identifying the location in array
        int ttl_bit_no = (_base_frame_no - base_frame_no)*2;
        int a_idx = ttl_bit_no / 8; // array location for frame
        int f_idx = (ttl_bit_no % 8) /2; //frame location in array

        //set the bitmap with inaccesible frames
        int frame_cnt = _n_frames;
        unsigned char ia_mask = 0x40;
        unsigned char inv_mask = 0xC0;
        ia_mask = ia_mask>>(f_idx*2);
        inv_mask = inv_mask>>(f_idx*2);
        while(frame_cnt > 0 && f_idx < 4) {
            bitmap[a_idx] = (bitmap[a_idx] & ~inv_mask) | ia_mask;
            ia_mask = ia_mask>>2;
            inv_mask = inv_mask>>2;
            frame_cnt--;
            f_idx++;
        }
		//marking for more than 1 array location
        for(int i = a_idx + 1; i< a_idx + _n_frames/4; i++) {
            ia_mask = 0x40;
            inv_mask = 0xC0;
            for (int j = 0; j< 4 ; j++) {
                if (frame_cnt == 0) {
                    break;
                }
                bitmap[i] = (bitmap[i] & ~inv_mask)| ia_mask;
                ia_mask = ia_mask>>2;
                inv_mask = inv_mask>>2;
                frame_cnt--;
            }
            if (frame_cnt ==0){
                break;
            }
        }
        
    }
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    //finding the correct pool
    ContFramePool* pool_add = ContFramePool::pool_start;
    while ( (pool_add->base_frame_no > _first_frame_no || pool_add->base_frame_no + pool_add->nframes < _first_frame_no) ) {
        if (pool_add->pool_next == NULL) {
            Console::puts("Frame not found in any pool, cannot release. \n");
            return;
        } else {
            pool_add = pool_add->pool_next;
        }
    }
    pool_add->release_frames_internal(_first_frame_no);
}

void ContFramePool::release_frames_internal(unsigned long _first_frame_no)
{
    //identifying the location in array
    int ttl_bit_no = (_first_frame_no - this->base_frame_no)*2;
    int a_idx = ttl_bit_no / 8;
    int f_idx = (ttl_bit_no % 8) /2;
	
	unsigned char* bm_ptr = this->bitmap;
	
	//reset the head frame
	
	unsigned char head_mask = 0x40;
    unsigned char rst_mask = 0xC0;
    head_mask = head_mask>>f_idx*2;
    rst_mask = rst_mask>>f_idx*2;
    if (((bm_ptr[a_idx]^head_mask)&rst_mask ) == rst_mask) {
        // head is set
        bm_ptr[a_idx] = bm_ptr[a_idx] & (~rst_mask);
        f_idx++;
        rst_mask = rst_mask>>2;
        this->nFreeFrames++;
	
	//release the rest of seq
        while (f_idx < 4) {
            if ((bm_ptr[a_idx] & rst_mask) == rst_mask) {
                bm_ptr[a_idx] = bm_ptr[a_idx] & (~rst_mask);
                f_idx++;
                rst_mask = rst_mask>>2;
                this->nFreeFrames++;
            } else {
                return;
            }
        }

        for(int i = a_idx+1; i < (this->base_frame_no + this->nframes)/4; i++ ) {
            rst_mask = 0xC0;
            for (int j = 0; j < 4 ;j++) {
                if ((bm_ptr[i] & rst_mask) == rst_mask) {
                    bm_ptr[i] = bm_ptr[i] & (~rst_mask);
                    rst_mask = rst_mask>>2;
                    this->nFreeFrames++;
                } else {
                    return;
                }
            }
        }

    } else {
        Console::puts("head frame not found \n");
    }
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
	//As we are using 2 bit, modifying the provided equation
	//Also using the method shown in kernel.c to calculate frame bit size, by adding KB and MB def
	return (_n_frames*2)/(8*4 KB) + ((_n_frames*2) % (8*4 KB) > 0 ? 1 : 0);
}

