/*
 *
 *  Copyright (c) 2013   Warren Jasper <wjasper@tx.ncsu.edu>
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
#include "usb-1608FS-Plus.h"

#define MAX_COUNT     (0xffff)
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

  float table_AIN[NGAINS_USB1608FS_PLUS][NCHAN_USB1608FS_PLUS][2];

  int ch;
  int i, j, k, m;
  __u8 input;
  int temp;
  __u8 options;
  char serial[9];
  __u8 channel, channels;
  __u8 range;
  __u8 ranges[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  __u16 value;
  __u32 count;
  double frequency;
  int ret;
  __u16 sdataIn[8*512]; // holds 16 bit unsigned analog input data
  __u16 dataC[256][8];  // corrected data
  __u16 data;
  int nchan, repeats, scan;
  int flag;

start:  udev = NULL;
  if ((udev = usb_device_find_USB_MCC(USB1608FS_PLUS_PID))) {
    printf("Success, found a USB 1608FS-Plus!\n");
  } else {
    printf("Failure, did not find a USB 1608FS-Plus!\n");
    return 0;
  }

  // some initialization

  //print out the wMaxPacketSize.
  printf("wMaxPacketSize = %d\n", usb_get_max_packet_size(udev,0));

  usbBuildGainTable_USB1608FS_Plus(udev, table_AIN);
  for (i = 0; i < NGAINS_USB1608FS_PLUS; i++ ) {
    for (j = 0; j < NCHAN_USB1608FS_PLUS; j++) {
      printf("Calibration Table: Range = %d Channel = %d Slope = %f   Offset = %f\n", 
	     i, j, table_AIN[i][j][0], table_AIN[i][j][1]);
    }
  }
  
  while(1) {
    printf("\nUSB 1608FS-Plus Testing\n");
    printf("----------------\n");
    printf("Hit 'b' to blink\n");
    printf("Hit 'c' to test counter\n");
    printf("Hit 'C' to test continuous sampling at 50 kHz. all channels.\n");
    printf("Hit 'd' to test digitial IO\n");
    printf("Hit 'i' to test Analog Input\n");
    printf("Hit 'I' to test Analog Input Scan\n");
    printf("Hit 'x' to test Analog Input Scan (Multi-channel)\n");
    printf("Hit 'r' to reset the device\n");
    printf("Hit 's' to get serial number\n");
    printf("Hit 'S' to get Status\n");
    printf("Hit 'e' to exit\n");

    while((ch = getchar()) == '\0' || ch == '\n');
    switch(ch) {
      case 'b': /* test to see if LED blinks */
        printf("Enter number or times to blink: ");
        scanf("%hhd", &options);
        usbBlink_USB1608FS_Plus(udev, options);
	break;
      case 'c':
        usbCounterInit_USB1608FS_Plus(udev);
        printf("Connect DIO0 to CTR0\n");
	usbDTristateW_USB1608FS_Plus(udev, 0xf0);
        toContinue();
        for (i = 0; i < 100; i++) {
	  usbDLatchW_USB1608FS_Plus(udev, 0x0);
	  usbDLatchW_USB1608FS_Plus(udev, 0x1);
        }
        printf("Count = %d.  Should read 100.\n", usbCounter_USB1608FS_Plus(udev));
        break;      
      case 'd':
        printf("\nTesting Digital I/O...\n");
	printf("connect pins DIO[0-3] <--> DIO[4-7]\n");
	usbDTristateW_USB1608FS_Plus(udev,0xf0);
	printf("Digital Port Tristate Register = %#x\n", usbDTristateR_USB1608FS_Plus(udev));
	do {
          printf("Enter a  number [0-0xf] : " );
          scanf("%x", &temp);
	  temp &= 0xf;
          usbDLatchW_USB1608FS_Plus(udev, (__u8)temp);
	  temp = usbDLatchR_USB1608FS_Plus(udev);
          input = usbDPort_USB1608FS_Plus(udev);
	  input = (input >> 0x4) & 0xf;
          printf("The number you entered = %#x   Latched value = %#x\n\n",input, temp);
	  for (i = 0; i < 4; i++) {
	    printf("Bit %d = %d\n", i, (temp>>i)&0x1);
	  }
        } while (toContinue());
        break;
      case 'i':
	printf("Input channel [0-7]: ");
	scanf("%hhd", &channel);
        printf("Input range [0-7]: ");
	scanf("%hhd", &range);
	for (i = 0; i < 20; i++) {
	  value = usbAIn_USB1608FS_Plus(udev, channel, range);
	  value = rint(value*table_AIN[range][channel][0] + table_AIN[range][channel][1]);
	  printf("Range %d  Channel %d   Sample[%d] = %#x Volts = %lf\n",
		 range, channel,  i, value, volts_USB1608FS_Plus(udev, value, range));
	  usleep(50000);	  
	}
        break;
      case 'I':
	printf("Testing USB-1608FS_Plus Analog Input Scan.\n");
	usbAInScanStop_USB1608FS_Plus(udev);
        printf("Enter number of scans (less than 512): ");
        scanf("%d", &count);
	printf("Input channel 0-7: ");
        scanf("%hhd", &channel);
        printf("Enter sampling frequency [Hz]: ");
	scanf("%lf", &frequency);
        printf("Enter Range [0-7]: ");
        scanf("%hhd", &range);
        ranges[channel] = range;
        usbAInScanStop_USB1608FS_Plus(udev);
	usbAInScanClearFIFO_USB1608FS_Plus(udev);
        usbAInScanConfig_USB1608FS_Plus(udev, ranges);
	sleep(1);
        usbAInScanConfigR_USB1608FS_Plus(udev, ranges);
        for (i = 0; i < 8; i++) {
          printf("Channel %d     range %d\n", i, ranges[i]);
	}
	usbAInScanStart_USB1608FS_Plus(udev, count, frequency, (0x1<<channel), 0);
	ret = usbAInScanRead_USB1608FS_Plus(udev, count, 1, sdataIn);
	printf("Number samples read = %d\n", ret/2);
	for (i = 0; i < count; i++) {
	  sdataIn[i] = rint(sdataIn[i]*table_AIN[range][channel][0] + table_AIN[range][channel][1]);
	  printf("Range %d Channel %d  Sample[%d] = %#x Volts = %lf\n", range, channel,
		 i, sdataIn[i], volts_USB1608FS_Plus(udev,sdataIn[i],range));
	}
        break;
      case 'C':
      	printf("Testing USB-1608FS_Plus Analog Input Scan in Continuous mode 8 channels\n");
        printf("Hit any key to exit\n");
	usbAInScanStop_USB1608FS_Plus(udev);
        count = 0;          // for continuous mode
        channel = 0xff;     // one bit for each channel
        frequency = 50000.;
        range = 0;
        for (i = 0; i < 8; i++) {
          ranges[i] = range;
	}
        usbAInScanStop_USB1608FS_Plus(udev);
	usbAInScanClearFIFO_USB1608FS_Plus(udev);
        usbAInScanConfig_USB1608FS_Plus(udev, ranges);
	sleep(1);
        i = 0;
	usbAInScanStart_USB1608FS_Plus(udev, count, frequency, channel, 0);
	flag = fcntl(fileno(stdin), F_GETFL);
	fcntl(0, F_SETFL, flag | O_NONBLOCK);
        do {
  	  ret = usbAInScanRead_USB1608FS_Plus(udev, 256, 8, sdataIn);
          for (scan = 0; scan < 256; scan++) { //for each scan
	    for (channel = 0; channel < 8; channel++){  // for each channel in a scan
              dataC[scan][channel] = rint(sdataIn[scan*8+channel]*table_AIN[range][channel][0] + table_AIN[range][channel][1]);
	    }
	  }
          if (i%100 == 0) {
            printf("Scan = %d\n", i);
	  }
          i++;
	} while (!isalpha(getchar()));
	fcntl(fileno(stdin), F_SETFL, flag);
        usbAInScanStop_USB1608FS_Plus(udev);
        usbReset_USB1608FS_Plus(udev);
        cleanup_USB1608FS_Plus(udev);
        sleep(2); // let things settle down.
        goto start;
        break;
      case 'x':
        printf("Testing USB-1608FS_Plus Mult-Channel Analog Input Scan.\n");
        usbAInScanStop_USB1608FS_Plus(udev);
        printf("enter number of channels (1-8) :");
        scanf("%d", &nchan);
        printf("Enter number of scans (less than 512): ");
        scanf("%d", &count);
        printf("Enter number of repeats: ");
        scanf("%d", &repeats);
        // Build bitmap for the first nchan in channels.
        channels = 0;
        for (i = 0; i < nchan; i++)
	  channels |= (1 << i);
        printf ("channels: %02X   count:%d\n", channels, count);
        frequency = 10000.;
        // Always use BP_10V to make it easy (BP_10V is 0...)
        range = 0;
        memset(ranges, 0, sizeof(ranges));
        usbAInScanConfig_USB1608FS_Plus(udev, ranges);
        // Run a loop for the specified number of repeats and
        // show the results...
        for (m = 0; m < repeats; m++) {
	  printf("\n\n---------------------------------------");
	  printf("\nrepeat: %d\n", m);
	  usbAInScanStart_USB1608FS_Plus(udev, count, frequency, channels, 0);
	  ret = usbAInScanRead_USB1608FS_Plus(udev, count, nchan, sdataIn);
	  printf("Number samples read = %d\n", ret/2);
	  if (ret != count * nchan * 2) {
	    printf("***ERROR***  ret = %d   count = %d  nchan = %d\n", ret, count, nchan);
	    continue;
	  } /* if (ret != count * nchan * 2) */
	  for (i = 0; i < count; i++) {
	    printf("%6d", i);
	    for (j = 0; j < nchan; j++)	{
	      k = i*nchan + j;
	      data = rint(sdataIn[k]*table_AIN[range][j][0] + table_AIN[range][j][1]);
	      printf(", %8.4f", volts_USB1608FS_Plus(udev, data, range));
	    } /* for (j - 0; j < 8, j++) */
	    printf("\n");
	  } /* for (i = 0; i < count; i++) */
	} /* for (m = 0; m < repeats; m++) */
	printf("\n\n---------------------------------------");
	break;
      case 'r':
        usbReset_USB1608FS_Plus(udev);
        break;
      case 's':
        usbGetSerialNumber_USB1608FS_Plus(udev, serial);
        printf("Serial number = %s\n", serial);
        break;
      case 'S':
        printf("Status = %#x\n", usbStatus_USB1608FS_Plus(udev));
	break;
      case 'e':
        cleanup_USB1608FS_Plus(udev);
        return 0;
      default:
        break;
    }
  }
}

