#include "stdbool.h"
#include "xparameters.h"
#include "xil_types.h"
#include "xgpio.h"
#include "xil_io.h"
#include "xil_exception.h"

#include <iostream>
using namespace std;

int main()
{

    static XGpio GPIOInstance_PtrAandB;
    int xStatus;

    cout << "#### Application Starts ####" << endl;

    // Step-1: AXI GPIO Initialization
    xStatus = XGpio_Initialize(&GPIOInstance_PtrAandB, XPAR_AXI_GPIO_FOR_A_AND_B_DEVICE_ID);
    if (xStatus != XST_SUCCESS) {
        cout << "GPIO A Initialization FAILED" << endl;
        return XST_FAILURE;
    }

	// Step-2: AXI GPIO Set the Direction
	// XGpio_SetDataDirection(XGpio *InstancePtr, unsigned Channel, u32 DirectionMask);
	// We use only channel 1, and 0 is the the parameter for output
	XGpio_SetDataDirection(&GPIOInstance_PtrAandB, 1, 0);


    // Write 0 to the port to clear the LEDs 
	XGpio_DiscreteWrite(&GPIOInstance_PtrAandB, 1, 0);

	bool A[8] = {0, 1, 0, 0, 1, 1, 1, 0};
	bool B[8] = {1, 1, 1, 0, 0, 1, 1, 1};

	int i;

	// Step-3: Concatenate the values of A and B and write it to the GPIO
	// Concatenate A into A_in_u8 and B in B_in_u8
	u8 A_in_u8 = 0;
	u8 B_in_u8 = 0;

	for(i = 0; i < 8; i++) {
		A_in_u8 |= (A[i]<<(8-i-1));
	}

	for(i = 0; i < 8; i++) {
		B_in_u8 |= (B[i]<<(8-i-1));
	}

	// Shift the concatenated A array 8 bits to the right and bitwise OR it with the concatenated B array
	// to form the 16 bit data to be written to the GPIO port
	cout << "Concatenated values: " << ::hex <<  ((A_in_u8<<8) | B_in_u8)  << endl; 

	XGpio_DiscreteWrite(&GPIOInstance_PtrAandB, 1, ((A_in_u8<<8) | B_in_u8));

    cout << "#### Application Ends ####" << endl;

    return 0;
}
