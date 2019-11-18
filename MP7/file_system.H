/* 
    File: file_system.H
    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 10/04/05
    Description: Simple File System.
    
*/

#ifndef _FILE_SYSTEM_H_                   // include file only once
#define _FILE_SYSTEM_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define MB * (0x1 << 20)

#define DISK_SIZE   (5 MB)
#define MAX_BLOCKS (DISK_SIZE / 512)

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "file.H"
#include "simple_disk.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/
//inode structure for each file
typedef struct node {
    unsigned long fd;
    unsigned long block[16];
    unsigned long size;
    unsigned long b_size;    
}mng_node;

/*--------------------------------------------------------------------------*/
/* FORWARD DECLARATIONS */ 
/*--------------------------------------------------------------------------*/

class File;

/*--------------------------------------------------------------------------*/
/* F i l e S y s t e m  */
/*--------------------------------------------------------------------------*/

class FileSystem {

friend class File; /* -- not sure if we need this; feel free to delete */

private:
     /* -- DEFINE YOUR FILE SYSTEM DATA STRUCTURES HERE. */
     
    SimpleDisk * disk;          //Pointer to the disk being mounted on this filesystem
    unsigned char block_map[512];   //block map for this filesystem
    unsigned long ttl_blcks;  //total blocks 
    unsigned long mng_blcks;     //management blocks
    unsigned long m_nodes;      //management nodes 

    unsigned long size;
    
     
public:

    FileSystem();
    /* Just initializes local data structures. Does not connect to disk yet. */
    
    bool Mount(SimpleDisk * _disk);
    /* Associates this file system with a disk. Limit to at most one file system per disk.
     Returns true if operation successful (i.e. there is indeed a file system on the disk.) */
    
    bool Format(SimpleDisk * _disk, unsigned int _size);
    /* Wipes any file system from the disk and installs an empty file system of given size. */
    
    File * LookupFile(int _file_id);
    /* Find file with given id in file system. If found, return the initialized
     file object. Otherwise, return null. */
    
    bool CreateFile(int _file_id);
    /* Create file with given id in the file system. If file exists already,
     abort and return false. Otherwise, return true. */
    
    bool DeleteFile(int _file_id);
    /* Delete file with given id in the file system; free any disk block occupied by the file. */
	// method to make the job easy
    int GetBlock(); //get the block
	
    void FreeBlock(int block_no);
	
    void UpdateSize(long size, unsigned long fd, File *file);
	
    void EraseFile(int _file_id);
	
    void UpdateBlockData(int fd, int block);
   
};
#endif