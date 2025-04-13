#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 256
#define NUM_PAGES 256
#define NUM_FRAMES 128
#define TLB_SIZE 16

typedef struct {
    int page;
    int frame;
} TLBEntry;

void loadPage(FILE *backingStore, int pageNum, signed char *physicalMemory, int frame)
{
  if (fseek(backingStore, pageNum * PAGE_SIZE, SEEK_SET) != 0) {
    fprintf(stderr, "Error: failed in backing store\n");
    exit(1);
  }
  if (fread(&physicalMemory[frame * PAGE_SIZE], sizeof(signed char), PAGE_SIZE, backingStore) != PAGE_SIZE) {
    fprintf(stderr, "Error: fread failed\n");
    exit(1);
  }
}

int main(int argc, char **argv) {
    /* error checking */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <address_file>\n", argv[0]);
        exit(1);
    }

    /* open the address file for reading logical addresses */
    FILE *addressFile = fopen(argv[1], "r");
    if (addressFile == NULL) {
        perror("can't open address file");
        exit(1);
    }

    /* open the backing store binary file */
    FILE *backingStore = fopen("BACKING_STORE.bin", "rb");
    if (backingStore == NULL) {
        perror("can't open backing_store.bin");
        exit(1);
    }

    /* open output files for writing results */
    FILE *out1 = fopen("out1.txt", "w"); /* logical addresses */
    FILE *out2 = fopen("out2.txt", "w"); /* physical addresses */
    FILE *out3 = fopen("out3.txt", "w"); /* values stored in physical memory */
    if (!out1 || !out2 || !out3) { /* error checking, check to see if they can be opened */
        perror("can't open output .txt file(s)");
        exit(1);
    }

    int logicalAddress = 0;
    int maskedAddress = 0;
    int pageNum = 0;
    int offset = 0;
    int frameNum = 0;
    int physicalAddress = 0;
    int totalAddress = 0;
    int hits = 0;
    int pageFaults = 0;
    int freeFrame = 0;
    int tlbIndex = 0;
    int currentTime = 0;

    signed char physicalMemory[NUM_FRAMES * PAGE_SIZE];
    memset(physicalMemory, 0, sizeof(physicalMemory));

    int pageTable[NUM_PAGES];
    memset(pageTable, -1, sizeof(pageTable));

    int frameTable[NUM_FRAMES];
    for (int i = 0; i < NUM_FRAMES; ++i) {
        frameTable[i] = -1;
    }

    int lastUsed[NUM_FRAMES];
    for (int i = 0; i < NUM_FRAMES; ++i) {
        lastUsed[i] = -1;
    }

    TLBEntry tlb[TLB_SIZE];
    for (int i = 0; i < TLB_SIZE; ++i) {
        tlb[i].page = -1;
        tlb[i].frame = -1;
    }

    while (fscanf(addressFile, "%d", &logicalAddress) == 1) {
        totalAddress++;
        fprintf(out1, "%d\n", logicalAddress);

        maskedAddress = logicalAddress & 0xFFFF;
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
            if (pageTable[pageNum] != -1) {
                /* Page table hit, get the frame number */
                frameNum = pageTable[pageNum];
                /* update the TLB on a page table hit if desired */
                tlb[tlbIndex].page = pageNum;
                tlb[tlbIndex].frame = frameNum;
                tlbIndex = (tlbIndex + 1) % TLB_SIZE;
            } else {
                /* Page fault occurs */
                pageFaults++;
                if (freeFrame < NUM_FRAMES) {
                    /* Use free frame available */
                    loadPage(backingStore, pageNum, physicalMemory, freeFrame);
                    frameNum = freeFrame;
                    pageTable[pageNum] = frameNum;
                    frameTable[frameNum] = pageNum;
                    freeFrame++;
                } else {
                    /* No free frame available, perform LRU replacement */
                    int lru = 0;
                    for (int i = 1; i < NUM_FRAMES; ++i) {
                        if (lastUsed[i] < lastUsed[lru]) {
                            lru = i;
                        }
                    }
                    int victimPage = frameTable[lru];
                    pageTable[victimPage] = -1;

                    /* invalidate any TLB entries for the victim page */
                    for (int i = 0; i < TLB_SIZE; ++i) {
                        if (tlb[i].page == victimPage) {
                            tlb[i].page = -1;
                        }
                    }

                    /* Load the new page into the LRU frame */
                    loadPage(backingStore, pageNum, physicalMemory, lru);
                    frameNum = lru;
                    pageTable[pageNum] = frameNum;
                    frameTable[frameNum] = pageNum;
                }
                /* Update the TLB with the newly loaded page */
                tlb[tlbIndex].page = pageNum;
                tlb[tlbIndex].frame = frameNum;
                tlbIndex = (tlbIndex + 1) % TLB_SIZE;
            }
        }

        physicalAddress = frameNum * PAGE_SIZE + offset;
        int value = physicalMemory[physicalAddress];

        fprintf(out2, "%d\n", physicalAddress);
        fprintf(out3, "%d\n", value);

        lastUsed[frameNum] = currentTime++;
    }

    fclose(addressFile);
    fclose(backingStore);
    fclose(out1);
    fclose(out2);
    fclose(out3);

    printf("Total addresses: %d\n", totalAddress);
    printf("TLB hits: %d\n", hits);
    printf("Page Faults: %d\n", pageFaults);
    printf("TLB Hit Rate: %.2f\n", (float)hits / totalAddress);
    printf("Page Fault Rate: %.2f\n", (float)pageFaults / totalAddress);

    return 0;
}
