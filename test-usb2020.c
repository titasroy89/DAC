/*
 *
 *  Copyright (c) 2014   Warren Jasper <wjasper@tx.ncsu.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <asm/types.h>

#include "pmd.h"
#include "usb-2020.h"

#define MAX_COUNT  (0xffff)
#define FALSE 0
#define TRUE 1

/* Test Program */
int toContinue()
{
  int answer;
  answer = 0; //answer = getchar();
  printf("Continue [yY]? ");
  while((answer = getchar()) == '\0' ||
    answer == '\n');
  return ( answer == 'y' || answer == 'Y');
}

int main (int argc, char **argv)
{
  usb_dev_handle *udev = NULL;

  double frequency;
  float temperature;
  float table_AIN[NGAINS_2020][2];
  ScanList list[NCHAN_2020];  // scan list used to configure the A/D channels.
  int ch;
  int i;
  int nSamples = 0;
  __u8 input;
  int temp, ret;
  __u8 options;
  char serial[9];
  __u16 version;

  __u16 value;
  __u16 dataAIn[256*20];  // holds 16 bit unsigned analog input data, must be multiple of 256.
  __u8 mode, gain, channel;

  udev = NULL;
  if ((udev = usb_device_find_USB_MCC(USB2020_PID))) {
    printf("Success, found a USB-2020.\n");
  } else {
    printf("Failure, did not find a USB-2020!\n");
    return 0;
  }
  // some initialization
  usbInit_USB2020(udev);

  //print out the wMaxPacketSize.
  printf("wMaxPacketSize = %d\n", usb_get_max_packet_size(udev,0));

  usbBuildGainTable_USB2020(udev, table_AIN);
  for (i = 0; i < NGAINS_2020; i++) {
    printf("Gain: %d   Slope = %f   Offset = %f\n", i, table_AIN[i][0], table_AIN[i][1]);
  }

  while(1) {
    printf("\nUSB 2020 Testing\n");
    printf("----------------\n");
    printf("Hit 'b' to blink\n");
    printf("Hit 'd' to test digitial IO\n");
    printf("Hit 'i' to test Analog Input\n");
    printf("Hit 'I' to test Analog Input Scan\n");
    printf("Hit 'r' to reset the device\n");
    printf("Hit 's' to get serial number\n");
    printf("Hit 'S' to get Status\n");
    printf("Hit 'T' to get temperature\n");
    printf("Hit 'v' to get version numbers\n");
    printf("Hit 'e' to exit\n");

    while((ch = getchar()) == '\0' || ch == '\n');
    switch(ch) {
      case 'b': /* test to see if LED blinks */
        printf("Enter number or times to blink: ");
        scanf("%hhd", &options);
        usbBlink_USB2020(udev, options);
	break;
      case 'd':
        printf("\nTesting Digital I/O...\n");
	printf("connect pins  [0-3] <--> [4-7]\n");
	usbDTristateW_USB2020(udev,0xf0);
	printf("Digital Port 1 Tristate Register = %#x\n", usbDTristateR_USB2020(udev));
	do {
          printf("Enter a  number [0-0xf]: " );
          scanf("%x", &temp);
          temp = temp;
          usbDLatchW_USB2020(udev, (__u16)temp);
	  temp = usbDLatchR_USB2020(udev);
          input = (usbDPort_USB2020(udev) >> 4) & 0xf;
          input = ~input & 0xf;
          printf("The number you entered = %#x   Latched value = %#x\n\n",input, temp);
	  for (i = 0; i < 4; i++) {
	    printf("Bit %d = %d\n", i, (temp>>i)&0x1);
	  }
        } while (toContinue());
        break;
      case 'e':
        cleanup_USB2020(udev);
        return 0;
      case 'i':
	printf("Input channel [0-1]: ");
	scanf("%hhd", &channel);
	printf("Gain Range for channel %d: 1 = +/-10V  2 = +/- 5V  3 = +/- 2V  4 = +/- 1V: ",channel);
	while((ch = getchar()) == '\0' || ch == '\n');
	switch(ch) {
	  case '1': gain = BP_10V; break;
  	  case '2': gain = BP_5V; break;
	  case '3': gain = BP_2V; break;
	  case '4': gain = BP_1V; break;
	  default:  gain = BP_10V; break;
	}
	mode = (LAST_CHANNEL| SINGLE_ENDED);
	list[0].range = gain;
        list[0].mode = mode;
	list[0].channel = channel;
	usbAInConfig_USB2020(udev, list);
	for (i = 0; i < 20; i++) {
	  value = usbAIn_USB2020(udev, channel);
	  value = rint(value*table_AIN[gain][0] + table_AIN[gain][1]);
	  printf("Channel %d  Mode = %#x  Gain = %d Sample[%d] = %#x Volts = %lf\n",
		 list[0].channel, list[0].mode, list[0].range, i, value, volts_USB2020(udev,gain,value));
	  usleep(50000);	  
	}
        break;
      case 'I':
	printf("Testing USB-2020 Analog Input Scan.\n");
	usbAInScanStop_USB2020(udev);
	usbAInScanClearFIFO_USB2020(udev);
        printf("Enter number of samples (less than 5000): ");
        scanf("%d", &nSamples);
	printf("Input channel [0-1]: ");
        scanf("%hhd", &channel);
	printf("Gain Range for channel %d: 1 = +/-10V  2 = +/- 5V  3 = +/- 2V  4 = +/- 1V: ",channel);
	while((ch = getchar()) == '\0' || ch == '\n');
	switch(ch) {
	  case '1': gain = BP_10V; break;
  	  case '2': gain = BP_5V; break;
	  case '3': gain = BP_2V; break;
	  case '4': gain = BP_1V; break;
	  default:  gain = BP_10V; break;
	}
	mode = (LAST_CHANNEL | SINGLE_ENDED);
        list[0].range = gain;
        list[0].mode = mode;
        list[0].channel = channel;
	usbAInConfig_USB2020(udev, list);
        printf("Enter sampling frequency [Hz]: ");
	scanf("%lf", &frequency);
        options = 0x0;
        for (i = 0; i < nSamples; i++) {
          dataAIn[i] = 0xbeef;
	}
	usbAInScanStart_USB2020(udev, nSamples, 0, frequency, nSamples-1, options);
	ret = usbAInScanRead_USB2020(udev, nSamples, 1, &dataAIn[0]);
	printf("Number samples read = %d\n", ret/2);
	for (i = 0; i < nSamples; i++) {
          dataAIn[i] &= 0xfff;
	  dataAIn[i] = rint(dataAIn[i]*table_AIN[gain][0] + table_AIN[gain][1]);
	  printf("Channel %d  Mode = %d  Gain = %d Sample[%d] = %#x Volts = %lf\n", channel,
		 mode, gain, i, dataAIn[i], volts_USB2020(udev,gain,dataAIn[i]));
	}
	usbAInScanStop_USB2020(udev);
	usbAInScanClearFIFO_USB2020(udev);
        break;
      case 'r':
	usbReset_USB2020(udev);
	return 0;
	break;
      case 's':
        usbGetSerialNumber_USB2020(udev, serial);
        printf("Serial number = %s\n", serial);
        break;
      case 'S':
        printf("Status = %#x\n", usbStatus_USB2020(udev));
	break;
      case 'T':
        usbTemperature_USB2020(udev, &temperature);
	printf("Temperature = %.2f deg C  or  %.2f deg F \n", temperature, 9.0/5.0*temperature + 32.);
	break;
      case 'v':
	version = 0xbeef;
        usbFPGAVersion_USB2020(udev, &version);
	printf("FPGA version %02x.%02x\n", version >> 0x8, version & 0xff);
	break;
    default:
        break;
    }
  }
}

