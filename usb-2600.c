/*
 *
 *  Copyright (c) 2013  Warren Jasper <wjasper@tx.ncsu.edu>
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
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <asm/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <string.h>

#include "pmd.h"
#include "usb-2600.h"
#include "usb-26xx.rbf"

#define HS_DELAY 2000

static int wMaxPacketSize;  // will be the same for all devices of this type so
                            // no need to be reentrant. 

void usbBuildGainTable_USB2600(usb_dev_handle *udev, float table[NGAINS_2600][2])
{
  /* 
     Builds a lookup table of calibration coefficents to translate values into voltages:
       voltage = value*table[gain#][0] + table[gain#][1]
     only needed for fast lookup.
  */

  int i, j;
  __u16 address = 0x7500;

  usbMemAddressW_USB2600(udev, address);  // Beginning of Calibration Table
  for (i = 0; i < NGAINS_2600; i++) {
    for (j = 0; j < 2; j++) {
      usbMemoryR_USB2600(udev, (__u8 *) &table[i][j], sizeof(float));
    }
  }
  return;
}

void usbBuildGainTable_USB26X7(usb_dev_handle *udev, float table_AO[NCHAN_AO_26X7][2])
{
  /*
    Builds a lookup table of calibration coefficents to translate values into voltages:
    corrected value = value*table[VDAC#][0] + table[VDAC][1]
  */

  int j, k;
  __u16 address = 0x7580;

  usbMemAddressW_USB2600(udev, address);
  
  for (j = 0; j < NCHAN_AO_26X7; j++) {
    for (k = 0; k < 2; k++) {
      usbMemoryR_USB2600(udev, (__u8 *) &table_AO[j][k], sizeof(float));
    }
  }
  return;
}

void usbInit_2600(usb_dev_handle *udev)
{
  int i;
  /* This function does the following:
     1. Configure the FPGA
     2. Finds the maxPacketSize for bulk transfers
  */

  wMaxPacketSize = usb_get_max_packet_size(udev, 0);

  if (!(usbStatus_USB2600(udev) & FPGA_CONFIGURED)) {
    usbFPGAConfig_USB2600(udev);
    if (usbStatus_USB2600(udev) & FPGA_CONFIG_MODE) {
      for (i = 0; i <= (sizeof(FPGA_data) - 64); i += 64) {
	usbFPGAData_USB2600(udev, &FPGA_data[i], 64);
      }
      if (sizeof(FPGA_data) % 64) {
	usbFPGAData_USB2600(udev, &FPGA_data[i], sizeof(FPGA_data)%64);
      }
      if (!(usbStatus_USB2600(udev) & FPGA_CONFIGURED)) {
	printf("Error: FPGA for the USB-26xx is not configured.  status = %#x\n", usbStatus_USB2600(udev));
	return;
      }
    } else {
      printf("Error: could not put USB-26xx into FPGA Config Mode.  status = %#x\n", usbStatus_USB2600(udev));
      return;
    }
  } else {
    printf("USB-26xx FPGA configured.\n");
    return;
  }
}

/***********************************************
 *            Digital I/O                      *
 ***********************************************/

/* Read/Write digital port tristate register */

__u16 usbDTristateR_USB2600(usb_dev_handle *udev, __u8 port)
{
  /* This command reads or writes the digital port tristate register.
   The tristate register determines if the latch register value is driven onto
   the port pin.  A '1' in the tristate register makes the corresponding
   pin an input, a '0' makes it an output.
  */

  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);
  __u16 data = 0x0;

  if (usb_control_msg(udev, requesttype, DTRISTATE, 0x0, port, (char*) &data, sizeof(data), HS_DELAY) < 0) {
    perror("usbDTristateR_USB2600: error in usb_control_msg()");
  }
  return data;
}

void usbDTristateW_USB2600(usb_dev_handle *udev, __u8 port, __u16 value)
{

  /* This command reads or writes the digital port tristate register.
   The tristate register determines if the latch register value is driven onto
   the port pin.  A '1' in the tristate register makes the corresponding
   pin an input, a '0' makes it an output.
  */

  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);

  if (usb_control_msg(udev, requesttype, DTRISTATE, value, port, NULL, 0x0, HS_DELAY) < 0) {
    perror("usbDTristateW_USB2600: error in usb_control_msg().");
  }
  return;
}

/* reads digital word  */
__u16 usbDPort_USB2600(usb_dev_handle *udev, __u8 port)
{
  /*
    This command reads the current state of the digital pins.
  */

  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);
  __u16 data;

  if (usb_control_msg(udev, requesttype, DPORT, 0x0, port, (char*) &data, sizeof(data), HS_DELAY) < 0) {
    perror("usbDPort_USB2600: error in usb_control_msg().");
  }
  return data;
}

/* read/writes digital port latch */
__u16 usbDLatchR_USB2600(usb_dev_handle *udev, __u8 port)
{
  /*
    This command reads the digital port latch register
  */
  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);
  __u16 data = 0x0;

  if (usb_control_msg(udev, requesttype, DLATCH, 0x0, port, (char *) &data, sizeof(data), HS_DELAY) < 0) {
    perror("usbDLatchR_USB2600: error in usb_control_msg().\n");
  }
  return data;
}

void usbDLatchW_USB2600(usb_dev_handle *udev, __u8 port, __u16 value)
{
  /*
    This command writes the digital port latch register
  */
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);

  if (usb_control_msg(udev, requesttype, DLATCH, value, port, NULL, 0x0, HS_DELAY) < 0) {
    perror("usbDLatchW_USB2600: error in usb_control_msg()");
  }
  return;
}

/***********************************************
 *            Analog Input                     *
 ***********************************************/

__u16 usbAIn_USB2600(usb_dev_handle *udev, __u16 channel)
{
  /*
    This command returns the  value from an analog input channel.  This
    command will result in a bus stall if AInScan is currently running.
  */
  __u16 value;
  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);

  usb_control_msg(udev, requesttype, AIN, channel, 0x0, (char *) &value, sizeof(value), HS_DELAY);
  return value;
}

void usbAInScanStart_USB2600(usb_dev_handle *udev, __u32 count, __u32 retrig_count, double frequency,
			        __u8 packet_size, __u8 options)
{
  /* This command starts the analog input channel scan.  The gain
     ranges that are currently set on the desired channels will be
     used (these may be changed with AInConfig) This command will
     result in a bus stall if an AInScan is currently running.

     Notes:

     The pacer rate is set by an internal 32-bit incrementing timer
     running at a base rate of 64 MHz.  The timer is controlled by
     pacer_period. If burst mode is specified, then this value is the
     period of the scan and the A/D is clocked at this maximum rate
     (1 MHz) for each channel in the scan.  If burst mode is not
     specified, then this value is the period of the A/D readings.  A
     pulse will be output at the AI_CLK_OUT pin at every pacer_period
     interval regardless of the mode.

     If pacer_period is set to 0, the device does not generate an A/D
     clock.  It uses the AI_CLK_IN pin as the pacer source.  Burst
     mode operates in the same fashion: if specified, the scan starts
     on every rising edge of AI_CLK_IN and the A/D is clocked at 1 MHz
     for the number of channels in the scan; if not specified, the A/D
     is clocked on every rising edge of AI_CLK_IN.

     The timer will be reset and sample acquired when its value equals
     timer_period.  The equation for calculating timer_period is:

     timer_period = [64MHz / (sample frequency)] - 1

     The data will be returned in packets utilizing a bulk IN endpoint.
     The data will be in the format:

     lowchannel sample 0: lowchannel + 1 sample 0: ... :hichannel sample 0
     lowchannel sample 1: lowchannel + 1 sample 1: ... :hichannel sample 1
     ...
     lowchannel sample n: lowchannel + 1 sample n: ... :hichannel sample n

     The scan will not begin until the AInScanStart command is sent (and
     any trigger conditiions are met.)  Data will be sent until reaching
     the specified count or an AInScanStop command is sent.

     The packet_size parameter is used for low sampling rates to avoid
     delays in receiving the sampled data. The buffer will be sent,
     rather than waiting for the buffer to fill.  This mode should
     not be used for high sample rates in order to avoid data loss.

     The external trigger may be used to start data collection
     synchronously.  If the bit is set, the device will wait until the
     appropriate trigger edge is detected, then begin sampling data at
     the specified rate.  No messages will be sent until the trigger
     is detected.

     The retrigger mode option and the retrig_count parameter are only
     used if trigger is used.  This option will cause the trigger to
     be rearmed after retrig_count samples are acquired, with a total
     of count samples being returned from the entire scan.
     
  */

  struct t_AInScan {
    __u32 count;        // The total number of scans to perform (0 for continuous scan)
    __u32 retrig_count; // The numer of scans to perform for each trigger in retrigger mode.
    __u32 pacer_period; // The pacer timer period value. (0 for AI_CLK_IN).
    __u8 packet_size;   // Number of samples - 1 to transfer at a time. 
    __u8 options;       /* bit 0:  1 = burst mode
                           bit 1:  Reserved
	                   bit 2:  Reserved
                           bit 3:  1 = use trigger
                           bit 4:  Reserved
			   bit 5:  Reserved
			   bit 6:  1 = retrigger mode, 0 = normal trigger
			   bit 7:  Reserved
		        */
    __u8 pad[2];
  } AInScan;
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);
  __u8 status;

  AInScan.count = count;
  AInScan.retrig_count = retrig_count;
  AInScan.pacer_period = rint((64.E6 / frequency) - 1);
  if (packet_size > wMaxPacketSize/2 - 1) packet_size = wMaxPacketSize/2 - 1;
  AInScan.packet_size = packet_size;
  AInScan.options = options;

  status = usbStatus_USB2600(udev);
  if (!(status & AIN_SCAN_DONE)) {
    printf("Analog In scan not done.\n");
    usbAInScanStop_USB2600(udev);
  }

  /* Pack the data into 14 bytes */
  usb_control_msg(udev, requesttype, AIN_SCAN_START, 0x0, 0x0, (char *) &AInScan, 14, HS_DELAY);
}

int usbAInScanRead_USB2600(usb_dev_handle *udev, int nScan, int nChan, __u16 *data)
{
  char value[MAX_PACKET_SIZE_HS];
  int ret = -1;
  int nbytes = nChan*nScan*2;   // nuber of bytes to read;
  __u8 status;

  ret = usb_bulk_read(udev, USB_ENDPOINT_IN|6, (char *) data, nbytes, HS_DELAY);
  if (ret != nbytes) {
    perror("usbAInScanRead_USB2600: error in usb_bulk_read.");
  }

  status = usbStatus_USB2600(udev);
  // if nbytes is a multiple of wMaxPacketSize the device will send a zero byte packet.
  if ((nbytes%wMaxPacketSize) == 0 && !(status & AIN_SCAN_RUNNING)) {
    usb_bulk_read(udev, USB_ENDPOINT_IN|6, value, 2, 100);
  }

  if ((status & AIN_SCAN_OVERRUN)) {
    printf("Analog In scan overrun.\n");
  }
  return ret;
}

void usbAInConfig_USB2600(usb_dev_handle *udev, ScanList scanList[NCHAN_2600])
{
  /*
    This command reads or writes the analog input channel
    configurations.  This command will result in a bus stall if an
    AInScan is currently running.

    mode:   SINGLE_ENDED  (Single-Ended)
            CALIBRATION   (Calibration mode)

    range:  0: +/- 10V range
            1: +/- 5V range
            2: +/- 2V range
	    3: _/- 1V range
  */

  int i;
  static __u8 Scan_list[64];
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);

  for (i = 0; i < NCHAN_2600; i++) {
    if (scanList[i].channel >= 0 && scanList[i].channel < NCHAN_2600) {
      Scan_list[i] = ((scanList[i].range << 5) | (scanList[i].channel & 0x1f));
    } else {
      printf("Error in Scan List[%d]  mode = %#x   channel = %d  range = %#x\n",
	     i, scanList[i].mode, scanList[i].channel, scanList[i].range);
      return;
    }
    
    if (scanList[i].mode & LAST_CHANNEL) {
      break;
    }
  }

  if (usbStatus_USB2600(udev) | AIN_SCAN_RUNNING) {
    usbAInScanStop_USB2600(udev);
  }
  usb_control_msg(udev, requesttype, AIN_CONFIG, 0x0, i, (char *) &Scan_list[0], sizeof(Scan_list), HS_DELAY);
}

void usbAInConfigR_USB2600(usb_dev_handle *udev, ScanList scanList[NCHAN_2600])
{
  static __u8 Scan_list[15];
  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);

  if (usbStatus_USB2600(udev) | AIN_SCAN_RUNNING) {
    usbAInScanStop_USB2600(udev);
  }
  
  usb_control_msg(udev, requesttype, AIN_CONFIG, 0x0, 0x0, (char *) Scan_list, sizeof(Scan_list), HS_DELAY);
}

void usbAInScanStop_USB2600(usb_dev_handle *udev)
{
  /*
    This command stops the analog input scan (if running).
  */
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);
  usb_control_msg(udev, requesttype, AIN_SCAN_STOP, 0x0, 0x0, NULL, 0x0, HS_DELAY);
}

void usbAInScanClearFIFO_USB2600(usb_dev_handle *udev)
{
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);
  usb_control_msg(udev, requesttype, AIN_CLR_FIFO, 0x0, 0x0, NULL, 0x0, HS_DELAY);
}

/***********************************************
 *            Analog Output                    *
 ***********************************************/
void usbAOut_USB26X7(usb_dev_handle *udev, __u8 channel, double voltage, float table_AO[NCHAN_AO_26X7][2])
{
  /*
    This command reads or writes the values for the analog output channels.
    The values are 16-bit unsigned numbers.  Both read and write will result
    in a control pipe stall if an output scan is running.  The equation for the
    output voltage is:

             ( value - 2^15 )
    V_out = -----------------  * V_ref
                 2^15

     were value is the value written to the channel and V_ref = 10V.
  */

  double dvalue;
  __u16 value;
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);

  if (channel > 3) {
    printf("usbAOut_USB26X7: channel must be between 0 and 3.\n");
    return;
  }

  /* correct voltage */
  dvalue = (voltage/10.*32768. + 32768.);
  dvalue = dvalue*table_AO[channel][0] + table_AO[channel][1];

  if (dvalue > 0xffff) {
    value = 0xffff;
  } else if (dvalue < 0.0) {
    value = 0x0;
  } else {
    value = rint(dvalue);
  }
  if (usb_control_msg(udev, requesttype, AOUT, value, channel, NULL, 0x0, HS_DELAY) < 0) {
    perror("usbAOut_USB26X7(): error in usb_control_msg.");
  }
}

void usbAOutR_USB26X7(usb_dev_handle *udev, __u8 channel, double *voltage, float table_AO[NCHAN_AO_26X7][2])
{
  __u16 value[4];
  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);
  
  usb_control_msg(udev, requesttype, AOUT, 0x0, 0x0, (char *) value, sizeof(value), HS_DELAY);
  *voltage = ((double)(value[channel] - table_AO[channel][1])) / (double) table_AO[channel][0];
  *voltage = (*voltage - 32768.)*10./32768.;
}

void usbAOutScanStop_USB26X7(usb_dev_handle *udev)
{
  /* This command stops the analog output scan (if running). */

  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);
  usb_control_msg(udev, requesttype, AOUT_SCAN_STOP, 0x0, 0x0, NULL, 0x0, HS_DELAY);
}

void usbAOutScanClearFIFO_USB26X7(usb_dev_handle *udev)
{
  /* This command clears any remaining output FIFO data after a scan */
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);
  usb_control_msg(udev, requesttype, AOUT_CLEAR_FIFO, 0x0, 0x0, NULL, 0x0, HS_DELAY);
}

void usbAOutScanStart_USB26X7(usb_dev_handle *udev, __u32 count, __u32 retrig_count, double frequency, __u8 options)
{
  /*
    This command starts the analog output channel scan.  This command
    will result in a bus stall if an AOutScan is currently running.

    count:        the total number of scans to perform (0 = continuous mode)
    retrig_count: the number of scans to perform for each trigger in
                  retrigger mode
    frequency:    pacer frequency (0 for AO_CLK_IN)
    options:      bit 0: 1 = include channel 0 in output scan
                  bit 1: 1 = include channel 1 in output scan
                  bit 2: 1 = include channel 2 in output scan
                  bit 3: 1 = include channel 3 in output scan
                  bit 4: 1 = use trigger
                  bit 5: 1 = retirgger mode, 0 = normal trigger
		  bit 6: reserved
		  bit 7: reserved
		  
    The output scan operates with the host continuously transferring
    data for the outputs until the end of the scan.  If the "count"
    parameter is 0, the scan will run until the AOutScanStop command
    is issued by the host; if it is nonzero, the scan will stop
    automatically after the specified number of scans have been
    output.  The channels in the scan are selected in the options bit
    field.  Scan refer to the number of updates to the channels (if
    both channels are used, one scan is an update to both channels).

    The output data is written to an internal FIFO.  The bulk endpoint data is
    only accepted if there is room in the FIFO.  Output data may be sent to the
    FIFO before the start of the scan, and the FIFO is cleared when the
    AOutScanClearFIFO command is received.  The scan will not begin until the
    AOutScanStart command is sent (and output data is in the FIFO).  Data will be
    output until reaching the specified number of scans (in single execution mode)
    or an AOutScanStop command is sent.
  */

  struct t_AOutScan {
    __u32 count;         // The total number of scans to perform.  0 = run forever.
    __u32 retrig_count;  // The number of scans to perform for each trigger in retrigger mode.
    __u32 pacer_period;  // Pacer timer period value (0 for AO_CLK_IN)
    __u8 options;
  } AOutScan;
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);

  AOutScan.pacer_period = (64.E6 / frequency) - 1;
  AOutScan.count = count;
  AOutScan.retrig_count = retrig_count;
  AOutScan.options = options;
  
  if (usb_control_msg(udev, requesttype, AOUT_SCAN_START, 0x0, 0x0, (char *) &AOutScan, sizeof(AOutScan), HS_DELAY) < 0) {
    perror("AOutScanStart(): usb_control_msg error.");
  }
}

/***********************************************
 *            Counter/Timer                    *
 ***********************************************/
void usbCounterInit_USB2600(usb_dev_handle *udev, __u8 counter)
{
  /*
    This command initializes the 32-bit event counters.  On a write, the
    specified counter (0 or 1) will be reset to zero.
  */

  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);

  if (usb_control_msg(udev, requesttype, COUNTER, counter, 0x0, NULL, 0x0, HS_DELAY) < 0) {
    perror("usbCounterInit_USB2600 error.");
  }
  return;
}

__u32 usbCounter_USB2600(usb_dev_handle *udev, __u8 counter)
{
  /*
    This command reads the 32-bit event counter.  
  */

  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);
  __u32 counts[4] = {0x0, 0x0, 0x0, 0x0};

  if (usb_control_msg(udev, requesttype, COUNTER, 0x0, 0x0, (char *) &counts, sizeof(counts), HS_DELAY) < 0) {
    perror ("usbCounter_USB2600 error.");
  }

  return counts[counter];
}

void usbTimerControlR_USB2600(usb_dev_handle *udev, __u8 timer, __u8 *control)
{
  /*
    This command reads/writes the timer control register
    control:   bit 0:    1 = enable timer,  0 = disable timer
               bit 1:    1 = timer running, 0 = timer stopped
                         (read only, useful when using count)
               bit 2:    1 = inverted output (active low)
	                 0 = normal output (active high)
	       bits 3-7: reserved
  */

  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);
  usb_control_msg(udev, requesttype, TIMER_CONTROL, 0x0, timer, (char *) control, sizeof(control), HS_DELAY);
}

void usbTimerControlW_USB2600(usb_dev_handle *udev, __u8 timer, __u8 control)
{
  /* This command reads/writes the timer control register */

  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);
  usb_control_msg(udev, requesttype, TIMER_CONTROL, control, timer, NULL, 0x0, HS_DELAY);
}

void usbTimerPeriodR_USB2600(usb_dev_handle *udev, __u8 timer, __u32 *period)
{
  /*
    The timer is based on a 64 MHz input clock and has a 32-bit period register. The
    frequency of the output is set to:

          frequency = 64 MHz / (period + 1)

    Note that the value for pulseWidth should always be smaller than the value for
    the period register or you may get unexpected results.  This results in a minimum
    allowable value for the period of 1, which sets the maximum frequency to 64 MHz/2.
  */

  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);
  if (usb_control_msg(udev, requesttype, TIMER_PERIOD, 0x0, timer, (char *) period, sizeof(period), HS_DELAY) < 0) {
    perror("usbTimerPeriodR_USB2600 error");
  }
}    

void usbTimerPeriodW_USB2600(usb_dev_handle *udev, __u8 timer, __u32 period)
{
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);

  if (usb_control_msg(udev, requesttype, TIMER_PERIOD, 0x0, timer, (char *) &period, sizeof(period), HS_DELAY) < 0) {
    perror("usbTimerPeriodW_USB2600 error.");
  }
}

void usbTimerPulseWidthR_USB2600(usb_dev_handle *udev, __u8 timer, __u32 *pulseWidth)
{
  /*
    This command reads/writes the timer pulse width register.
    The timer is based on a 64 MHz input clock and has a 32-bit pulse width register.
    The width of the output pulse is set to:

    pulse width = (pulseWidth + 1) / 64 MHz

    Note that the value for pulseWidth should always be smaller than the value for
    the period register or you may get unexpected results.
  */
  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);
  if (usb_control_msg(udev, requesttype, TIMER_PULSE_WIDTH, 0x0, timer, (char *) pulseWidth, sizeof(pulseWidth), HS_DELAY) < 0) {
    perror("usbTimerPulseWidthR_USB2600 error.");
  }
}

void usbTimerPulseWidthW_USB2600(usb_dev_handle *udev, __u8 timer, __u32 pulseWidth)
{
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);
  usb_control_msg(udev, requesttype, TIMER_PULSE_WIDTH, 0x0, timer, (char *) &pulseWidth, sizeof(pulseWidth), HS_DELAY);
}

void usbTimerCountR_USB2600(usb_dev_handle *udev, __u8 timer, __u32 *count)
{
  /*
    This command reads/writes the timer count register.
    The number of output pulses can be controlled with the count register.  Setting
    this register to 0 will result in pulses being generated until the timer is disabled.
    Setting it to a non-zero value will results in the specified number of pulses being
    generated then the output will go low until the timer is disabled.
  */

  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);

  if (usb_control_msg(udev, requesttype, TIMER_COUNT, 0x0, timer, (char *) count, sizeof(count), HS_DELAY) < 0) {
    perror("usbTimerCountR_USB2600 error.");
  }
}

void usbTimerCountW_USB2600(usb_dev_handle *udev, __u8 timer, __u32 count)
{
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);
  usb_control_msg(udev, requesttype, TIMER_COUNT, 0x0, timer, (char *) &count, sizeof(count), HS_DELAY);
}

void usbTimerDelayR_USB2600(usb_dev_handle *udev, __u8 timer, __u32 *delay)
{
  /*
     This command reads/writes the timer start delay register.  This
     register is the amount of time to delay before starting the timer
     output after enabling the output.  The value specifies the number
     of 40 MHZ clock pulses to delay.  This value may not be written
     while the timer output is enabled.
  */
  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);
  usb_control_msg(udev, requesttype, TIMER_START_DELAY, 0x0, timer, (char *) delay, sizeof(delay), HS_DELAY);
}

void usbTimerDelayW_USB2600(usb_dev_handle *udev, __u8 timer, __u32 delay)
{
  /*
    This register is the amount of time to delay before starting the timer output after
    enabling the output.  The value specifies the number of 64MHz clock pulses to
    delay.  This value may not be written while the timer output is enabled.
  */
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);
  usb_control_msg(udev, requesttype, TIMER_START_DELAY, 0x0, timer, (char *) &delay, 0x0, HS_DELAY);
}

void usbTimerParamsR_USB2600(usb_dev_handle *udev, __u8 timer, timerParams *params)
{
  /*
    This command reads/writes all timer parameters in one call.
  */
  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);
  usb_control_msg(udev, requesttype, TIMER_PARAMETERS, 0x0, timer, (char *) params, sizeof(timerParams), HS_DELAY);
}

void usbTimerParamsW_USB2600(usb_dev_handle *udev, __u8 timer, timerParams *params)
{
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);
  usb_control_msg(udev, requesttype, TIMER_PARAMETERS, 0x0, timer, (char *) params, sizeof(timerParams), HS_DELAY);
}

/***********************************************
 *            Memory Commands                  *
 ***********************************************/
void usbMemoryR_USB2600(usb_dev_handle *udev, __u8 *data, __u16 length)
{
  /*
    This command reads or writes data from the EEPROM memory.  The
    read will begin at the current address, which may be set with
    MemAddress.  The address will automatically increment during a
    read or write but stay within the range allowed for the EEPROM.
    The amount of data to be written or read is specified in wLength.

    The range from 0x0000 to 0x3FFF is used for storing the
    microcontroller firmware and is write-protected during normal
    operation.
  */
  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);
  int ret;
  ret = usb_control_msg(udev, requesttype, MEMORY, 0x0, 0x0, (char *) data, length, HS_DELAY);
  if (ret != length) {
    printf("usbMemoryR_USB2600: error in reading memory\n");
  }
}

void usbMemoryW_USB2600(usb_dev_handle *udev, __u8 *data, __u16 length)
{
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);
  usb_control_msg(udev, requesttype, MEMORY, 0x0, 0x0, (char *) data, length, HS_DELAY);
}

void usbMemAddressR_USB2600(usb_dev_handle *udev, __u16 address)
{
  /*
    This command reads or writes the address used for memory accesses.
    The upper byte is used to denominate different memory areas.  The
    memory map for this device is

       Address                            Description
    =============               ============================
    0x0000-0x74FF               Microcontroller firmware (write protected)
    0x7500-0x75AB               Calibration storage      (write protected)
    0X7600-0X78FF               DaqFlex Device Capabilities
    0x7900-0x7939               MBD User ID
    0x7F10-0x74F7               User Data
    0x7FF8-0x7FFF               Serial Number

    The firmware area is protected by a separate command so is not typically
    write-enabled.  The calibration area is unlocked by writing the value 0xAA55
    to address 0x8000.  The area will remain unlocked until the device is reset
    or a value other than 0xAA55 is written to address 0x8000.
  */
  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);
  usb_control_msg(udev, requesttype, MEM_ADDRESS, 0x0, 0x0, (char *) &address, sizeof(address), HS_DELAY);
}

void usbMemAddressW_USB2600(usb_dev_handle *udev, __u16 address)
{
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);
  usb_control_msg(udev, requesttype, MEM_ADDRESS, 0x0, 0x0, (char *) &address, sizeof(address), HS_DELAY);
}

void usbMemWriteEnable_USB2600(usb_dev_handle *udev)
{
  /*
    This command enables writes to the EEPROM memory in the range
    0x0000-0x6FFF.  This command is only to be used when updating the
    microcontroller firmware.
  */

  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);
  __u8 unlock_code = 0xad;
  usb_control_msg(udev, requesttype, MEM_ADDRESS, 0x0, 0x0, (char *) &unlock_code, sizeof(unlock_code), HS_DELAY);
}

/***********************************************
 *          Miscellaneous Commands             *
 ***********************************************/
  
/* blinks the LED of USB device */
void usbBlink_USB2600(usb_dev_handle *udev, __u8 count)
{
  /*
    This command will blink the device LED "count" number of times
  */
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);

  usb_control_msg(udev, requesttype, BLINK_LED, 0x0, 0x0, (char *) &count, sizeof(count), HS_DELAY);
  return;
}

__u16 usbStatus_USB2600(usb_dev_handle *udev)
{
  /*
    This command retrieves the status of the device.
  */

  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);
  __u16 status = 0x0;

  usb_control_msg(udev, requesttype, STATUS, 0x0, 0x0, (char *) &status, sizeof(status), HS_DELAY);
  return status;
}

void usbReset_USB2600(usb_dev_handle *udev)
{
  /*
    This function causes the device to perform a reset.  The device
    disconnects from the USB bus and resets its microcontroller.
  */

  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);

  usb_control_msg(udev, requesttype, RESET, 0x0, 0x0, NULL, 0, HS_DELAY);
  return;
}

void usbTriggerConfig_USB2600(usb_dev_handle *udev, __u8 options)
{
  /*
    This function configures the AInScan trigger.  Once the trigger is
    received, the AInScan will proceed as configured.  The "use
    trigger" option must be used in the AInScanStart command to
    utilize this feature.

    options:     bit 0: trigger mode (0 = level,  1 = edge)
                 bit 1: trigger polarity (0 = low / falling, 1 = high / rising)
                 bits 2-7: reserved
  */

  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);
  usb_control_msg(udev, requesttype, TRIGGER_CONFIG, 0x0, 0x0, (char *) &options, sizeof(options), HS_DELAY);
}

void usbTriggerConfigR_USB2600(usb_dev_handle *udev, __u8 *options)
{
  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);
  usb_control_msg(udev, requesttype, TRIGGER_CONFIG, 0x0, 0x0, (char *) options, sizeof(options), HS_DELAY);
}

void usbTemperature_USB2600(usb_dev_handle *udev, float *temperature)
{
  /*
    This command reads the internal temperature.  The temperature in degrees
    Celsius is calculated as:

     T = 128.(value/2^15)
  */

  __s16 temp;
  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);
  int ret;

  ret =  usb_control_msg(udev, requesttype, TEMPERATURE, 0x0, 0x0, (char *) &temp, sizeof(temp), HS_DELAY);
  if (ret < 0) {
    printf("usbTemperature_USB2600: error in reading temperature.  Error = %d\n", ret);
  }
  *temperature = temp/256.0;
}

void usbGetSerialNumber_USB2600(usb_dev_handle *udev, char serial[9])
{
  /*
    This commands reads the device USB serial number.  The serial
    number consists of 8 bytes, typically ASCII numeric or hexadecimal digits
    (i.e. "00000001"). 
  */
  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);

  usb_control_msg(udev, requesttype, SERIAL, 0x0, 0x0, serial, 8, HS_DELAY);
  serial[8] = '\0';
  return;
}

/***********************************************
 *          FPGA Commands                      *
 ***********************************************/

void usbFPGAConfig_USB2600(usb_dev_handle *udev)
{
  /*
    This command puts the device into FPGA configuration update mode,
    which allows downloading the configuration for the FPGA.  The
    unlock code must be correct as a further safely device.  If the
    device is not in FPGA config mode, then the FPGAData command will
    result in a control pipe stall.

    Use the Status command to determine if the FPGA needs to be
    configured.  If so, use this command to enter configuration mode.
    Open the .rbf file containing the FPGA configuration and stream
    the data to the device using FPGAData.  After the FPGA is
    configured, then the DAQ commands will work.
  */
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);
  __u8 unlock_code = 0xad;
  usb_control_msg(udev, requesttype, FPGA_CONFIG, 0x0, 0x0, (char *) &unlock_code, sizeof(unlock_code), HS_DELAY);
}

void usbFPGAData_USB2600(usb_dev_handle *udev, __u8 *data, __u8 length)
{
  /*
    This command writes the FPGA configuration data to the device.  This
    command is not accepted unless the device is in FPGA config mode.  The
    number of bytes to be written must be specified in wLength.

    data: max length is 64 bytes
  */
  __u8 requesttype = (HOST_TO_DEVICE | VENDOR_TYPE | DEVICE_RECIPIENT);

  if (length > 64) {
    printf("usbFPGAData_USB2600: max length = 64 bytes\n");
    return;
  }
  usb_control_msg(udev, requesttype, FPGA_DATA, 0x0, 0x0, (char *) data, length, HS_DELAY);
}

void usbFPGAVersion_USB2600(usb_dev_handle *udev, __u16 *version)
{
  /*
    This command reads the FPGA version.
  */

  __u8 requesttype = (DEVICE_TO_HOST | VENDOR_TYPE | DEVICE_RECIPIENT);

  usb_control_msg(udev, requesttype, FPGA_VERSION, 0x0, 0x0, (char *) version, sizeof(__u16), HS_DELAY);
}

void cleanup_USB2600( usb_dev_handle *udev )
{
  if (udev) {
    usb_clear_halt(udev, USB_ENDPOINT_IN|1);
    usb_clear_halt(udev, USB_ENDPOINT_OUT|1);
    usb_release_interface(udev, 0);
    usb_close(udev);
  }
}

double volts_USB2600(usb_dev_handle *udev, const __u8 gain, __u16 value)
{

  double volt = 0.0;

  switch (gain) {
    case BP_10V:
      volt = (value - 32768.)*10./32768.;
      break;
    case BP_5V:
      volt = (value - 32768.)*5./32768.;
      break;
    case BP_2V:
      volt = (value - 32768.)*2./32768.;
      break;
    case BP_1V:
      volt = (value - 32768.)*1./32768.; 
      break;
  }
  return volt;
}
