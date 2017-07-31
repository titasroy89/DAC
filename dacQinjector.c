/*
 *
 *  Copyright (c) 2006  Warren Jasper <wjasper@tx.ncsu.edu>
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

/*
 * your kernel needs to be configured with /dev/usb/hiddev support
 * I think most distros are already
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <asm/types.h>

#include "pmd.h"
#include "usb-3100.h"

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

int main(int argc, char **argv) {

  HIDInterface*  hid = 0x0;
  float volts;
  //int flag;
  __u8 channel, range;
  int temp;//, i;
  //int ch;
  __u16 value;
  char serial[9];

  hid_return ret;
  int nInterfaces = 0;
  //__u8 memory[62];

  
  // Debug information.  Delete when not needed    
  // hid_set_debug(HID_DEBUG_ALL);
  // hid_set_debug_stream(stderr);
  // hid_set_usb_debug(2);

  ret = hid_init();
  if (ret != HID_RET_SUCCESS) {
    fprintf(stderr, "hid_init failed with return code %d\n", ret);
    return -1;
  }
 if ((nInterfaces = PMD_Find_Interface(&hid, 0, USB3101_PID)) >= 0) {
    printf("USB 3101 Device is found! Number of Interfaces = %d\n", nInterfaces);
  } else if ((nInterfaces = PMD_Find_Interface(&hid, 0, USB3102_PID)) >= 0) {
    printf("USB 3102 Device is found! Number of Interfaces = %d\n", nInterfaces);
  } else if ((nInterfaces = PMD_Find_Interface(&hid, 0, USB3103_PID)) >= 0) {
    printf("USB 3103 Device is found! Number of Interfaces = %d\n", nInterfaces);
  } else if ((nInterfaces = PMD_Find_Interface(&hid, 0, USB3104_PID)) >= 0) {
    printf("USB 3104 Device is found! Number of Interfaces = %d\n", nInterfaces);
  } else if ((nInterfaces = PMD_Find_Interface(&hid, 0, USB3105_PID)) >= 0) {
    printf("USB 3105 Device is found! Number of Interfaces = %d\n", nInterfaces);
  } else if ((nInterfaces = PMD_Find_Interface(&hid, 0, USB3106_PID)) >= 0) {
    printf("USB 3106 Device is found! Number of Interfaces = %d\n", nInterfaces);
  } else if ((nInterfaces = PMD_Find_Interface(&hid, 0, USB3110_PID)) >= 0) {
    printf("USB 3110 Device is found! Number of Interfaces = %d\n", nInterfaces);
  } else if ((nInterfaces = PMD_Find_Interface(&hid, 0, USB3112_PID)) >= 0) {
    printf("USB 3112 Device is found! Number of Interfaces = %d\n", nInterfaces);
  } else if ((nInterfaces = PMD_Find_Interface(&hid, 0, USB3114_PID)) >= 0) {
    printf("USB 3114 Device is found! Number of Interfaces = %d\n", nInterfaces);
  } else {
    fprintf(stderr, "USB 31XX  not found.\n");
    exit(1);	
  }

  /* config mask 0x01 means all inputs */
  //usbDConfigPort_USB31XX(hid, DIO_DIR_OUT);
  //usbDOut_USB31XX(hid, 0);

  // Configure all analog channels for 0-10V output
  //for (i = 0; i <= 11; i++) {
  //  usbAOutConfig_USB31XX(hid, i, UP_10_00V);
  //}

  strncpy(serial, PMD_GetSerialNumber(hid), 9);
  printf("DAC Serial Number = %s\n", serial);

  // arguments
  bool runCmd = true;
  if ( argc == 1 ) runCmd = false;

  if (runCmd)
    {
      if ( !strcmp(argv[1], "-o") )
        {
	  if ( argc != 3) {
	    printf("Wrong command line option. Only accept -o <voltage>\n");
	    return 1;
	  }
	  volts = atof( argv[2] );
	  printf("Output voltage for channels 0-11 is set to = %f\n", volts );
	}
      else if (!strcmp(argv[1], "-b") )
	{
	  usbBlink_USB31XX(hid, 4);
	  return 0;
	}
      else if ( !strcmp(argv[1], "-r") )
	{
	  usbReset_USB31XX(hid);
	  return 0;
	}
      else if ( !strcmp(argv[1], "-e") )
	{
	  ret = hid_close(hid);
	  if (ret != HID_RET_SUCCESS) {
	    fprintf(stderr, "hid_close failed with return code %d\n", ret);
	    return 1;
	  }
	  hid_delete_HIDInterface(&hid);
	  ret = hid_cleanup();
	  if (ret != HID_RET_SUCCESS) {
	    fprintf(stderr, "hid_cleanup failed with return code %d\n", ret);
	    return 1;
	  }
	  return 0;
	}
      else
	{
	  printf("Wrong option, only accept -o <voltage>, -r, and -e.\n");
	  return 1;
	}
    }
  else
    {

      printf("Enter a voltage for channels 0-11: ");
      scanf("%f", &volts);
    }

  int ichannel = 0;
  for ( ichannel = 0; ichannel <= 11; ichannel++)
    {
      channel = (__u8) ichannel;
      //printf("Enter a range: 0 = 0-10V, 1 = +/- 10V, 2 = 0-20mA ");
      //scanf("%d", &temp);
      temp = 0; // unipolar
      range = (__u8) temp;
      value = volts_USB31XX(hid, range, volts);
      usbAOutConfig_USB31XX(hid, channel, range);
      usbAOut_USB31XX(hid, channel, value, 0);
    }

  /*  if (!runCmd)
    {
      printf("Enter to quit");
      scanf("%d", &temp);
    }
  */
  return 0;
}

