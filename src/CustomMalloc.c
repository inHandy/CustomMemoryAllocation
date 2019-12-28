#include <stdlib.h>
#include <stdio.h>
#include "CustomMalloc.h"
#define MEMBLOCKSIZE 4096

#pragma region Stuctures
/**
 * Each Record represents a memory allocation.
 */
typedef struct Record Record;
struct Record
{
	void* ptr;
	int size;
	int numOfRefs;
	Record* next;

};

/**
 * Represents allocated memory space for multiple records.
 */
typedef struct MemBlock MemBlock;
struct MemBlock
{
	void* memHead;
	Record* firstRecord;
	Record* lastRecord;
	int availSpace;
	int size;
	MemBlock* nextBlock;
};
#pragma endregion

#pragma region Members
static MemBlock* firstMemBlock;
static MemBlock* latestMemBlockSave;
static MemBlock* latestMemBlockSaveFree;
#pragma endregion

#pragma region Methods
/**
 * Handles errors. Occurs when an unexpected value is found.
 */
void* ErrorHandler(size_t size) {
	printf("Error during execution. Using native malloc.\n");
	return malloc(size);
}

/**
 * Creates a new record.
 */
int* NewRecord(MemBlock* memBlock, void* recordPtr, int size, Record* nextRec) {
	int* contentPtr = (int*)((char*)recordPtr + sizeof(Record));
	Record* newRec = (Record*)recordPtr;
	newRec->ptr = contentPtr;
	newRec->numOfRefs = 1;
	newRec->size = (int)size;
	newRec->next = nextRec;
	memBlock->availSpace = memBlock->availSpace - ((int)size + (int) sizeof(Record));
	return contentPtr;
}

/**
 * Initialises the first block of memory.
 */
void InitialiseFirstBlock(int size) {
	int blockSize = MEMBLOCKSIZE;
	if (size > MEMBLOCKSIZE) {
		blockSize = size;
	}
	firstMemBlock = (MemBlock*)malloc(blockSize);
	if (firstMemBlock == NULL) {
		return;
	}
	firstMemBlock->memHead = (char*)firstMemBlock + sizeof(MemBlock);
	firstMemBlock->firstRecord = NULL;
	firstMemBlock->lastRecord = NULL;
	firstMemBlock->size = blockSize;
	firstMemBlock->availSpace = blockSize - sizeof(MemBlock);
	firstMemBlock->nextBlock = NULL;
}

/**
 * Allocates memory space for the given size.
 */
void* CustomMalloc(size_t size) {
	int minSize = size + sizeof(Record) + sizeof(MemBlock);

	if (firstMemBlock == NULL) {
		InitialiseFirstBlock(minSize);
	}

	MemBlock* currentBlock = firstMemBlock;

	Record* currentAppendableRec;
	Record* nextRec = currentBlock->firstRecord;
	Record* lastRec = currentBlock->lastRecord;

	// Save states for optimization
	int latestMemBlockSaveUsed = 0;
	int AssignLatestMemBlockSave = 0;
	
	do {
		// Whether there's enough space in currentBlock
		if (currentBlock->availSpace < (size + sizeof(Record)) && currentBlock->nextBlock != NULL) {
			currentBlock = currentBlock->nextBlock;
			latestMemBlockSaveUsed = 1;

			if (latestMemBlockSave != NULL && latestMemBlockSave != currentBlock && AssignLatestMemBlockSave == 0) {
				AssignLatestMemBlockSave = 1;
				currentBlock = latestMemBlockSave;
			}

			if (currentBlock->firstRecord == NULL) {
				continue;
			}

			currentAppendableRec = NULL;
			nextRec = currentBlock->firstRecord;
			lastRec = currentBlock->lastRecord;
		}

		// Whether currentBlock is empty.
		if (currentBlock->firstRecord == NULL && minSize <= currentBlock->size) {
			nextRec = (Record*)currentBlock->memHead;
			currentBlock->firstRecord = nextRec;
			currentBlock->lastRecord = nextRec;

			if (latestMemBlockSaveUsed == 1)
				latestMemBlockSave = currentBlock;

			return NewRecord(currentBlock, nextRec, size, NULL);
		}

		// currentPotentialRec is not available for insertion, switch to next Record.
		currentAppendableRec = nextRec;
		if (currentAppendableRec != NULL) {
			nextRec = nextRec->next;
		}

		//Wheter Memblock has space at first index.
		if (currentBlock->firstRecord != NULL && ((char*)currentBlock->firstRecord - (char*)currentBlock->memHead) >= size) {
			Record* recInsert = (Record*)currentBlock->memHead;
			currentBlock->firstRecord = recInsert;
			return NewRecord(currentBlock, recInsert, size, currentAppendableRec);
		}

		
		if (currentAppendableRec == lastRec) {

			int BlockEndRemainingSpace = -1;
			if (lastRec != NULL)
				BlockEndRemainingSpace = ((char*)currentBlock + currentBlock->size) - ((char*)lastRec->ptr + lastRec->size);

			//Whether insertion after lastRec is possible.
			if (BlockEndRemainingSpace != -1 && BlockEndRemainingSpace >= size + sizeof(Record)) {
				if (latestMemBlockSaveUsed == 1) {
					latestMemBlockSave = currentBlock;
				}
				nextRec = (Record*)((char*)currentAppendableRec + sizeof(Record) + currentAppendableRec->size);
				currentBlock->lastRecord = nextRec;
				currentAppendableRec->next = nextRec;
				return NewRecord(currentBlock, nextRec, size, NULL);

			}

			//Whether there's an existing block after currentBlock
			if (currentBlock->nextBlock == NULL) {

				int customBlockSize = MEMBLOCKSIZE;

				//Cas ou on a une taille superieur a la taille par defaut (~4000 dans notre cas)
				if (size > (MEMBLOCKSIZE - sizeof(Record) - sizeof(MemBlock))) {
					customBlockSize = size + sizeof(Record) + sizeof(MemBlock);

				}
				currentBlock->nextBlock = (MemBlock*)malloc(customBlockSize);
				currentBlock = currentBlock->nextBlock;

				if (currentBlock == NULL) {
					return ErrorHandler(size);
				}
				currentBlock->memHead = (char*)currentBlock + sizeof(MemBlock);
				currentBlock->firstRecord = NULL;
				currentBlock->lastRecord = NULL;
				currentBlock->availSpace = customBlockSize - sizeof(MemBlock);
				currentBlock->size = customBlockSize;
				currentBlock->nextBlock = NULL;

			} else {
				currentBlock = currentBlock->nextBlock;
				if (currentBlock->firstRecord == NULL)
					continue;

				currentAppendableRec = NULL;
				nextRec = currentBlock->firstRecord;
				lastRec = currentBlock->lastRecord;
			}
			continue;
		}

		//Whether it's possible to sandwich a Record between two existing Records.
		int spaceBetweenCurrentNext = (((int)nextRec) - (((int)currentAppendableRec->ptr) + ((int)currentAppendableRec->size)));
		if (spaceBetweenCurrentNext >= size + sizeof(Record)) {
			void* rightSideRec = nextRec;
			nextRec = (Record*)((char*)currentAppendableRec + sizeof(Record) + currentAppendableRec->size);
			currentAppendableRec->next = nextRec;
			return NewRecord(currentBlock, nextRec, size, (Record*)rightSideRec);
		}
	} while (1);
 }

 /**
 * Increments the reference counter for a pointer.
 */
int IncrementRefCount(void* ptr) {
	MemBlock* currentBlock = firstMemBlock;
	Record* currentRec = firstMemBlock->firstRecord;

	while (currentRec != NULL || currentBlock->nextBlock != NULL) {
		if (currentRec->ptr == ptr) {
			currentRec->numOfRefs += 1;
			break;
		}
		if (currentRec->next == NULL) {
			currentBlock = currentBlock->nextBlock;

			if (currentBlock == NULL)
				return 0;

			currentRec = (Record*)currentBlock->memHead;
			continue;
		}
		currentRec = currentRec->next;
	}

	if (currentRec == NULL)
		return 0;

	return currentRec->numOfRefs;
}

/**
 * Frees a pointer or decrements its reference count.
 */
void CustomFree(void* ptr) {
	if (ptr == NULL)
		return;

	Record* prevRec = NULL;
	Record* currentRec = firstMemBlock->firstRecord;
	MemBlock* prevMemBlock = NULL;
	MemBlock* currentMemBlock = firstMemBlock;

	latestMemBlockSave = NULL;
	int latestMemBlockSaveUsed = 0;

	while (currentRec != NULL || currentMemBlock->nextBlock != NULL) {
		void* beginningAdress = currentMemBlock->memHead;
		void* endAdress = (char*)currentMemBlock + currentMemBlock->size;

		//Whether ptr is in currentMemBlock's adress range.
		if ((int)ptr<(int)beginningAdress || (int)ptr>(int)endAdress) {
			prevMemBlock = currentMemBlock;
			currentMemBlock = currentMemBlock->nextBlock;

			if (latestMemBlockSaveFree != NULL && latestMemBlockSaveFree != currentMemBlock && latestMemBlockSaveFree != prevMemBlock) {
				prevMemBlock = latestMemBlockSaveFree;
				currentMemBlock = latestMemBlockSaveFree->nextBlock;
				latestMemBlockSaveUsed = 1;
				latestMemBlockSaveFree = NULL;
			}
			if (currentMemBlock == NULL) {
				return;
			}
			currentRec = (Record*)currentMemBlock->memHead;
			continue;
		}

		if (currentRec == currentMemBlock->firstRecord && currentRec == NULL) {
			currentMemBlock = currentMemBlock->nextBlock;
			return;
		}

		//Whether the relevant record was found.
		if (currentRec->ptr == ptr) {
			currentRec->numOfRefs -= 1;

			//Whether the reference count reached 0.
			if (currentRec->numOfRefs == 0) {
				
				//Whether the currentRec is the only Record in currentMemBlock.
				if (currentMemBlock->firstRecord == currentRec && currentRec->next == NULL) {

					if (prevMemBlock == NULL) {
						if (currentMemBlock->nextBlock == NULL) {
							free(currentMemBlock);
							firstMemBlock = NULL;
							latestMemBlockSaveFree = prevMemBlock;
							return;
						}
						
						firstMemBlock = currentMemBlock->nextBlock;
						free(currentMemBlock);
						latestMemBlockSaveFree = prevMemBlock;
						return;
					}

					//Links MemBlocks surrounding currently removed MemBlock.
					prevMemBlock->nextBlock = currentMemBlock->nextBlock;
					free(currentMemBlock);
					latestMemBlockSaveFree = prevMemBlock;
					return;
				}

				//Whether the currentRec is the last Record of currentMemBlock.
				if (currentRec->next == NULL) {
					prevRec->next = currentRec->next;
					currentMemBlock->availSpace += currentRec->size + sizeof(Record);
					currentMemBlock->lastRecord = prevRec;
					latestMemBlockSaveFree = prevMemBlock;
					return;
				}

				//Whether the currentRec is the first Record of currentMemBlock.
				if (currentMemBlock->firstRecord == currentRec) {
					currentMemBlock->availSpace += currentRec->size + sizeof(Record);
					currentMemBlock->firstRecord = currentRec->next;
					latestMemBlockSaveFree = prevMemBlock;
					return;
				}
				
				//Links Records surrounding currently freed Record.
				prevRec->next = currentRec->next;
				currentMemBlock->availSpace += currentRec->size + sizeof(Record);
				latestMemBlockSaveFree = prevMemBlock;
				return;
			}
			return;
		}

		if (currentRec->next == NULL) {
			prevMemBlock = currentMemBlock;
			currentMemBlock = currentMemBlock->nextBlock;
			if (currentMemBlock == NULL) {
				if (latestMemBlockSaveUsed == 1) {
					currentMemBlock = firstMemBlock;
					currentRec = (Record*)currentMemBlock->memHead;
					continue;
				}
				return;
			}
			currentRec = (Record*)currentMemBlock->memHead;
			continue;
		}

		prevRec = currentRec;
		currentRec = currentRec->next;
	}
}
#pragma endregion

#pragma region Main
int main() {
	//int* integer = (int*)CustomMalloc(sizeof(int));
	//RunTests();
	return 0;
}
#pragma endregion