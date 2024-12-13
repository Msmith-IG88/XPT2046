#include <stdio.h>
#include "xparameters.h"	// Contains hardware addresses and bit masks
#include "xil_cache.h"		// Cache Drivers
#include "xintc.h"		// Interrupt Drivers
#include "xtmrctr.h"		// Timer Drivers
#include "xtmrctr_l.h" 		// Low-level timer drivers
#include "xil_printf.h" 	// Used for xil_printf()
#include "xgpio.h" 		// LED driver, used for General purpose I/i
#include "xspi.h"
#include "xspi_l.h"
#include "lcd.h"
#include "sleep.h"


#define CMD_READ_X         0xD0 // Command to read X-coordinate
#define CMD_READ_Y         0x90 // Command to read Y-coordinate
#define CMD_READ_Z1        0xB0 // Command to read Z1 (pressure)
#define CMD_READ_Z2        0xC0 // Command to read Z2 (pressure)

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320

// Calibration bounds
uint16_t x_min = 180;  // Raw value at top-left
uint16_t x_max = 1800; // Raw value at bottom-right
uint16_t y_min = 200;  // Raw value at top-left
uint16_t y_max = 1900; // Raw value at bottom-right

static XGpio dc;
static XSpi spi;

uint16_t read_touch(uint8_t command);
void get_touch_coordinates(uint16_t *x, uint16_t *y);
int is_touched(uint16_t *z1, uint16_t *z2);
void init_screen();
void configSpi();
void test_driver();
void printToScr(uint16_t x, uint16_t y);
uint16_t map_touch_to_screen(uint16_t raw, uint16_t raw_min, uint16_t raw_max, uint16_t screen_size);

int main()
{
    Xil_ICacheInvalidate();
	Xil_ICacheEnable();
	Xil_DCacheInvalidate();
	Xil_DCacheEnable();

	XSpi_Config *spiConfig;	/* Pointer to Configuration data */

	u32 status;
	u32 controlReg;
	uint16_t x = 0, y = 0, z1 = 0, z2 = 0;

	Xil_ICacheEnable();
	Xil_DCacheEnable();
	print("---Entering main (CK)---\n\r");

	/*
	 * Initialize the GPIO driver so that it's ready to use,
	 * specify the device ID that is generated in xparameters.h
	 */
	status = XGpio_Initialize(&dc, XPAR_SPI_DC_DEVICE_ID);
	if (status != XST_SUCCESS)  {
		xil_printf("Initialize GPIO dc fail!\n");
		return XST_FAILURE;
	}

	/*
	 * Set the direction for all signals to be outputs
	 */
	XGpio_SetDataDirection(&dc, 1, 0x0);

	/*
	 * Initialize the SPI driver so that it is  ready to use.
	 */
	spiConfig = XSpi_LookupConfig(XPAR_SPI_DEVICE_ID);
	if (spiConfig == NULL) {
		xil_printf("Can't find spi device!\n");
		return XST_DEVICE_NOT_FOUND;
	}

	status = XSpi_CfgInitialize(&spi, spiConfig, spiConfig->BaseAddress);
	if (status != XST_SUCCESS) {
		xil_printf("Initialize spi fail!\n");
		return XST_FAILURE;
	}

	/*
	 * Reset the SPI device to leave it in a known good state.
	 */
	XSpi_Reset(&spi);

	status = XSpi_SetOptions(&spi, XSP_MASTER_OPTION | XSP_MANUAL_SSELECT_OPTION);
	if (status != XST_SUCCESS) {
	    xil_printf("Failed to set SPI options\n");
	    return XST_FAILURE;
	}

	/*
	 * Setup the control register to enable master mode
	 */
	controlReg = XSpi_GetControlReg(&spi);
	XSpi_SetControlReg(&spi,
			(controlReg | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK) &
			(~XSP_CR_TRANS_INHIBIT_MASK));

	XSpi_Start(&spi);

	XSpi_IntrGlobalDisable(&spi);

	// Select 1st slave device
	u32 slaveSelectReg = XSpi_GetSlaveSelectReg(&spi);
	xil_printf("SPI Slave Select Register: 0x%08X\n", slaveSelectReg);

	//Initialize the LCD Screen
	init_screen();

	//Polling loop for touches
	while (1) {
		//Check z data for touches
		if (is_touched(&z1, &z2)) {
			get_touch_coordinates(&x, &y);//get coordinates when touched
			printf("Touch detected: X = %d, Y = %d, Z1 = %d, Z2 = %d\n", x, y, z1, z2);
			printToScr(x, y);//Print coordinates to the display
		} else {
			printf("No touch detected.\n");
		}
	}

	xil_printf("End\n");
	return 0;
}

//Debug function to check the SPI driver status
void test_driver(){
	u32 controlReg;

	u32 options = XSpi_GetOptions(&spi);
	xil_printf("Current SPI Options: 0x%08X\n", options);

	if (options & XSP_MASTER_OPTION) {
		xil_printf("SPI is in master mode.\n");
	}
	if (options & XSP_MANUAL_SSELECT_OPTION) {
		xil_printf("SPI is in manual CS mode.\n");
	} else {
		xil_printf("SPI is in automatic CS mode.\n");
	}
	controlReg = XSpi_GetControlReg(&spi);
	xil_printf("SPI Control Register: 0x%08X\n", controlReg);

	if (controlReg & XSP_CR_ENABLE_MASK) {
		xil_printf("SPI controller is enabled.\n");
	} else {
		xil_printf("SPI controller is disabled.\n");
	}

	if (controlReg & XSP_CR_MASTER_MODE_MASK) {
		xil_printf("SPI is in master mode.\n");
	} else {
		xil_printf("SPI is in slave mode.\n");
	}

	if (controlReg & XSP_CR_TRANS_INHIBIT_MASK) {
		xil_printf("SPI transfers are inhibited.\n");
	} else {
		xil_printf("SPI transfers are allowed.\n");
	}
	u32 statusReg = XSpi_GetStatusReg(&spi);
	xil_printf("SPI Status Register: 0x%08X\n", statusReg);

	if (statusReg & XSP_SR_TX_FULL_MASK) {
		xil_printf("SPI transmit FIFO is full.\n");
	}

	if (statusReg & XSP_SR_RX_EMPTY_MASK) {
		xil_printf("SPI receive FIFO is empty.\n");
	}

	u32 slaveSelectReg = XSpi_GetSlaveSelectReg(&spi);
	xil_printf("SPI Slave Select Register: 0x%08X\n", slaveSelectReg);

	if ((~slaveSelectReg) & 0x01) {
		xil_printf("Slave 0 (LCD) is selected.\n");
	}

	if ((~slaveSelectReg) & 0x02) {
		xil_printf("Slave 1 (Touch Controller) is selected.\n");
	}
}

//Reconfigure the SPI driver
void configSpi(){
	u32 status;
	u32 controlReg;

	/*
	 * Setup the control register to enable manual slave select
	 */
	status = XSpi_SetOptions(&spi, XSP_MASTER_OPTION | XSP_MANUAL_SSELECT_OPTION);
	if (status != XST_SUCCESS) {
		xil_printf("Failed to set SPI options\n");
		return XST_FAILURE;
	}

	/*
	 * Setup the control register to enable master mode
	 */
	controlReg = XSpi_GetControlReg(&spi);
	XSpi_SetControlReg(&spi,
			(controlReg | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK) &
			(~XSP_CR_TRANS_INHIBIT_MASK));

	//Start the spi
	XSpi_Start(&spi);

	//disable any global interrupts
	XSpi_IntrGlobalDisable(&spi);
}

// Print any touch data to the lcd display
void printToScr(uint16_t x, uint16_t y){
	//Select the LCD Display slave
	XSpi_SetSlaveSelectReg(&spi, ~0x01);
	//XGpio_DiscreteWrite(&dc, 1, 0);

	char buffer[32];
	uint16_t screen_x, screen_y;

	//Get mapped x and y data
	screen_x = map_touch_to_screen(x, x_min, x_max, SCREEN_WIDTH);
	screen_y = map_touch_to_screen(y, y_min, y_max, SCREEN_HEIGHT);

  // Clear the previous coordinate display
	setColor(0, 0, 0); // Set background color (black)
	fillRect(10, 10, 150, 40); // Clear the coordinate display area

	// Print the new coordinates
	setColor(0, 255, 0); // Set text color (green)
	setColorBg(0, 0, 0); // Set background color (black)
	sprintf(buffer, "X: %d", screen_x); // Format X value
	lcdPrint(buffer, 20, 20); // Print X at (20, 20)

	setColor(0, 0, 0); // Set background color (black)
	fillRect(40, 40, 150, 60);
	setColor(0, 255, 0); // Set text color (green)
	setColorBg(0, 0, 0); // Set background color (black)
	// Print Y value below X
	sprintf(buffer, "Y: %d", screen_y); // Format Y value
	lcdPrint(buffer, 20, 40); // Print Y at (20, 40)

	// Use to print mapped coordinates
/*
	sprintf(buffer, "X: %d Y: %d", screen_x, screen_y); // Format the coordinates
	sprintf(buffer, "X: %d Y: %d", x, y);
	lcdPrint(buffer, 20, 20);
*/
	//Reset the spi to empty the FIFO
	XSpi_Reset(&spi);
	configSpi();

}

// Initialize the LCD Display
void init_screen(){
	//Select the LCD Display slave
	XSpi_SetSlaveSelectReg(&spi, ~0x01);

	//Initial screen in lcd.c
	initLCD();
	//clear the screen
	clrScr();

	//Deselect the LCD slave
	XSpi_SetSlaveSelectReg(&spi, ~0x00);

	xil_printf("End scr init\n");
	//reset the spi and reconfigure
	XSpi_Reset(&spi);
	configSpi();
}

// Request and retrieve a X or Y data from controller using command byte
uint16_t read_touch(uint8_t command) {
	//buffers to send to the touch controller
    uint8_t tx_buf[3] = {command, 0x00, 0x00};
    uint8_t rx_buf[3] = {0};
    int status;

    // Deselect LCD and select touch controller
    XSpi_SetSlaveSelect(&spi, 0x02);
    usleep(3000);
    // Transfer command and read response
    status = XSpi_Transfer(&spi, tx_buf, rx_buf, 3);
    if (status != XST_SUCCESS) {
        xil_printf("SPI Transfer failed! Status = %d\r\n", status);
        return 0; // Return default value on failure
    }

    // Deselect touch controller
    XSpi_SetSlaveSelectReg(&spi, ~0x00);
    usleep(3000);

    // Combine high and low bytes, discard least significant 4 bits
    uint16_t result = ((rx_buf[1] << 8) | rx_buf[2]) >> 4;
    return result;
}

// Get X and Y coordinates from the touch controller
void get_touch_coordinates(uint16_t *x, uint16_t *y) {
    *x = read_touch(CMD_READ_X);
    *y = read_touch(CMD_READ_Y);

    //Reset the spi and reconfigure
    XSpi_Reset(&spi);
    configSpi();
}

// Check if the screen is being touched
int is_touched(uint16_t *z1, uint16_t *z2) {
    *z1 = read_touch(CMD_READ_Z1);
    *z2 = read_touch(CMD_READ_Z2);

    // Simple pressure check: return true if Z2 > Z1
    return (*z1 > 10 && *z2 < 2000);
}

// Map the raw touch coordinates based on the LCD dimensions
uint16_t map_touch_to_screen(uint16_t raw, uint16_t raw_min, uint16_t raw_max, uint16_t screen_size) {
    if (raw < raw_min){
    	raw = raw_min; // Clamp to min
    }
    if (raw > raw_max){
    	raw = raw_max; // Clamp to max
    }
    return (raw - raw_min) * screen_size / (raw_max - raw_min);
}

