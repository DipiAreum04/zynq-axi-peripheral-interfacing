#include "stdbool.h"
#include "xparameters.h"
#include "xil_types.h"
#include <iostream>
using namespace std;

int main()
{

    cout << "#### PWM LED Blinking Application Starts ###" << endl;

    // base address of AXI timer = TCSR0
    volatile u32 *Timer_Ptr = (u32 *)XPAR_TMRCTR_0_BASEADDR;

    // Register map (word offsets) based on the AXI Timer datasheet
    // Timer_Ptr[0] = TCSR0  (0x00)
    // Timer_Ptr[1] = TLR0   (0x04)
    // Timer_Ptr[2] = TCR0   (0x08)
    // Timer_Ptr[4] = TCSR1  (0x10)
    // Timer_Ptr[5] = TLR1   (0x14)
    // Timer_Ptr[6] = TCR1   (0x18)

    const double CLOCK_FREQ = 50000000.0;
    double period;
    double duty_cycle;

    // Configure the timer for PWM mode
    // Bit 9 = 1, bit 4 = 1, bit 2 = 1, bit 1 = 1, so hex value = 0x216
    *Timer_Ptr = 0x216;       // Configure TCSR0
    *(Timer_Ptr + 4) = 0x216; // Configure TCSR1

    while (true)
    {

        // Ask the user to input desired PWM period and duty cycle
        cout << "Enter desired PWM period (in seconds): ";
        cin >> period;
        cout << endl;

        cout << "Enter desired duty cycle (in % ranging from 0 to 100): ";
        cin >> duty_cycle;
        cout << endl;

        if (period <= 0.0)
        {
            cout << "Period must be > 0" << endl;
            continue;
        }

        if (duty_cycle < 0.0 || duty_cycle > 100.0)
        {
            cout << "Duty cycle must be between 0 and 100" << endl;
            continue;
        }

        // Calculate high time in seconds
        double high_time = period * (duty_cycle / 100.0);

        // Stop the timers before updating (Clear bit 7 - ENT)
        *(Timer_Ptr) &= ~(1 << 7);
        *(Timer_Ptr + 4) &= ~(1 << 7);

        // Calculate load values based on formulas for period and high time
        u32 TLR0_value = (u32)((period * CLOCK_FREQ) - 2);
        u32 TLR1_value = (u32)((high_time * CLOCK_FREQ) - 2);

        cout << "Loading Timer0 (period) with: " << TLR0_value << endl;
        cout << "Loading Timer1 (high time) with: " << TLR1_value << endl;

        // Load the calculated values into the load registers (TLR0 and TLR1)
        *(Timer_Ptr + 1) = TLR0_value;
        *(Timer_Ptr + 5) = TLR1_value;

        // Set Load bit (bit 5) in each of the control registers (TCSR0 and TCSR1)
        *(Timer_Ptr) |= 1 << 5;
        *(Timer_Ptr + 4) |= 1 << 5;

        // Start the PWM application by clearing load bit (bit 5 = 0) and then setting the enable bit (bit 7 = 1)
        // Bit 9 = 1, bit 7 = 1, bit 4 = 1, bit 2 = 1, bit 1 = 1, so hex value = 0x296
        *(Timer_Ptr) = 0x296;
        *(Timer_Ptr + 4) = 0x296;
        cout << "PWM application started! The LED should start blinking at the desired PWM rate." << endl;
    }

    return 0;
}