#include "xparameters.h"
#include "iic.h"
#include "AD7991.h"
#include "xil_io.h"

/*****************************************************************************/
/************************** Macros Definitions *******************************/
/*****************************************************************************/
#define XPAR_PMOD_IO_SWITCH_BASEADDR XPAR_MB_1_MB1_SWITCH_S00_AXI_BASEADDR
#define IIC_BASEADDR  XPAR_IIC_0_BASEADDR
// Reference Voltage value
// Actual value * 10000
// E.g. VREF = 2.048 * 10000
#define VREF 20480
#define MAILBOX_CMD_ADDR (*(volatile unsigned *)(0x00007FFC)) // command from A9 to MB0
#define MAILBOX_DATA(x) (*(volatile unsigned *)(0x00007000+((x)*4)))
// Passed parameters in MAILBOX_WRITE_CMD
// bits 31:16 => delay between every channels samples in MS
// bits 15:8 => number of channels samples if 0 then infinite and the result will be placed only at MAILBOX_DATA
//				if non-zero then must be between 1 and 255 and the result will be placed starting at MAILBOX_DATA
// bits 7:5 => not used
// bit 4 => Channel 3 to be sampled Vref
// bit 3 => Channel 2 to be sampled
// bit 2 => Channel 1 to be sampled
// bit 1 => Channel 0 to be sampled
// bit 0 => 1 command issued, 0 command completed

u8 WriteBuffer[10];
u8 ReadBuffer[10];
u16 SlaveAddress;

int main()
{
    unsigned int sample;
    u8 useChan0, useChan1, useChan2, useChan3;
    u8 useVref, useFILT, useBIT, useSample;
    u32 delay;
    u32 cmd;
    u32 numofsamples;
    u8 numofchannels, i, j;

    //	Configuring PMOD IO Switch to connect to I2C[0].SCLK to pmod bit 2, I2C[0].SDA to pmod bit 3
    // rest of the bits are configured to default gpio channels, i.e. pmod bit[0] to gpio[0]
    // pmod bit[1] to gpio[1], pmod bit[4] to gpio[4], pmod bit[5] to gpio[5]
    // pmod bit[6] to gpio[6], pmod bit[7] to gpio[7]
	Xil_Out32(XPAR_PMOD_IO_SWITCH_BASEADDR+4,0x00000000); // isolate configuration port by writing 0 to slv_reg1[31]
	Xil_Out32(XPAR_PMOD_IO_SWITCH_BASEADDR,0x76549810); //
	Xil_Out32(XPAR_PMOD_IO_SWITCH_BASEADDR+4,0x80000000); // Enable configuration by writing 1 to slv_reg1[31]

    useVref=1;	// use internal VREF, make sure that JP1 is bridged between pin1 and center pin
    useFILT=0;	// filtering on SDA and SCL is enabled
    useBIT=0;	// use BIT delay enabled
    useSample=0;	// use Sample delay enabled
	while(1){
		while((MAILBOX_CMD_ADDR & 0x01)==0); // wait for bit[0] to become 1
		cmd = MAILBOX_CMD_ADDR;
		numofchannels=0;
		if((cmd >> 1) & 0x01) {
			useChan0=1;
			numofchannels++;
		}
		else
			useChan0 = 0;
		if((cmd >> 2) & 0x01) {
		    useChan1=1;
		    numofchannels++;
		}
		else
			useChan1=0;
		if((cmd >> 3) & 0x01) {
			useChan2=1;
			numofchannels++;
		}
		else
			useChan2=0;
		useChan3=0;		// Since VREF is used, Ch3 cannot be sampled
		if(((cmd >> 16) & 0x0ffff)==0)
			delay = 1000;	// set to 1 second if the field is set to 0
		else
			delay=(cmd >> 16) & 0x0ffff; // multiple of approximate milliseconds
		if(((cmd >> 8) & 0x000ff)==0)
			numofsamples = 0;	// indicate infinite number of samples
		else
			numofsamples=(cmd >> 8) & 0x0ff; // set to number of samples
		if(numofsamples==0) {
			// infinitely
			   AD7991_Config(useChan3,useChan2,useChan1,useChan0,
					   useVref,useFILT,useBIT,useSample);
			   while(1) {
				   for(j=0; j<numofchannels; j++) {
					   sample = AD7991_Read(1, VREF);
					   MAILBOX_DATA(j)=sample;
					   delay_ms(delay);
				   }
			   }
		}
		else
		{
			   AD7991_Config(useChan3,useChan2,useChan1,useChan0,
					   useVref,useFILT,useBIT,useSample);
			   for(i=0;i<numofsamples;i++)
			   {
				   for(j=0; j<numofchannels; j++) {
					   sample = AD7991_Read(1, VREF);
					   MAILBOX_DATA(i*numofchannels+j)=sample;
					   delay_ms(delay);
				   }
			   }
		}
		MAILBOX_CMD_ADDR = 0x0;
	}
   return 0;
}
