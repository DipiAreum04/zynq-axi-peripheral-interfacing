#include <iostream>
#include "xparameters.h"
#include "xil_types.h"

using namespace std;

int main()
{
    // AXI Timer register map for Timer 0:
    // Timer_Ptr + 0 --> TCSR0 (control/status)
    // Timer_Ptr + 1 --> TLR0  (load register / captured value in capture mode)
    // Timer_Ptr + 2 --> TCR0  (counter register)
    volatile u32* Timer_Ptr = (volatile u32*)XPAR_TMRCTR_0_BASEADDR;

    u32 capture_value;
    u32 new_capture_value;
    u32 diff;

    cout << "==== AXI Timer Capture Mode ====" << endl;
    cout << "Press SW5 switch once to capture the first timer value..." << endl;

    // Step 1: Put timer in capture mode with overwrite enabled
    *(Timer_Ptr + 0) = 0x19;

    // Step 2: Load 0 as the initial reset value into TLR0
    *(Timer_Ptr + 1) = 0x00;

    // Step 3: Load TLR0 into TCR0 by setting LOAD bit
    // 0x19 + bit5(LOAD) = 0x39
    // Then clear LOAD again so the timer can run
    *(Timer_Ptr + 0) = 0x39;   // capture mode + overwrite + capture enable + LOAD
    *(Timer_Ptr + 0) = 0x19;   // clear LOAD

    // Step 4: Start timer by setting ENT bit
    // 0x19 + bit7(ENT) = 0x99
    *(Timer_Ptr + 0) = 0x99;

    // Step 5: Wait until first capture happens
    // In capture mode, captured value is written into TLR0
    while (*(Timer_Ptr + 1) == 0x00000000)
    {
        // wait for first press of SW5
    }

    capture_value = *(Timer_Ptr + 1);

    cout << "First captured value = " << capture_value << endl;
    cout << "Press SW5 switch again to capture the second timer value..." << endl;

    // Step 6: Wait until a new captured value overwrites the old one
    while (*(Timer_Ptr + 1) == capture_value)
    {
        // wait for second press of SW5
    }

    new_capture_value = *(Timer_Ptr + 1);
    diff = new_capture_value - capture_value;

    cout << "Second captured value = " << new_capture_value << endl;
    cout << "Difference in clock cycles = " << diff << endl;

    // Stop timer (clear ENT, keep capture config)
    *(Timer_Ptr + 0) = 0x19;

    return 0;
}