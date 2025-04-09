#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 256
#define NUM_PAGES 256
#define NUM_FRAMES 256
#define TLB_SIZE 16

typedef struct {
  int page;
  int frame;
} TLBEntry;

int main(int argc, char** argv)
{
  FILE *addressFile = fopen(argv[1], "r");
  if (addressFile == NULL) {
    perror("Can't open file");
    exit(1);
  }

  FILE *backingStore = fopen("BACKING_STORE.bin", "rb");
  if (backingStore == NULL) {
    perror("Can't open .bin");
    exit(1);
  }

  // writes the output of out1 - out3 to a .txt file
  FILE *out1 = fopen("out1.txt", "w"); 
  FILE *out2 = fopen("out2.txt", "w");
  FILE *out3 = fopen("out3.txt", "w");

  if (!out1 || !out2 || !out3) { //error handling
    perror("Can't open and output .txt file(s)");
    exit(1);
  }

  int logicalAddress = 0; /* logical address read from file */
  int maskedAddress = 0; /* logical address with masking  */

  int pageNum = 0; /* page # extracted from upper 8 bits */
  int offset = 0; /* offset, from lower 8 bits*/

  int frameNum = 0; /* frame number assigned to page */
  int physicalAddress = 0; /* (frame number * PAGE_SIZE) + offset */

  int totalAddress = 0; /* total # of processed addresses */
  int hits = 0; /* counter for TLB Hits */
  int pageFaults = 0; /* counter for page faults */

  int freeFrame = 0; /* index for the next free frame within physical memory */
  int tlbIndex = 0; /* index used for FIFO within TLB */

  signed char physicalMemory[NUM_FRAMES * PAGE_SIZE];
  memset(physicalMemory, 0, sizeof(physicalMemory));

  int pageTable[NUM_PAGES];
  memset(pageTable, -1, sizeof(pageTable));

  TLBEntry tlb[TLB_SIZE];
  memset(tlb, -1, sizeof(tlb));

  while (fscanf(addressFile, "%d", &logicalAddress) != EOF) {
    totalAddress++;
  }

  maskedAddress = logicalAddress + 0xFFFF;

  pageNum = (maskedAddress >> 8) & 0xFF;
  offset = maskedAddress & 0xFF;

  int foundInTLB = 0;
  for (int i = 0; i < TLB_SIZE; i++) {
    if (tlb[i].page == pageNum) {
      frameNum = tlb[i].frame;
      hits++;
      foundInTLB = 1;
      break;
    }
  }

  if (!foundInTLB) {
    if (pageTable[pageNum] != 1) {
      frameNum = pageTable[pageNum];
    } else {
      pageFaults++;
      if (fseek(backingStore, pageNum * PAGE_SIZE, SEEK_SET) != 0) {

      }
    }
  }

  fclose(addressFile);
  fclose(backingStore);
  fclose(out1);
  fclose(out2);
  fclose(out3);

  return 0;
}