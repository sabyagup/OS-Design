/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file.H"
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File() {
    /* We will need some arguments for the constructor, maybe pointer to disk
     block with file management and allocation data. */
    Console::puts("In file constructor.\n");
	//fill in the initial variables 
	fd = -1;
    size = 0;
    curr_block = -1;
    idx = 1;
    pos = 0;
    file_system = NULL;
    
	//assert(false);
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char * _buf) {
    Console::puts("reading from file\n");

    int read = 0;
    int bytes_to_read = _n;
    
    char buf[512];
    memset(buf, 0, 512);        //setting the buffer to read the disk.
    Console::puts("Reading block ");

    file_system->disk->read(curr_block, (unsigned char *)buf); //accessing read function of the disk

    while (!EoF() && (bytes_to_read > 0)) {  //intiating loop to read the data
        _buf[read] = buf[pos];
        bytes_to_read--;
        read++; 
        pos++;
        if (pos >= 512) {
            idx++;            
            if (idx > 15) {
                return read;
            }
            curr_block = blck[idx-1];
            memset(buf, 0, 512);        //set the buffer to 0, to be used in reading the disk.
            file_system->disk->read(curr_block, (unsigned char *)buf);
            pos = 0;
        }
        
    }
    //Console::puts("Read bytes = ");Console::puti(read);Console::puts("\n");
    return read;
	
    //assert(false);
}


void File::Write(unsigned int _n, const char * _buf) {
    Console::puts("writing to file\n");
    int write = 0;
    int bytes_to_write = _n;
    unsigned char buf[512];
    memset(buf, 0, 512);        //set the buffer to read the disk.
    //read the values first then write
    file_system->disk->read(curr_block, buf);

    while (bytes_to_write > 0 ) {
        buf[pos] = _buf[write];
        write++;
        pos++;
        bytes_to_write--;
        if (pos >= 512) {
            file_system->disk->write(curr_block, (unsigned char *)buf);
            curr_block = file_system->GetBlock();
            file_system->UpdateBlockData(fd, curr_block);
            memset(buf, 0, 512);
            file_system->disk->read(curr_block, (unsigned char *)buf);
            pos = 0;
        }
    }

    file_system->UpdateSize(write, fd, this);
    file_system->disk->write(curr_block, buf);
    //assert(false);
}

void File::Reset() {
    Console::puts("reset current position in file\n");
    pos = 0;
    curr_block  = blck[0];
	//assert(false);
    
}

void File::Rewrite() {
    Console::puts("erase content of file\n");
	//first we erase the file then rewrite it to zero
	file_system->EraseFile(fd);
    for (int i = 1; i < 16; i++) {
        blck[i] = 0;
    }
	
    //assert(false);
}

bool File::EoF() {
    Console::puts("testing end-of-file condition\n");
	char buf[512];
    memset(buf, 0, 512);
	if ( ( ((idx - 1)*512) + pos ) > size ) //checking if the postion reached the end or not
        return true;

    return false;
	
    //assert(false);
}
