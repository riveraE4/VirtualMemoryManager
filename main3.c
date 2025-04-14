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

int main(int argc, char **argv) {

    /* open the address file for reading logical addresses */
    FILE *addressFile = fopen(argv[1], "r");
    /* open the backing store binary file */
    FILE *backingStore = fopen("BACKING_STORE.bin", "rb");

    /* open output files for writing results */
    FILE *out1 = fopen("out1.txt", "w"); /* logical addresses */
    FILE *out2 = fopen("out2.txt", "w"); /* physical addresses */
    FILE *out3 = fopen("out3.txt", "w"); /* values stored in physical memory */

    int logicalAddress = 0; /* logical address read from the input */
    int maskedAddress = 0; /* logical is turned into 16 bits */
    int pageNum = 0; /* page number upper 8 bits*/
    int offset = 0; /* offset which is the lower 8 bits*/
    int frameNum = 0; /* frame number related to the page where it is stored in physical memory */
    int physicalAddress = 0; /* computation done by frameNum * PAGE_SIZE + offset */
    
    int totalAddress = 0; /* total number of processed addresses */
    int hits = 0; /* # of TLB hits */
    int pageFaults = 0; /* # of page faults */

    int freeFrame = 0; /* next frame in physical memory */
    int tlbIndex = 0; /* updating index in TLB */
    int currentTime = 0; /* time counter for LRU frame replacement */

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

                    /* load the new page into the LRU frame */
                    frameNum = lru;
                    pageTable[pageNum] = frameNum;
                    frameTable[frameNum] = pageNum;
                }
                /* update the TLB with the newly loaded page */
                tlb[tlbIndex].page = pageNum;
                tlb[tlbIndex].frame = frameNum;
                tlbIndex = (tlbIndex + 1) % TLB_SIZE;
            }
        }
        /* physical addresses are retrieved and calculated */
        physicalAddress = frameNum * PAGE_SIZE + offset;
        int value = physicalMemory[physicalAddress];

        fprintf(out2, "%d\n", physicalAddress);
        fprintf(out3, "%d\n", value);

        lastUsed[frameNum] = currentTime++; /* a record of the last time of use for LRU*/
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
