
#include "sfs_api.h"
#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fuse.h>
#include <strings.h>
#include <semaphore.h>
#include "disk_emu.h"
#define ZHOU_YIJIE_DISK "sfs_disk.disk"
#define NUM_BLOCKS 1024  //maximum number of data blocks on the disk.
#define BITMAP_ROW_SIZE (NUM_BLOCKS/8) // this essentially mimcs the number of rows we have in the bitmap. we will have 128 rows. 

/* macros */
#define FREE_BIT(_data, _which_bit) \
    _data = _data | (1 << _which_bit)

#define USE_BIT(_data, _which_bit) \
    _data = _data & ~(1 << _which_bit)

#define max(a,b)(a>b?a:b)
#define min(a,b)(b<a?b:a)

#define BUFF_MUTEX "/yijiezhou_MUTEX"



//initialize all bits to high
uint8_t free_bit_map[BITMAP_ROW_SIZE] = { [0 ... BITMAP_ROW_SIZE - 1] = UINT8_MAX };

int sfs_mkerror = 0;
int rootDir_ptr = 0; 
sem_t *disk_access;

inode_t inode_table[100];
file_descriptor fd_table[100];
directory_entry rootDir[100];

void force_set_index(uint32_t index) {
    
    uint32_t i = index / 8;
    uint8_t bit = index % 8;
    USE_BIT(free_bit_map[i], bit);

}
uint32_t get_index() {
    uint32_t i = 0;

    // find the first section with a free bit
    // let's ignore overflow for now...
    while (free_bit_map[i] == 0) { 
        i++;
        if(i>128){
            return 0;
        } 
    }

    // now, find the first free bit
    /*
        The ffs() function returns the position of the first (least
       significant) bit set in the word i.  The least significant bit is
       position 1 and the most significant position is, for example, 32 or
       64.  
    */
    // Since ffs has the lsb as 1, not 0. So we need to subtract
    uint8_t bit = ffs(free_bit_map[i]) - 1;

    // set the bit to used
    USE_BIT(free_bit_map[i], bit);

    //return which block we used
    return i*8 + bit;
}

void rm_index(uint32_t index) {

    // get index in array of which bit to free
    uint32_t i = index / 8;

    // get which bit to free
    uint8_t bit = index % 8;

    // free bit
    FREE_BIT(free_bit_map[i], bit);
}


//struct superblock{
//	int magic, blocksize, filesystemsize, inodetablelength, rootdir;
//} superblock;

int write_partblock(int block, const char* buffer, int byteOffset, int numBytes)
{
	
	char* readbuf = (char*) malloc(1024);
	read_blocks(block,1,readbuf);

	
    memcpy(byteOffset + readbuf, buffer, numBytes);
    
    //printf("in whelper: %s\n",readbuf);
	
	write_blocks(block,1, readbuf);
	free(readbuf);
} 


int write_helper(const char* buffer, int numBytes, int* blockNums, int offset, int start)
{
    int x;
    x = start+1; 
	int writtenBytes =0; 
	int remainingBytes, veryLastBytes;

	
    char* bufptr = (char*) malloc(numBytes);
    


	memcpy(bufptr,buffer, numBytes);


	
	remainingBytes = min(numBytes, 1024 - offset);


	write_partblock(blockNums[start], bufptr, offset, remainingBytes);
	writtenBytes = writtenBytes + remainingBytes;
	bufptr = bufptr + remainingBytes;


	while(1024 < (numBytes - writtenBytes))
	{
		write_blocks(blockNums[x],1, bufptr);
		
	
		x++;
		bufptr = bufptr + 1024;
		writtenBytes = writtenBytes + 1024;
	}
    veryLastBytes = numBytes - writtenBytes;
	if( veryLastBytes > 0)
	{
		write_partblock(blockNums[x], bufptr, 0, veryLastBytes);
		writtenBytes = writtenBytes + veryLastBytes;
	}
	return writtenBytes;
}



void mksfs(int fresh) {
    
    if(fresh)//new sfs or not
	{
        disk_access = sem_open(BUFF_MUTEX,O_CREAT,S_IRUSR | S_IWUSR, 1);
        //printf("start a fresh sfs\n");
		sfs_mkerror = init_fresh_disk(ZHOU_YIJIE_DISK, 1024, NUM_BLOCKS);
        //superblock
        //printf("sfs_mkerror is %d\n",sfs_mkerror);
        sem_wait(disk_access);
        struct superblock_t s;
        s.magic = 0xACBD0005;
        s.block_size = 1024;
        s.fs_size = NUM_BLOCKS;
        s.inode_table_len = 30;
        s.root_dir_inode = 0;
        char* superblock_buffer = (char*) malloc(sizeof(superblock_t));
        memcpy(superblock_buffer,&s,sizeof(superblock_t));
        
       

        int superblockerror = write_blocks(0,1,superblock_buffer);

        //char* readbuf = (char*) malloc(sizeof(superblock_t));
        //read_blocks(0,1,readbuf);

        //struct superblock_t buffer;
        //memcpy(&buffer,readbuf,sizeof(superblock_t));
        //printf("len is %d\n",buffer.inode_table_len);

        //printf("superblk write error is %d\n",superblockerror);

        free(superblock_buffer);
        force_set_index(0);

        //inode
        
         
        for(int i =0;i<100;i++){
            inode_table[i].mode = 1;
            inode_table[i].link_cnt = 0;
            inode_table[i].uid = 0;
            inode_table[i].gid = 0;
            inode_table[i].size = 0;
            inode_table[i].indirectPointer = 0;
            for(int j =0;j<12;j++){
                inode_table[i].data_ptrs[j] = 0;
            }
        }
        

        write_blocks(1,8,&inode_table);//7200 bytes

        //inode_t read[100];
        //char* readbuf = (char*)malloc(8192);

        //read_blocks(1,8,readbuf);
        //memcpy(&read,readbuf,7200);
        //printf("mode is %d\n",read[5].mode);


        for(int k = 1;k<32;k++){
            force_set_index(k);
        }
        
        
        //fd table
        
        for (int a = 0; a<100;a++){
            fd_table[a].inodeIndex = a;
            //fd_table[a].inode = -1;
            fd_table[a].rwptr = 0;
        }
        
        
        //rootDir
        for(int b = 0; b<100;b++){
            rootDir[b].num = b;
            strcpy(rootDir[b].name,"");
            
        }
        //printf("rootdir size is: %d\n",sizeof(rootDir));


        write_blocks(33,2,&rootDir);
        force_set_index(33);
        force_set_index(34);
        
        

        //bitmap
        force_set_index(32);
        //printf("free bit map size is : %d\n",sizeof(free_bit_map));
        write_blocks(32,1,&free_bit_map);


        sem_post(disk_access);



	}

	else//exist
	{
        
        //printf("open old disk\n");
        sfs_mkerror = init_disk(ZHOU_YIJIE_DISK, 1024, NUM_BLOCKS);
        //read superblk inode rootdir bitmap
        char* readbuf = (char*) malloc(sizeof(superblock_t));
        read_blocks(0,1,readbuf);

        struct superblock_t s;
        memcpy(&s,readbuf,sizeof(superblock_t));
        free(readbuf);
        //printf("len is %d\n",buffer.inode_table_len);

        //inode_t read[100];
        readbuf = (char*)malloc(8192);
        read_blocks(1,8,readbuf);
        memcpy(&inode_table,readbuf,7200);
        free(readbuf);

        readbuf = (char*)malloc(8192*2);
        read_blocks(33,2,readbuf);
        memcpy(&rootDir,readbuf,sizeof(rootDir));
        free(readbuf);

        readbuf = (char*)malloc(8192);
        read_blocks(32,1,readbuf);
        memcpy(&free_bit_map,readbuf,sizeof(free_bit_map));
        free(readbuf);

        //printf("mode is %d\n",read[5].mode);
        
		

	}
    //return sfs_mkerror;
    
    
}
int sfs_getnextfilename(char *fname){
    
    sem_wait(disk_access);

    rootDir_ptr = rootDir_ptr%100;
    if(rootDir_ptr == 99){
        rootDir_ptr = -1;
    }

    if(strcmp("", rootDir[rootDir_ptr+1].name) == 0){
        //no new file
        strcpy(fname,rootDir[rootDir_ptr].name);
        sem_post(disk_access);
        return 0;
    }else{
        strcpy(fname,rootDir[rootDir_ptr].name);
        rootDir_ptr++;
        sem_post(disk_access);
        return rootDir_ptr;
    }
}
int sfs_getfilesize(const char* path){
    sem_wait(disk_access);
    
    for(int i = 0;i<100;i++){
        if (strcmp(rootDir[i].name,path) == 0){

            sem_post(disk_access);
            return inode_table[rootDir[i].num].size;
            
        }
        
    }
    printf("fname not found, couldn't return size, return -1\n");
    sem_post(disk_access);
    return -1;
    
}
int sfs_fopen(char *name){
    //check filename length 
    int islegal = 0;
    
    //printf("char is %c\n",*(name+8));
    if(strlen(name)<21){
        islegal = 1;
    }

    if(islegal == 0){
        printf("exceed maximum name length, return -1\n");
        return -1;
    }
    int periodptr = -1;
    for(int i =0;i<20;i++){
        if(*(name + i)=='.'){
            periodptr = i;
        }
    }
    if(periodptr == -1){
        printf("no extension, return -1\n");
        return -1;
    }
    int ext =0;
    for(int i = periodptr;i<20;i++){
        if(*(name+i) == '\0'){
            break;
        }
        ext++;
    }
    if(ext>4){
        printf("illegal extension, ext has a length of %d, return -1\n",ext-1);
        return -1;
    }
    sem_wait(disk_access);
    
    for(int i=0;i<100;i++){
        if(strcmp(rootDir[i].name,name)==0){
            //append mode
            //printf("append mode\n");
            fd_table[rootDir[i].num].rwptr = inode_table[rootDir[i].num].size;
            //printf("pointer set to %d\n",fd_table[rootDir[i].num].rwptr);
            inode_table[rootDir[i].num].mode = 0;
            //printf("indicator bit set to %d\n",inode_table[rootDir[i].num].mode);
            sem_post(disk_access);
            return rootDir[i].num;
            //return -1;
        }
        
    }
    
    //not exist
    //printf("new file\n");
    for(int j=0;j<100;j++){
        if(strcmp(rootDir[j].name,"") == 0){
            //strcpy(rootDir[j].name, name);
            memcpy(rootDir[j].name,name,21);
            //printf("name is %s\n",rootDir[j].name);
            inode_table[rootDir[j].num].size = 0;
            inode_table[rootDir[j].num].mode = 0;
            //printf("size set to %d\n",inode_table[rootDir[j].num].size);
            //printf("indicator bit set to %d\n",inode_table[rootDir[j].num].mode);
            sem_post(disk_access);
            return rootDir[j].num;
        }
    }
    sem_post(disk_access);
    
    return -1;

}
int sfs_fclose(int fileID) {
    
    sem_wait(disk_access);

    if(inode_table[fileID].mode == 1 ){
        printf("you are closing a closed file,return -1\n");
        sem_post(disk_access);
        return -1;
    }else{
        inode_table[fileID].mode = 1;
        //printf("closed fd %d\n",fileID);
        sem_post(disk_access);
        return 0;
    }
    
    
}


int read_partblock(int block, char* buffer, int offset, int numBytes)
{
	//load block
	char* readbuf = (char*) malloc(sizeof(char) * 1024);
	read_blocks(block,1,readbuf);

    //write where need be
    //printf("in readpart, numBytes is %d\n",numBytes);
	int x = memcpy(buffer, offset + readbuf, numBytes);//strncpy or memcpy? TBD
	
	free(readbuf);

	return x;
}


int read_helper(char* buffer, int numBytes, int* blockNums, int offset, int start)
{
    int x;
    x = start + 1;
	int readBytes =0; 
	int remainingBytes, veryLastBytes;

	remainingBytes = min(numBytes, 1024 - offset);
	char* bufptr = buffer;

	//read first block
	read_partblock(blockNums[start], bufptr, offset, remainingBytes);
	readBytes = readBytes + remainingBytes;
	//augment ptr to next block
	
	bufptr = bufptr + remainingBytes;


	//write remaining blocks
	while( 1024 < numBytes - readBytes)
	{
		read_blocks(blockNums[x],1, bufptr);

		//increment to next block
		x++;
		readBytes = readBytes + 1024;
		bufptr = bufptr + 1024;
	}

	//wrote possible ending partial block
	veryLastBytes = numBytes - readBytes;
	if(0 < veryLastBytes)
	{
		read_partblock(blockNums[x], bufptr, 0, veryLastBytes);
	}

}


int sfs_fread(int fileID, char *buf, int length) {

    sem_wait(disk_access);
    //printf("rwptr: %d\n",fd_table[fileID].rwptr);
    printf("read: fd %d, length: %d\n",fileID,length);

    if(inode_table[fileID].mode == 1){
        printf("you are reading from a file that is closed or not existed\n");
        sem_post(disk_access);
        return -1;
    }
    
    int numBytes = length;
	//check for out of bounds read
	if(inode_table[fileID].size < fd_table[fileID].rwptr + numBytes)
	{
        printf("fd %d read out of bounds, return -1\n", fileID);
        sem_post(disk_access);
        return -1;
		// if so correct length
		numBytes = inode_table[fileID].size - fd_table[fileID].rwptr;
	}

	//acquire necessary datablocks numbers
    int* blockNums = (int*) malloc ( (256+12) * sizeof(int));
    int numIndPtrs = 0;
    //int numBlocksUsed = load_blocknums(fileID, blockNums);
    if(inode_table[fileID].indirectPointer == 0){
        //just direct
        for(int i = 0;i<12;i++){
            if(inode_table[fileID].data_ptrs[i] == 0){
                //dirp_ptr = i;
                break;
            }else{
                blockNums[i] = inode_table[fileID].data_ptrs[i];
            }
            //numBlocks++;
        }
    }else{
        //indirect
        //indir_flag = 1;
        //printf("read indirect blk present\n");
        for(int i = 0;i<12;i++){
            
            blockNums[i] = inode_table[fileID].data_ptrs[i];
            
        }
        int* indirectblock = (int*)malloc(1024);
        read_blocks(inode_table[fileID].indirectPointer,1,indirectblock);
        if(((inode_table[fileID].size - (12*1024))%1024) > 0){
            numIndPtrs = ((inode_table[fileID].size - (12*1024)) / 1024 )+ 1;
        }else{
            numIndPtrs = ((inode_table[fileID].size - (12*1024)) / 1024 );
        }
        memcpy((blockNums+ 12), indirectblock, sizeof(int) * numIndPtrs);
        free(indirectblock);
        //indir_ptr = numIndPtrs;
        //numBlocks = 12 + numIndPtrs;
    }
    
    int start_block = fd_table[fileID].rwptr/1024;
    //execute read
    //printf("blockNUm: %d, offset: %d, startblk: %d\n",blockNums[0],fd_table[fileID].rwptr%1024,start_block);
    //printf("numBytes is %d\n",numBytes);
    read_helper(buf, numBytes, blockNums, fd_table[fileID].rwptr%1024,start_block);
    fd_table[fileID].rwptr = fd_table[fileID].rwptr + numBytes;
    //printf("now rwptr: %d\n",fd_table[fileID].rwptr);
    sem_post(disk_access);
	return numBytes;


}
int sfs_fwrite(int fileID, const char *buf, int length) {
    //check length legit? 
    sem_wait(disk_access);
    printf("write: fd %d, length: %d\n",fileID,length);
    if(length+fd_table[fileID].rwptr>274432){
        printf("exceed maximum file size, \n");
        sem_post(disk_access);
        return -1;
    }

    if(inode_table[fileID].mode == 1){
        printf("entered fileID is closed or not existed\n");
        sem_post(disk_access);
        return -1;
    }
    
    int* blockNums  = (int*)malloc((256+12) *sizeof(int));
    //printf("numblk is %d\n",blockNums[0]);
    int numBlocks = 0;
    int numIndPtrs = 0;
    int indir_flag = 0;
    int dirp_ptr = 12;
    int indir_ptr = 0;
    //printf("start write\n");
    
    if(inode_table[fileID].indirectPointer == 0){
        //just direct
        //printf("no indirect blk\n");
        for(int i = 0;i<12;i++){
            if(inode_table[fileID].data_ptrs[i] == 0){
                dirp_ptr = i;
                break;
            }else{
                blockNums[i] = inode_table[fileID].data_ptrs[i];
            }
            numBlocks++;
        }
    }else{
        //indirect
        //printf("indirect blk present\n");
        //printf("indir is %d\n",inode_table[fileID].indirectPointer);
        indir_flag = 1;
        for(int i = 0;i<12;i++){
            
            blockNums[i] = inode_table[fileID].data_ptrs[i];
            //printf("dir is %d\n",blockNums[i]);
            
        }
        
        int* indirectblock = (int*)malloc(1024);
        
        read_blocks(inode_table[fileID].indirectPointer,1,indirectblock);
        //printf("size is %d\n",inode_table[fileID].size);
        if(((inode_table[fileID].size - (12*1024))%1024) > 0){
            numIndPtrs = ((inode_table[fileID].size - (12*1024)) / 1024 )+ 1;
        }else{
            numIndPtrs = ((inode_table[fileID].size - (12*1024)) / 1024 );
        }
        //printf("numIndPtrs is %d\n",numIndPtrs);
        memcpy((blockNums+ 12), indirectblock, numIndPtrs);
        free(indirectblock);
        indir_ptr = numIndPtrs;
        numBlocks = 12 + numIndPtrs;
    }
    
    //printf("numblk is %d\n",blockNums[0]);
    //printf("dirp_dtr is %d, and indir_ptr is %d, and numblocks is %d \n",dirp_ptr,indir_ptr,numBlocks);

	

	//check whether write will require new block
	if(numBlocks * 1024 < fd_table[fileID].rwptr + length)//more blocks required
	{
        //printf("more blk needed\n");
        int numNewBlocks = 0;
        
        if((fd_table[fileID].rwptr + length - (numBlocks * 1024))%1024 == 0){
            numNewBlocks = (fd_table[fileID].rwptr + length - (numBlocks * 1024))/1024;
        }else{
            numNewBlocks = (fd_table[fileID].rwptr + length - (numBlocks * 1024))/1024 + 1;
        }
        //printf("number of new blks needed is %d\n",numNewBlocks);
		
        int x;
        int store_size = numNewBlocks;
        int store_blk[store_size];
		for(x = 0; x< numNewBlocks; x++)
		{
            uint32_t numbuffer = get_index();
            if(numbuffer == 0){
                printf("disk is full\n");
                sem_post(disk_access);
                return -1;
            }
            //printf("blk %d is free\n",numbuffer);
			
            blockNums[numBlocks+ x] = numbuffer;
            store_blk[x]=numbuffer;
        }
        //printf("update inode pointers\n");
        if(indir_flag){
            //indir
            //printf("update indir only\n");
            int* readbuffer = (int*)malloc(1024);
            read_blocks(inode_table[fileID].indirectPointer,1,readbuffer);
            memcpy(readbuffer+indir_ptr,store_blk,sizeof(store_blk));
            write_blocks(inode_table[fileID].indirectPointer,1,readbuffer);
            free(readbuffer);

        }else{
            //dir
            //printf("update dir, indire if necessary, %d and %d\n",store_size,dirp_ptr);
            if(store_size <= (12-dirp_ptr)){
                //dir is enough
                //printf("dir is enough\n");
                for (int j=0;j<store_size;j++){
                    
                    inode_table[fileID].data_ptrs[dirp_ptr + j] = store_blk[j];
                    //printf("inode update: %d\n",inode_table[fileID].data_ptrs[dirp_ptr + j]);
                }
            }else{
                //add indir
                //printf("update dir and indir is necessary\n");
                inode_table[fileID].indirectPointer = get_index();
                //printf("obtain indir blk num: %d\n",inode_table[fileID].indirectPointer);
                int writebuffer[(store_size - 12 + dirp_ptr)];
                for (int j=0;j<store_size;j++){
                    if(j<(12-dirp_ptr)){
                        inode_table[fileID].data_ptrs[dirp_ptr + j] = store_blk[j];
                    }else{
                        writebuffer[(j-12+dirp_ptr)]=store_blk[j];
                    }
                     
                }
                
                write_blocks(inode_table[fileID].indirectPointer,1,writebuffer);
               
            }
        }
    }
    
    int start_block = fd_table[fileID].rwptr/1024;
    //printf("start_block is %d\n",start_block);

	int writtenBytes = write_helper(buf, length, blockNums, fd_table[fileID].rwptr%1024,start_block);

    if((fd_table[fileID].rwptr + length) > inode_table[fileID].size){
        
        inode_table[fileID].size = fd_table[fileID].rwptr + length;
        //printf("fd %d need to update size, new size is %d\n", fileID, inode_table[fileID].size);
    }
   

    fd_table[fileID].rwptr = fd_table[fileID].rwptr + writtenBytes;
    //printf("fd %d , updated rwptr: %d\n", fileID,fd_table[fileID].rwptr);
    


    free(blockNums);
    sem_post(disk_access);

	return writtenBytes;
}
int sfs_fseek(int fileID, int loc) {
    sem_wait(disk_access);
    printf("set fd %d rwptr to %d\n",fileID, loc);
    fd_table[fileID].rwptr = loc;
    sem_post(disk_access);
}
int sfs_remove(char *file) {

    //it's not used in test file,
    //comment out for better performance 
    //remove comment if test it.

    
    /*
    for(int i=0;i<100;i++){
        
        if(strcmp(rootDir[i].name,file)==0){
            
            fd_table[rootDir[i].num].rwptr = 0;
            
            inode_table[rootDir[i].num].mode = 1;

            int id = rootDir[i].num;
            rm_index(inode_table[id].indirectPointer);
            for (int i=0;i<12;i++){
                rm_index(inode_table[id].data_ptrs[i]);
                inode_table[id].data_ptrs[i];
            }
            strcpy(rootDir[id].name,"");
            
            return 0;
        }
        
        
    }
    return 0;
    
    */

}

