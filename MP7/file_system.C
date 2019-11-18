/*
     File        : file_system.C
     Author      : Riccardo Bettati
     Modified    : 2017/05/01
     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define NODES_PER_BLOCK (512/sizeof(mng_node))

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file_system.H"


/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() {
    Console::puts("In file system constructor.\n");
    //intitializing the variables
	FileSystem::disk    = NULL;
    ttl_blcks        = 0;
    mng_blcks            = 0;
    m_nodes             = 0;
    size                = 0;
    
}

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("mounting file system form disk\n");
    if (disk == NULL) {
        disk = _disk; //setting the disk as the given disk
        return true;
    }
    return true;
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) {
    Console::puts("formatting disk\n");
	//setting the variables
    FileSystem::disk = _disk;
    FileSystem::size = _size;
    FileSystem::ttl_blcks = (FileSystem::size / 512) + 1;
    FileSystem::m_nodes = (FileSystem::ttl_blcks/ 16) + 1;
    FileSystem::mng_blcks = ((FileSystem::m_nodes * sizeof(mng_node)) / 512 ) + 1;

    // resetting the map
    for (int j = 0; j < (ttl_blcks/8); j++){
        block_map[j] = 0; 
    }
	//setting the current block to 1
    int i;
    for (i = 0; i < (mng_blcks/8) ; i++) {
        FileSystem::block_map[i] = 0xFF;
    }

    block_map[i]  = 0;
    for (int j = 0; j < (mng_blcks%8) ; j++) {
        FileSystem::block_map[i] = block_map[i] | (1 << j) ; // Set the remaining blocks to 1
    }

    char buf[512];
    memset(buf,0,512);
    for (int j = 0; j < ttl_blcks; j++) {
        disk->write(j, (unsigned char *)buf);
    }

    return true;
}

File * FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file\n");
    
    File * file = (File *) new File();
    char buf[512];
    memset(buf, 0, 512);    
//reading each block and its inode file
    for(int i = 0; i < mng_blcks; i++) {
        memset(buf, 0, 512);
        disk->read(i, (unsigned char *)buf);
        mng_node* m_node_l = (mng_node *)buf;
        for (int j = 0; j < NODES_PER_BLOCK; j++) {
            if (m_node_l[j].fd == _file_id) {
                file->fd = _file_id;
                file->size = m_node_l[j].size;
                file->curr_block = m_node_l[j].block[0];
                file->idx = 1;
                file->pos = 0;

                for(int k = 0; k <16; k++) {
                    file->blck[k] = m_node_l[j].block[k];
                }
                file->file_system = FILE_SYSTEM;
                if (file->file_system == NULL)
                    Console::puts("NULL in look up");Console::puts("\n");
                Console::puts("file with id found ");Console::puti(_file_id);Console::puts("\n");
                return file;
            }
        }
    }

    return NULL;
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file\n");
    char buf[512];
    memset(buf, 0, 512);

    for (int i = 0; i < mng_blcks; i++) {
		//setting up buffer
        memset(buf, 0, 512);
        disk->read (i, (unsigned char *)buf);
        mng_node* m_node_l = (mng_node *) buf;

        for (int j = 0; j < NODES_PER_BLOCK; j++) {
            if (m_node_l[j].fd == 0) {
                m_node_l[j].fd = _file_id;
                m_node_l[j].block[0] = GetBlock();
                Console::puts("get block "); Console::puti(m_node_l[j].block[0]);
                m_node_l[j].b_size   = 1;

                disk->write(i, (unsigned char *)buf);
                return true;
            }
        }
    }

    return false;
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file\n");
    
    char buf[512];
    memset(buf, 0, 512);

    for (int i = 0; i < mng_blcks; i++) {

        memset(buf, 0, 512);
        disk->read (i, (unsigned char *)buf);
        mng_node* m_node_l = (mng_node *) buf;

        for (int j = 0; j < NODES_PER_BLOCK; j++) {
            if (m_node_l[j].fd == _file_id) {
                m_node_l[j].fd = 0;
                m_node_l[j].size = 0;
                m_node_l[j].b_size = 0;
                for (int k = 0; k < 16; k++) {
                    if (m_node_l[j].block[k] != 0) {
                        FreeBlock(m_node_l[j].block[k]);
                    }
                    m_node_l[j].block[k] = 0;
                }
                disk->write(i, (unsigned char *)buf);
                return true;
            }
        }
    }
    Console::puts("File Not found, check id \n");
    return false;
}

void FileSystem::EraseFile(int _file_id) {
    Console::puts("Erasing File Content \n");

    char buf[512];
    char buf_2[512];
    memset(buf, 0, 512); 
    memset(buf_2, 0, 512);

    for (int i = 0; i < mng_blcks; i++) {

        memset(buf, 0, 512);
        disk->read (i, (unsigned char *)buf);
        mng_node* m_node_l = (mng_node *) buf;

        for (int j = 0; j < NODES_PER_BLOCK; j++) {
            if (m_node_l[j].fd == _file_id) {
                
                m_node_l[j].size = 0;
                m_node_l[j].b_size = 0;
                for (int k = 0; k < 16; k++) {
                    if (m_node_l[j].block[k] != 0) {
                        disk->write(m_node_l[j].block[k], (unsigned char *)buf_2);
                        if (k!=0) {             // Dont free the first block of the file. Just erase the content.
                            FreeBlock(m_node_l[j].block[k]);
                            m_node_l[j].block[k] = 0;
                        }
                    }
                    
                }
                disk->write(i, (unsigned char *)buf);
                return;
            }
        }
    }

}


int FileSystem::GetBlock() {
    Console::puts("Total blocks "); Console::puti(ttl_blcks/8);Console::puts("\n");

    for (int i = 0; i < (ttl_blcks / 8); i++) {
        if (block_map[i] != 0xFF) {
            for (int j = 0; j < 8; j++) {
                if (block_map[i] & (1 << j)) {
                    continue;
                } else {
                    block_map[i] = block_map[i] | (1 << j);
                    int b= j + i*8;
                    Console::puts("Allocating block number");Console::puti(b);Console::puts("\n");
                    return b;
                }
            }
        }
    }
    Console::puts("returning block 0\n");
    return 0;
}

void FileSystem::FreeBlock(int block_no) {
    
    int node = block_no / 8;
    int idx = block_no % 8;
//freeing and updating the blocks
    block_map[node] = block_map[node] | (1 << idx) ;
    block_map[node] = block_map[node] ^ (1 << idx) ;
}

void FileSystem::UpdateSize(long size, unsigned long fd, File *file) {

    Console::puts("Updating the block size \n");
    char buf[512];
    memset(buf, 0, 512);

    for (int i = 0; i < mng_blcks; i++) {

        memset(buf, 0, 512);
        disk->read (i, (unsigned char *)buf);
        mng_node* m_node_l = (mng_node *) buf;

        for (int j = 0; j < NODES_PER_BLOCK; j++) {
            if (m_node_l[j].fd == fd) {
                
                m_node_l[j].size += size;
                file->size = m_node_l[j].size;
                disk->write(i, (unsigned char *)buf);
                return;
            }
        }
    }
    Console::puts("File with this fd not found for size update\n");
    return;
}

void FileSystem::UpdateBlockData(int fd, int block) {

    Console::puts("Updating the block data \n");
    char buf[512];
    memset(buf, 0, 512);

    for (int i = 0; i < mng_blcks; i++) {
        memset(buf, 0, 512);
        disk->read (i, (unsigned char *)buf);
        mng_node* m_node_l = (mng_node *) buf;

        for (int j = 0; j < NODES_PER_BLOCK; j++) {
            if (m_node_l[j].fd == fd) {
                
                m_node_l[j].b_size += 1;
                m_node_l[j].block[m_node_l[j].b_size] = block;

                disk->write(i, (unsigned char *)buf);
                return;
            }
        }
    }
    Console::puts("File with this fd not found for block update\n");
    return;
}