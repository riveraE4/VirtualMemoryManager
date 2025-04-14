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

int main(int argc, char** argv) {
    
    /* open the address file for reading logical addresses */
    FILE *addressFile = fopen(argv[1], "r");

    /* open the backing store binary file */
    FILE *backingStore = fopen("BACKING_STORE.bin", "rb");

    /* open output files for writing results */
    FILE *out1 = fopen("out1.txt", "w"); /* logical addresses */
    FILE *out2 = fopen("out2.txt", "w"); /* physical addresses */
    FILE *out3 = fopen("out3.txt", "w"); /* values stored in physical memory */

    /* declare variables for address translation */
    int logicalAddress = 0;   
    int maskedAddress = 0;   
    int pageNum = 0;      
    int offset = 0;           
    int frameNum = 0;         
    int physicalAddress = 0;  

    /* stats counter */
    int totalAddress = 0;      
    int hits = 0;              
    int pageFaults = 0;        

    int freeFrame = 0;         
    int tlbIndex = 0;          

    /* initialize physical memory and set all bytes to zero */
    signed char physicalMemory[NUM_FRAMES * PAGE_SIZE];
    memset(physicalMemory, 0, sizeof(physicalMemory));

    /* initialize page table; -1 indicates page not loaded */
    int pageTable[NUM_PAGES];
    memset(pageTable, -1, sizeof(pageTable));

    /* initialize the tlb, setting all entries to -1 */
    TLBEntry tlb[TLB_SIZE];
    memset(tlb, -1, sizeof(tlb));

    /* process each logical address from the address file */
    while (fscanf(addressFile, "%d", &logicalAddress) == 1) {
        totalAddress++;

        /* write the logical address to out1.txt */
        fprintf(out1, "%d\n", logicalAddress);

        /* mask the logical address to 16 bits */
        maskedAddress = logicalAddress & 0xFFFF;

        /* extract page number and offset */
        pageNum = (maskedAddress >> 8) & 0xFF;
        offset = maskedAddress & 0xFF;

        int foundInTLB = 0; /* flag for a tlb hit */

        /* check if the page number is in the tlb */
        for (int i = 0; i < TLB_SIZE; i++) {
            if (tlb[i].page == pageNum) {
                frameNum = tlb[i].frame;
                hits++;
                foundInTLB = 1;
                break;
            }
        }

        /* if the page is not found in the tlb, check the page table */
        if (!foundInTLB) {
            if (pageTable[pageNum] != -1) {
                /* page table hit */
                frameNum = pageTable[pageNum];
            } else {
                /* page fault, page is not in physical memory */
                pageFaults++;
                /* use the free frame that was filled */
                frameNum = freeFrame;
                /* update the page table with mapping */
                pageTable[pageNum] = frameNum;
                freeFrame++;
            }

            /* update the tlb using fifo replacement policy */
            tlb[tlbIndex].page = pageNum;
            tlb[tlbIndex].frame = frameNum;
            tlbIndex = (tlbIndex + 1) % TLB_SIZE;
        }

        /* find the physical address */
        physicalAddress = frameNum * PAGE_SIZE + offset;

        /* retrieve the value stored at the physical address in physical memory */
        int value = physicalMemory[physicalAddress];

        /* write the physical address to out2.txt and the value to out3.txt */
        fprintf(out2, "%d\n", physicalAddress);
        fprintf(out3, "%d\n", value);
    }

    /* close all open files */
    fclose(addressFile);
    fclose(backingStore);
    fclose(out1);
    fclose(out2);
    fclose(out3);

    /* print stats */
    printf("Total Addresses: %d\n", totalAddress);
    printf("TLB Hits: %d\n", hits);
    printf("Page Faults: %d\n", pageFaults);
    printf("TLB Hit Rate: %.2f\n", (float)hits / totalAddress);
    printf("Page Fault Rate: %.2f\n", (float)pageFaults / totalAddress);

    return 0;
}