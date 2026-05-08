#include <iostream>
#include "xparameters.h"
#include "xil_types.h"
#include "xtmrctr.h"
#include "xil_io.h"
#include "xil_exception.h"
#include "xscugic.h"

using namespace std;

// Instance of the Interrupt Controller
XScuGic InterruptController;

// The configuration parameters of the controller
static XScuGic_Config *GicConfig;

// Timer Instance
XTmrCtr TimerInstancePtr;


void Timer_InterruptHandler(void) {
    cout << "I am the interrupt handler" << endl;
}


int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr)
{
	// Connect the interrupt controller interrupt handler to the hardware interrupt handling logic in the ARM processor
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler) XScuGic_InterruptHandler, XScuGicInstancePtr);

	// Enable interrupts in the ARM
	Xil_ExceptionEnable();
	return XST_SUCCESS;
}


int ScuGicInterrupt_Init(u16 DeviceId,XTmrCtr *TimerInstancePtr)
{
	int Status;
	
	// Initialize the interrupt controller driver so that it is ready to use
	GicConfig = XScuGic_LookupConfig(DeviceId);
	if (GicConfig == NULL) {
		return XST_FAILURE;
	}

	// Configure the interrupt controller
	Status = XScuGic_CfgInitialize(&InterruptController, GicConfig, GicConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// Setup the Interrupt System
	Status = SetUpInterruptSystem(&InterruptController);
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	// Write the End of Interrupt (EOI) register
	XScuGic_CPUWriteReg(&InterruptController, XSCUGIC_EOI_OFFSET, XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR);

	// Connect a device driver handler that will be called when an interrupt for the device occurs 
	// the device driver handler performs the specific interrupt processing for the device
	Status = XScuGic_Connect(&InterruptController, XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR, (Xil_ExceptionHandler)XTmrCtr_InterruptHandler, (void *)TimerInstancePtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// Enable the interrupt for the device
	XScuGic_Enable(&InterruptController, XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR);

	return XST_SUCCESS;
}


// Main Function
int main()
{
    cout << "==== AXI Timer Interrupt Application Starts ====" << endl;
    int xStatus;

	// Step-1: AXI Timer Initialization
	xStatus = XTmrCtr_Initialize(&TimerInstancePtr, XPAR_AXI_TIMER_0_DEVICE_ID);
	if (xStatus != XST_SUCCESS) {
		cout << "TIMER INIT FAILED " << endl;
		if (xStatus == XST_DEVICE_IS_STARTED) {
			cout << "TIMER has already started" << endl;
			cout << "Please power cycle your board, and re-program the bitstream" << endl;
		}
		return XST_FAILURE;
	}

	// Step-2: Set Timer Handler
	XTmrCtr_SetHandler(&TimerInstancePtr, (XTmrCtr_Handler)Timer_InterruptHandler, &TimerInstancePtr);

    // Initialize time pointer with value from xparameters.h file
	unsigned int* timer_ptr = (unsigned int* )XPAR_AXI_TIMER_0_BASEADDR;

	// Step-3: Load the reset value
	*(timer_ptr+ 1) = 0xffffff00;

	// Step-4: Set the timer options
	// Configure timer in generate mode, count up, interrupt enabled with autoreload of load register
	*(timer_ptr)  = 0x0f4 ;

	// Step-5: SCUGIC interrupt controller Initialization
	// Register the Timer ISR
	xStatus= ScuGicInterrupt_Init(XPAR_PS7_SCUGIC_0_DEVICE_ID, &TimerInstancePtr);
	if (xStatus != XST_SUCCESS) {
		cout << " :( SCUGIC INIT FAILED )" << endl;
		return 1;
	}

	// Wait for user input to start the timer
	char input;
	cout << "Press any key to start the timer" << endl;
	cin >> input ;
	cout << "You pressed "<<  input << endl;
    cout << "Enabling the timer to start" << endl;

    *(timer_ptr) = 0x0d4 ;   // deassert the load 5 to allow the timer to start counting

    // let timer run forever generating periodic interrupts
    while (true) {
        // wait forever and let the timer generate periodic interrupts
    }

    return 0;
}

