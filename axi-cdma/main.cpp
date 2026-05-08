#include "xil_exception.h"
#include "xil_cache.h"
#include "xparameters.h"
#include "xil_types.h"
#include <iostream>
using namespace std;

#define MAX_BYTES_TO_TRANSFER_SIZE (8388607)
#define ARRAY_MAX_SIZE (MAX_BYTES_TO_TRANSFER_SIZE / sizeof(u32))

// DMA error bits in CDMASR
// bit 4 = 1 -> CDMA internal error detected
// bit 5 = 1 -> CDMA slave error detected
// bit 6 = 1 -> CDMA decode error detected
// error bit mask = ...0111 0000 = 0x00000070
#define CDMA_ERROR_MASK (0x00000070)

// Function to start the timer
void start_timer(volatile u32 *timer_base_address) {
    // AXI Timer register map:
    // *(timer_base_address + 0) = TCSR0  (0x00)
    // *(timer_base_address + 1) = TLR0   (0x04)
    // *(timer_base_address + 2) = TCR0   (0x08)

    *(timer_base_address + 0) = 0x00000000;   // stop timer
    *(timer_base_address + 1) = 0xFFFFFFFF;   // load max value
    *(timer_base_address + 0) = 0x00000020;   // set LOAD bit
    *(timer_base_address + 0) = 0x00000000;   // clear LOAD bit
    *(timer_base_address + 0) = 0x00000082;   // UDT = 1, ENT = 1
}

// Function to stop the timer and get the number of cycles
u32 stop_timer_and_get_cycles(volatile u32 *timer_base_address) {
    u32 current_count = *(timer_base_address + 2);
    *(timer_base_address + 0) = 0x00000000;   // stop timer
    return (0xFFFFFFFF - current_count);
}

// Function to initialize the source and destination arrays
void initialize_arrays(volatile u32 *source_address, volatile u32 *dest_address, u32 array_size) {
    for (u32 i = 0; i < array_size; i++) {
        *(source_address + i) = i;
        *(dest_address + i) = 0xFFFFFFFF;
    }
}

// Function to compare the source and destination arrays and check if they are properly copied
bool compare_arrays(volatile u32 *source_address, volatile u32 *dest_address, u32 array_size) {
    for (u32 i = 0; i < array_size; i++) {
        if (*(source_address + i) != *(dest_address + i)) {
            cout << "Mismatch at index " << i << endl;
            return false;
        }
    }
    return true;
}

// Function to measure the number of cycles taken by the software copy
u32 measure_software_copy(volatile u32 *timer_base_address, volatile u32 *source_address, volatile u32 *dest_address, u32 array_size) {
    start_timer(timer_base_address);
    for (u32 i = 0; i < array_size; i++) {
        *(dest_address + i) = *(source_address + i);
    }
    return stop_timer_and_get_cycles(timer_base_address);
}

// Function to measure the number of cycles taken by the CDMA copy
u32 measure_cdma_copy(volatile u32 *timer_base_address, volatile u32 *cdma_base_address, volatile u32 *source_address, volatile u32 *dest_address, u32 array_size) {
    // Calculate the number of bytes to transfer
    u32 bytes_to_transfer = array_size * sizeof(u32);

    // Register map (word offsets) from the AXI CDMA Product Guide
    // *(cdma_base_address + 0)  = CDMACR  (0x00) -> control register
    // *(cdma_base_address + 1)  = CDMASR  (0x04) -> status register
    // *(cdma_base_address + 6)  = SA      (0x18) -> source address register
    // *(cdma_base_address + 8)  = DA      (0x20) -> destination address register
    // *(cdma_base_address + 10) = BTT     (0x28) -> bytes to transfer register (Initiates transfer)

    // Reset the CDMA by writing the appropriate bit pattern to the control register
    *cdma_base_address = 0x00000004; // bit 2 = 1 to reset the CDMA

    // Wait until reset completes
    while ((*cdma_base_address) & 0x00000004) { // wait until bit 2 is 0 i.e. reset is complete
        // do nothing - wait
    }

    // Verify CDMASR.IDLE = 1 before configuring
    while ((*(cdma_base_address + 1) & 0x00000002) == 0) { // wait until bit 1 is 1 i.e. CDMA has finished resetting
        // do nothing - wait
    }

    // Configure the CDMA in Simple DMA mode with no interrupts enabled
    *cdma_base_address = 0x00000000; // bit 3 = 0 means simple DMA mode

    // Load source and destination addresses into the SA and DA registers
    *(cdma_base_address + 6) = (u32)source_address;
    *(cdma_base_address + 8) = (u32)dest_address;

    // Flush cache before starting the DMA transfer
    Xil_DCacheFlush();

    start_timer(timer_base_address);

    // load the value of the number of bytes to transfer in the BTT register to initiate the DMA transfer
    *(cdma_base_address + 10) = bytes_to_transfer;

    // Poll until CDMASR.IDLE = 1 again to check if the DMA transfer is complete
    while ((*(cdma_base_address + 1) & 0x00000002) == 0) { // bit 1 = 0 means the DMA operation is in progress
        // wait for the transfer to complete i.e. wait until idle bit becomes 1
    }

    // Stop the timer and get the number of cycles taken by the DMA transfer
    u32 cycles = stop_timer_and_get_cycles(timer_base_address);

    // Invalidate destination cache so the CPU can see the updated DMA data
    Xil_DCacheInvalidateRange((INTPTR)dest_address, bytes_to_transfer);

    // Check DMA error bits
    if ((*(cdma_base_address + 1)) & CDMA_ERROR_MASK) {
        cout << "CDMA transfer failed. Status register = 0x" << hex << *(cdma_base_address + 1) << dec << endl;
    }

    // Return the number of cycles taken by the DMA transfer
    return cycles;
}

// Main function to test the software and CDMA copy
int main() {
    
    cout << "### Start of Q1 AXI Timer + CDMA Application ###" << endl;

    volatile u32 *cdma_base_address = (u32 *)XPAR_AXI_CDMA_0_BASEADDR;
    volatile u32 *source_address = (u32 *)XPAR_PS7_DDR_0_S_AXI_HP0_BASEADDR;
    volatile u32 *dest_address = (u32 *)XPAR_PS7_DDR_0_S_AXI_HP2_BASEADDR;
    volatile u32 *timer_base_address = (u32 *)XPAR_TMRCTR_0_BASEADDR;

    const u32 test_sizes[] = {1024, 4096, 8192, 16384, 32767, 1048576, ARRAY_MAX_SIZE};
    const u32 num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

    cout << "Array Size\tSW Cycles\tCDMA Cycles\tSpeedup\t\tResult" << endl;

    for (u32 test = 0; test < num_sizes; test++) {
        u32 current_size = test_sizes[test];

        // ---------------- Software copy ----------------
        initialize_arrays(source_address, dest_address, current_size);
        u32 sw_cycles = measure_software_copy(timer_base_address, source_address, dest_address, current_size);

        // Compare the source and destination arrays to check if they are properly copied
        bool sw_match = compare_arrays(source_address, dest_address, current_size);

        // ---------------- CDMA copy ----------------
        initialize_arrays(source_address, dest_address, current_size);
        u32 cdma_cycles = measure_cdma_copy(timer_base_address, cdma_base_address, source_address, dest_address, current_size);

        // Compare the source and destination arrays to check if they are properly copied
        bool cdma_match = compare_arrays(source_address, dest_address, current_size);

        double speedup = 0.0;
        if (cdma_cycles != 0) {
            speedup = (double)sw_cycles / (double)cdma_cycles;
        }

        // Print the results
        cout << current_size << "\t\t" << sw_cycles << "\t\t" << cdma_cycles << "\t\t" << speedup << "\t\t";

        // Check if the software and CDMA copy are properly copied
        if (sw_match && cdma_match)
            cout << "PASS";
        else
            cout << "FAIL";

        cout << endl;
    }

    cout << "### End of Application ###" << endl;
    return 0;
}