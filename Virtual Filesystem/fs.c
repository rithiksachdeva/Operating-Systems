#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"
#define FAT_EOC 0xFFFF

/* Superblock Data Structure (packed closely together) */
struct __attribute__ ((__packed__)) superblock {
	/* Never need negative values for blocks (use unit) */
	uint8_t signature[8];  /* 8 Bytes */
	uint16_t totalBlocks;  /* 2 Bytes */
	uint16_t rootIndex;    /* 2 Bytes */
	uint16_t dataIndex;    /* 2 Bytes */
	uint16_t totalData;    /* 2 Bytes */
	uint8_t totalFAT;      /* 1 Byte */
	uint8_t padding[4079];  /* 4079 Bytes*/
};

/* File Allocation Table Data Structure */
struct __attribute__ ((__packed__)) FATblock {
	uint16_t entries[2048];      /* 2 x 2048 = 4096 Bytes */
	struct FATblock* continued;  /* If total data blocks > 2048 */
};

/* Root Directory Data Structure */
struct __attribute__ ((__packed__)) rootdirectory {
	uint8_t filename[FS_FILENAME_LEN];  /* 16 Bytes */
	uint32_t filesize;     /* 4 Bytes */
	uint16_t dataIndex;    /* 2 Bytes */
	uint8_t padding[10];   /* 10 Bytes */
};

/* Holds Other Important Information */
struct fileentry{
	uint32_t fileOffset;
	struct rootdirectory* rootDirEntry;
	uint8_t filename[16];
};

/* Global Variables for each segment of disk */
struct superblock* SUPERBLOCK;
struct FATblock* FAT;
struct rootdirectory ROOT[FS_FILE_MAX_COUNT];
struct fileentry fdtable[FS_OPEN_MAX_COUNT];
int numFilesOpen;

int* find_sequence(int first_block, int num_blocks)
{

	int current_block = first_block;
	int fat_block;
	int fat_block_index;

	int* sequence = (int*)malloc(sizeof(int) * num_blocks);
	struct FATblock* currentFAT = FAT;

	if(num_blocks == 0){
		sequence[0] = current_block;
		return sequence;
	}

	for (int i = 0; i < num_blocks; i++) {  //  iterate through all blocks
		fat_block = (int)floor(current_block / 2048);  //  find correct FAT block
		fat_block_index = current_block % 2048;  //  find correct index of specific FAT block
		
		for (int j = 0; j < fat_block; j++) {  //  move to correct FAT block 
			currentFAT = currentFAT->continued;
		}

		sequence[i] = current_block;  // add index number to array
		current_block = currentFAT->entries[fat_block_index];  //  get index of next
		currentFAT = FAT;  //  reset current FAT pointer
	}

	if (current_block != FAT_EOC) {
		printf("ERROR: Did not reach end of chain\n");
	}

	return sequence;
}

int* findBlocks(int fileOffset, int countOfBytes, int firstDatablockIndex, int num_blocks){ 
	
	int* allBlocks = find_sequence(firstDatablockIndex, num_blocks);
	int initialBlock = fileOffset / BLOCK_SIZE;
	int blockRemaining = BLOCK_SIZE - (fileOffset % BLOCK_SIZE);
	int moreBlocks = ((countOfBytes - blockRemaining) / BLOCK_SIZE);
	if ((countOfBytes - blockRemaining) % BLOCK_SIZE > 0){
		moreBlocks += 1;
	}
	
	int* findBlocksarray = (int*)malloc(sizeof(int) * (moreBlocks + 1));
	for(int i = 0; i <= moreBlocks; i++){
		findBlocksarray[i] = allBlocks[i+initialBlock];
	}

	free(allBlocks);
	return findBlocksarray;
}

int fs_mount(const char *diskname)
{
	/* Open Disk File with Block API */
	if (block_disk_open(diskname)) { 
		return -1;
	}

	/* Allocate Space for Superblock & read block*/
	SUPERBLOCK = (struct superblock*)malloc(sizeof(struct superblock));
	block_read(0, SUPERBLOCK);

	if ((int)SUPERBLOCK->totalFAT == 0) {  //  Integrity Check of Superblock
		printf("0 FAT Blocks\n");
		return -1;
	}

	/* Allocate Space for FAT Linked List*/
	FAT = (struct FATblock*)malloc(sizeof(struct FATblock));
	FAT->continued = NULL;

	struct FATblock* currentFAT = (struct FATblock*)malloc(sizeof(struct FATblock)); // EXTRA MEMORY ALLOCATION
	currentFAT = FAT;
	for (int i = 1; i < (int)SUPERBLOCK->totalFAT; i++) {

		struct FATblock* newFAT = (struct FATblock*)malloc(sizeof(struct FATblock));
		newFAT->continued = NULL;
		currentFAT->continued = newFAT;
		currentFAT = currentFAT->continued;

	}

	/* Read Data into FAT*/
	currentFAT = FAT;
	for (int i = 0; i < (int)SUPERBLOCK->totalFAT; i++) {
		block_read(i + 1, &(currentFAT->entries));
		currentFAT = currentFAT->continued;
	}
	
	/* Root Dir and Read block */
	block_read(SUPERBLOCK->rootIndex, &ROOT);
	free(currentFAT);

	/* Initialize FD Table */
	numFilesOpen = 0;
	memset(fdtable, 0, sizeof(fdtable));
	return 0;
}

int fs_umount(void)
{
	/* Check to see if a disk is open */
	if (block_disk_count() == -1 || numFilesOpen > 0) {
		return -1;
	}

	/* Write Superblock to disk */
	block_write(0, 	SUPERBLOCK);

	struct FATblock* currentFAT;
	currentFAT = FAT;
	/* Write FAT Block(s) to disk */
	for (int i = 0; i < (int)SUPERBLOCK->totalFAT; i++) {
		block_write(i + 1, &(currentFAT->entries));
		currentFAT = currentFAT->continued;
	}

	/* Write Root Directory to disk */
	block_write(SUPERBLOCK->rootIndex, &ROOT);

	free(SUPERBLOCK);

	/* Deallocate linked list with while loop... */
	while(FAT != NULL){
		currentFAT = FAT;
		FAT = FAT->continued;
		free(currentFAT);
	}

	block_disk_close();
	return 0;
}

int fs_info(void)
{
	if(block_disk_count() == -1) {
		return -1;
	}

	/* Print out easily accessible information */
	printf("FS Info:\n");
	printf("total_blk_count=%d\n", SUPERBLOCK->totalBlocks);
	printf("fat_blk_count=%d\n", SUPERBLOCK->totalFAT);
	printf("rdir_blk=%d\n", SUPERBLOCK->rootIndex);
	printf("data_blk=%d\n", SUPERBLOCK->dataIndex);
	printf("data_blk_count=%d\n", SUPERBLOCK->totalData);

	/* calculate number of free data blocks */
	int freefats = 0;
	struct FATblock* currentFAT;
	currentFAT = FAT;
	for (int i = 0; i < (int)SUPERBLOCK->totalFAT; i++) {
		for (int j = 0; j < 2048; j++) {
			 if ((i*2048 + j) >= (int)SUPERBLOCK->totalData) {
				break;
			 }
			 if (currentFAT->entries[j] == 0) {
				freefats++;
			 }
		}
		currentFAT = currentFAT->continued;
	}
	printf("fat_free_ratio=%d/%d\n", freefats, SUPERBLOCK->totalData);

	/* calculate number of free root directories */
	int freeRoots = 0;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (ROOT[i].filename[0] == '\0') {
			freeRoots++;
		}
	}
	printf("rdir_free_ratio=%d/%d\n", freeRoots, FS_FILE_MAX_COUNT);

	return 0;
}

int fs_create(const char *filename)
{
	/* ERROR MANAGEMENT */
	int filenameLength = strlen(filename);
	//Create flag instead of block_disk_count()
	if(filename[filenameLength] != '\0' || filenameLength > FS_FILENAME_LEN || block_disk_count() == -1){
		return -1;
	}

	int indexofNewFile = -1;
	int count = 0;
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++){
		if(strcmp((char*)ROOT[i].filename, filename) == 0){
			return -1;
		}

		if(ROOT[i].filename[0] != '\0'){
			count++;
		}else{
			if(indexofNewFile == -1){
				indexofNewFile = i;
			}
		}
	}
	if(count > FS_FILE_MAX_COUNT){
		return -1;
	}

	/* CREATING FILE */
	strcpy((char*)ROOT[indexofNewFile].filename, filename);
	ROOT[indexofNewFile].filesize = 0;
	ROOT[indexofNewFile].dataIndex = FAT_EOC;
	return 0;
	
}

int fs_delete(const char *filename)
{
	if (block_disk_count() == -1 || filename == NULL) {
		return -1;
	}

	int find_root_index;
	int does_exist = 0;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (strcmp((char*)ROOT[i].filename, filename) == 0) {
			find_root_index = i;
			does_exist = 1;
			break;
		}
	}

	if (!does_exist) {
		return -1; // if filename doesn't exist in root directory 
	}

	int first_fat = (int)ROOT[find_root_index].dataIndex;
	int num_blocks = (ROOT[find_root_index].filesize / BLOCK_SIZE);
	if((ROOT[find_root_index].filesize % BLOCK_SIZE) > 0){
		num_blocks++;
	}
	int* sequence_array = find_sequence(first_fat, num_blocks);

	int fat_block;  
	int fat_block_index;
	struct FATblock* currentFAT = FAT;
	for (int i = 0; i < num_blocks; i++) {
		fat_block = (int)floor(sequence_array[i] / 2048);  
		fat_block_index = sequence_array[i] % 2048;

		for (int j = 0; j < fat_block; j++) {
			currentFAT = currentFAT->continued;
		}
		currentFAT->entries[fat_block_index] = 0;
		currentFAT = FAT;
	}
	
	free(sequence_array);
	ROOT[find_root_index].dataIndex = 0;
	ROOT[find_root_index].filename[0] = '\0';
	ROOT[find_root_index].filesize = 0;
	
	return 0;
}

int fs_ls(void)
{
	if (block_disk_count() == -1) {
		return -1; 
	}

	printf("FS Ls:\n");
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (ROOT[i].filename[0] != '\0'){
			printf("file: %s, size: %d, data_blk: %d\n", ROOT[i].filename, ROOT[i].filesize, ROOT[i].dataIndex);
		}
	}

	return 0;
	
}

int fs_open(const char *filename)
{
	int filenameLength = strlen(filename);
	if(filename[filenameLength] != '\0' || filenameLength > FS_FILENAME_LEN || numFilesOpen == FS_OPEN_MAX_COUNT || block_disk_count() == -1){
		return -1;
	}

	int tempindex = -1;
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (strcmp((char*)ROOT[i].filename, filename) == 0) {
			tempindex = i;
			break;
		}
	}
	if(tempindex == -1){
		return -1;
	}

	for(int j = 0; j < FS_OPEN_MAX_COUNT; j++){
		if(fdtable[j].filename[0] == '\0'){
			strcpy((char*)fdtable[j].filename,filename);
			fdtable[j].fileOffset = 0;
			fdtable[j].rootDirEntry = &(ROOT[tempindex]);
			numFilesOpen++;
			return(j);
		}
	}
	return -1;
}

int fs_close(int fd)
{
	if(fd > FS_OPEN_MAX_COUNT || fdtable[fd].filename[0] == '\0' || block_disk_count() == -1 || numFilesOpen == 0){
		return -1;
	}

	fdtable[fd].filename[0] = '\0';
	numFilesOpen--;
	return 0; 
}

int fs_stat(int fd)
{
	if(fd > FS_OPEN_MAX_COUNT || fdtable[fd].filename[0] == '\0' || block_disk_count() == -1 || numFilesOpen == 0){
		return -1;
	}

	return fdtable[fd].rootDirEntry->filesize;
}

int fs_lseek(int fd, size_t offset)
{
	if(fd > FS_OPEN_MAX_COUNT || fdtable[fd].filename[0] == '\0' || block_disk_count() == -1 || numFilesOpen == 0 || offset > fs_stat(fd) || offset < 0){
		return -1;
	}

	fdtable[fd].fileOffset = offset;
	return 0;
}

int* findNewFATBlock(){
	int fatBlock = -1;
	int newFATEntry = -1;
	struct FATblock* currentFAT;
	currentFAT = FAT;

	for (int i = 0; i < (int)SUPERBLOCK->totalFAT; i++) {
		for (int j = 0; j < 2048; j++) {
			 if (currentFAT->entries[j] == 0) {
				fatBlock = i;
				newFATEntry = j;
				break;
			 }
		}
		if(fatBlock > -1 && newFATEntry > -1){
			break;
		}
		currentFAT = currentFAT->continued;
	}

	if(fatBlock == -1 && newFATEntry == -1){
		return(NULL);
	}

	int* fatEntryInfo = (int*)malloc(sizeof(int) * (2));
	fatEntryInfo[0] = fatBlock;
	fatEntryInfo[1] = newFATEntry;
	
	return(fatEntryInfo);

}

int fs_write(int fd, void *buf, size_t count)
{
	/* ERROR HANDLING*/
	if(fd > FS_OPEN_MAX_COUNT || fdtable[fd].filename[0] == '\0' || block_disk_count() == -1 || numFilesOpen == 0 || buf == NULL || count == 0){
		return -1;
	}

	fdtable[fd].rootDirEntry->filesize = fdtable[fd].fileOffset;
	uint32_t fileOffset = fdtable[fd].fileOffset;
	uint32_t fileSize = fdtable[fd].rootDirEntry->filesize;
	uint16_t firstDatablockIndex = fdtable[fd].rootDirEntry->dataIndex;
	uint16_t startDataIndex = SUPERBLOCK->dataIndex;

	if(firstDatablockIndex == FAT_EOC){
		int* fatEntry = findNewFATBlock();
		if (fatEntry == NULL){
			return 0;
		}

		fdtable[fd].rootDirEntry->dataIndex = fatEntry[1];
		firstDatablockIndex = fdtable[fd].rootDirEntry->dataIndex;
		// update file table index as FAT_EOC
		
		struct FATblock* currentFAT;
		currentFAT = FAT;
		while(fatEntry[0] > 0){
			currentFAT = currentFAT->continued;
			fatEntry[0]--;
		}
		currentFAT->entries[fatEntry[1]] = FAT_EOC;
	}
	
	int bytestoWrite = count;
	char* tempBlock = malloc(BLOCK_SIZE);
	int bufferPointer = 0;
	int num_blocks;
	int* blocksOfInterest = NULL;
	while(bytestoWrite > 0){
		num_blocks = ((fileSize + bufferPointer) / BLOCK_SIZE);
		if (((fileSize + bufferPointer) % BLOCK_SIZE) >= 0){
			num_blocks++;
		}

		blocksOfInterest = findBlocks(fileOffset, count, firstDatablockIndex, num_blocks);
		int startingBlock = blocksOfInterest[0];
		uint16_t blockwhereOffsetIs = startingBlock + startDataIndex;
		block_read(blockwhereOffsetIs,tempBlock);
		int blockOffset = fileOffset % BLOCK_SIZE;

		if((blockOffset+bytestoWrite) < BLOCK_SIZE){
			memcpy(tempBlock + blockOffset, buf + bufferPointer, bytestoWrite);
			fileOffset += bytestoWrite;
			bufferPointer += bytestoWrite;
			bytestoWrite = 0;
			block_write(blockwhereOffsetIs,tempBlock);

		}else if((blockOffset+bytestoWrite) > BLOCK_SIZE){
			int partOfBlock = BLOCK_SIZE - blockOffset;
			memcpy(tempBlock+blockOffset, buf + bufferPointer, partOfBlock);
			fileOffset += partOfBlock;
			bufferPointer += partOfBlock;
			bytestoWrite = bytestoWrite - partOfBlock;
			block_write(blockwhereOffsetIs, tempBlock);
			
			int* newfatEntry2 = findNewFATBlock();
			if(newfatEntry2 == NULL){
				fdtable[fd].fileOffset = fileOffset;
				fdtable[fd].rootDirEntry->filesize = fileSize + bufferPointer;
				free(blocksOfInterest);
				free(tempBlock);
				return(bufferPointer);
			}

			int fat_block;  
			int fat_block_index;
			struct FATblock* currentFAT2 = FAT;
			fat_block = (int)floor(startingBlock / 2048);  
			fat_block_index = startingBlock % 2048;
			for (int j = 0; j < fat_block; j++) {
				currentFAT2 = currentFAT2->continued;
			}
			currentFAT2->entries[fat_block_index] = newfatEntry2[1];
			currentFAT2 = FAT;

			struct FATblock* currentFAT3 = FAT;
			while(newfatEntry2[0] > 0){
				currentFAT3 = currentFAT3->continued;
				newfatEntry2[0]--;
			}
			currentFAT3->entries[newfatEntry2[1]] = FAT_EOC;
		}else if((blockOffset+bytestoWrite) == BLOCK_SIZE){
			memcpy(tempBlock+blockOffset, buf + bufferPointer, bytestoWrite);
			fileOffset += bytestoWrite;
			bufferPointer += bytestoWrite;
			bytestoWrite = bytestoWrite - BLOCK_SIZE;
			block_write(blockwhereOffsetIs, tempBlock);
		}
	}

	fdtable[fd].rootDirEntry->filesize = fileSize + bufferPointer;
	fdtable[fd].fileOffset = fileOffset;
	free(blocksOfInterest);
	free(tempBlock);
	return(bufferPointer);
	/* TODO: Phase 4 */
}

int fs_read(int fd, void *buf, size_t count)
{
	/* ERROR HANDLING*/
	if(fd > FS_OPEN_MAX_COUNT || fdtable[fd].filename[0] == '\0' || block_disk_count() == -1 || numFilesOpen == 0 || buf == NULL || count == 0){
		return -1;
	}

	uint32_t fileOffset = fdtable[fd].fileOffset;
	uint32_t fileSize = fdtable[fd].rootDirEntry->filesize;
	uint16_t firstDatablockIndex = fdtable[fd].rootDirEntry->dataIndex;
	uint16_t startDataIndex = SUPERBLOCK->dataIndex;


	if(firstDatablockIndex == FAT_EOC){
		return(0);
	}

	char *readBlock = malloc(BLOCK_SIZE);
	int bufferPointer = 0;
	
	int bytestoRead = count;
	if((fileSize-fileOffset) < count){
		bytestoRead = fileSize-fileOffset;
	}

	int num_blocks = (fileSize / BLOCK_SIZE);
	if((fileSize % BLOCK_SIZE) > 0){
		num_blocks++;
	}

	int* blocksOfInterest = findBlocks(fileOffset, count, firstDatablockIndex, num_blocks);

	int blockIndex = 0;
	while(bytestoRead > 0){
		int startingBlock = blocksOfInterest[blockIndex];
		uint16_t blockwhereOffsetIs = startingBlock + startDataIndex;
		
		block_read(blockwhereOffsetIs,readBlock);
		int blockOffset = fileOffset % BLOCK_SIZE;
		if((blockOffset + bytestoRead) < BLOCK_SIZE){ 
			memcpy(buf + bufferPointer, readBlock + blockOffset,bytestoRead);
			fileOffset += bytestoRead;
			bufferPointer += bytestoRead;
			bytestoRead = 0;
		}
		else if ((blockOffset + bytestoRead) > BLOCK_SIZE){
			int partOfBlock = BLOCK_SIZE - blockOffset;
			memcpy(buf + bufferPointer, readBlock+blockOffset, partOfBlock);
			fileOffset += partOfBlock; 
			bufferPointer += partOfBlock;
			bytestoRead -= partOfBlock; 
			blockIndex += 1;
		}
		else if((blockOffset + bytestoRead) == BLOCK_SIZE){
			memcpy(buf + bufferPointer, readBlock + blockOffset, bytestoRead);
			fileOffset += bytestoRead;
			bufferPointer += bytestoRead;
			bytestoRead = bytestoRead - BLOCK_SIZE;
		}
	}

	fdtable[fd].fileOffset = fileOffset;
	free(blocksOfInterest);
	free(readBlock);
	return(bufferPointer);
}

