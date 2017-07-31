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
#include <linux/hiddev.h>

#include "pmd.h"
#include "usb-3100.h"

#define FS_DELAY 1000

static USB31XX_CalibrationTable CalTable[NCHAN_31XX];

/* configures digital port */
void usbDConfigPort_USB31XX(HIDInterface* hid, __u8 direction)
{
  /* The command sets the direction of the DIO port bits or output. */

  struct config_port_t {
    __u8 reportID;
    __u8 direction;  // 0 = output, 1 = input
  } config_port;

  config_port.reportID = DCONFIG;
  config_port.direction = direction;

  PMD_SendOutputReport(hid, 0, (__u8*) &config_port, sizeof(config_port), FS_DELAY);
}

/* configures digital bit */
void usbDConfigBit_USB31XX(HIDInterface* hid, __u8 bit_num, __u8 direction)
{
  /* The command sets the direction of an individual DIO bit to input or output */

  struct config_bit_t {
    __u8 reportID;
    __u8 bit_num;      // the bit to configure (0-7)
    __u8 direction;    // 0 = output, 1 = input
  } config_bit;

  config_bit.reportID = DCONFIG_BIT;
  config_bit.bit_num = bit_num;
  config_bit.direction = direction;

  PMD_SendOutputReport(hid, 0, (__u8*) &config_bit, sizeof(config_bit), FS_DELAY);
}

/* reads digital port  */
void usbDIn_USB31XX(HIDInterface* hid, __u8* value)
{
  /* 
     This command reads the current state of the DIO port bits.  The return value will be the 
     value seen at the port pins.
  */

  __u8 reportID = DIN;
  struct read_port_t {
    __u8 reportID;
    __u8 value;
  } read_port;

  PMD_SendOutputReport(hid, 0, &reportID, 1, FS_DELAY);
  usb_interrupt_read(hid->dev_handle, USB_ENDPOINT_IN | 1,
		     (char *) &read_port, sizeof(read_port), FS_DELAY);
  *value = read_port.value;
  return;
}

/* reads digital bit  */
void usbDBitIn_USB31XX(HIDInterface* hid, __u8 bit_num, __u8* value)
{
  /* 
     This command reads an individual digital port bit.  It will return the value seen
     at the port pin, so may be used for an input or output bit.
  */

  struct read_bit_t {
    __u8 reportID;
    __u8 value;     // contains bit_number (0-7) on send and value on receive.
  } read_bit;

  read_bit.reportID = DBIT_IN;
  read_bit.value = bit_num;

  PMD_SendOutputReport(hid, 0, (__u8*) &read_bit, 2, FS_DELAY);
  usb_interrupt_read(hid->dev_handle, USB_ENDPOINT_IN | 1,
		     (char *) &read_bit, sizeof(read_bit), FS_DELAY);
  *value = read_bit.value;
  return;
}

/* writes digital port */
void usbDOut_USB31XX(HIDInterface* hid, __u8 value)
{
  /* This command writes data to the DIO port bits that are configured as outputs. */

  struct write_port_t {
    __u8 reportID;
    __u8 value;    // value to write to the port.
  } write_port;

  write_port.reportID = DOUT;
  write_port.value = value;

  PMD_SendOutputReport(hid, 0, (__u8*) &write_port, sizeof(write_port), FS_DELAY);
}

/* writes digital bit  */
void usbDBitOut_USB31XX(HIDInterface* hid, __u8 bit_num, __u8 value)
{
  /* 
     This command reads an individual digital port bit.  It will return the value seen at the port pin, 
     so may be used for an input or output bit.
  */

  struct write_bit_t {
    __u8 reportID;
    __u8 bit_num;   // the bit to write (0-7)
    __u8 value;     // the value to write to the bit (0 or 1)
  } write_bit;

  write_bit.reportID = DBIT_OUT;
  write_bit.bit_num = bit_num;
  write_bit.value = value;

  PMD_SendOutputReport(hid, 0, (__u8*) &write_bit, sizeof(write_bit), FS_DELAY);
  return;
}

/* Analog Out */
void usbAOutConfig_USB31XX(HIDInterface* hid, __u8 channel, __u8 range)
{

  /* 
     This command configures the output range of an analog output channel.  The output 
     will be set to 0V in the selected range, so an AOut is not needed after this command.
  */

  struct aOutConfig_t {
    __u8 reportID;
    __u8 channel;    // the channel to write (0-15)
    __u8 range;      // 0 = 0-10V, 1 = +/- 10V  2 = 0-20 mA
  } aOutConfig;

  __u16 address;

  if (channel > 15) channel = 15;

  aOutConfig.reportID = AOUT_CONFIG;
  aOutConfig.channel = channel;                          // 0 to 15
  aOutConfig.range = range;                              // 0 = 0-10V, 1 = +/- 10V

  switch (range) {
    case UP_10_00V:
      address = 0x100 + 0x10*channel;
      break;
    case BP_10_00V:
      address = 0x108 + 0x10*channel;
      break;
    case I_0_20_mA:
      address = 0x200 + 0x8*channel;
      aOutConfig.range = 0;          // don't care when doing current output
      break;
    default:
      address = 0x100 + 0x10*channel;
  }
  
  usbReadMemory_USB31XX(hid, address, sizeof(float), (__u8*) &CalTable[channel].slope);
  usbReadMemory_USB31XX(hid, address+0x4, sizeof(float), (__u8*) &CalTable[channel].offset);
  PMD_SendOutputReport(hid, 0, (__u8*) &aOutConfig, sizeof(aOutConfig), FS_DELAY);
 //  printf("Channel = %d    Slope = %f    Offset = %f\n", channel, CalTable[channel].slope, CalTable[channel].offset);
}
 
/* writes to analog out */
void usbAOut_USB31XX(HIDInterface* hid, __u8 channel, __u16 value, __u8 update)
{
  /* 
     This command writes the value to an analog output channel.  The value is a 16-bit
     unsigned value.  The output range for a channel may be set with AOutConfig.

     The equation for the output voltage is:

                (k - 2^15)
        V_out = ---------  * 10V   for +/- 10V range 
                   2^15

                   (k)
        V_out = ---------  * 10V   for 0-10V range 
                  2^16

        The equation for the current output is:


                (k)      5V
        I_out = ----  * ----       for a 0-20mA range.  
                2^16    249 Ohm

	The current output is independent of the range selection.  k is the value written to the device.  A
        current and voltage are always output for a given value.
  */

  double dvalue;
  struct aout_t {
    __u8 reportID;
    __u8 channel;    // the channel to write (0-15)
    __u8 value[2];   // the value to write (0 - ffff)
    __u8 update;     // 0 = update immediately, 1 = update on sync signal
  } aout;

  if (update != 1) update = 0;
  if (channel > NCHAN_31XX) channel = NCHAN_31XX;
  
  dvalue = CalTable[channel].slope*value + CalTable[channel].offset;
  if (dvalue > 0xffff) {
    value = 0xffff;
  } else if (dvalue < 0.0) {
    value = 0x0;
  } else {
    value = (__u16) dvalue;
  }

  aout.reportID = AOUT;
  aout.channel = channel;                          // 0 - 15
  aout.value[0] = (__u8) (value & 0xff);           // low byte
  aout.value[1] = (__u8) ((value >> 0x8) & 0xff);  // high byte
  aout.update = update;

  PMD_SendOutputReport(hid, 0, (__u8*) &aout, sizeof(aout), FS_DELAY);
}

void usbAOutSync_USB31XX(HIDInterface* hid)
{
  __u8 reportID = AOUT_SYNC;

  /*
    This command sends the output update signal to all D/A converters.  If the sync signal is
    configured as an input with SetSync, this command has no effect; if the sync signal is
    configured as an output, a pulse will be generated on the suync pin.
  */

  PMD_SendOutputReport(hid, 0, &reportID, sizeof(reportID), FS_DELAY);
}


/* Initialize the counter */
void usbInitCounter_USB31XX(HIDInterface* hid)
{
  /* The command initializes the event counter and resets the count to zero. */

    __u8 reportID = CINIT;
  PMD_SendOutputReport(hid, 0, (__u8*) &reportID, sizeof(reportID), FS_DELAY);
}

__u32 usbReadCounter_USB31XX(HIDInterface* hid)
{
  /* 
     This function reads the 32-bit event counter on the device.  This counter tallies the transitions
     of an external input attached to the CTRpin on the screw terminal of the device.
  */

   __u32 value;
  struct counter_t {
    __u8 reportID;
    __u8 value[4];   // the value of the counter
  } counter;

  counter.reportID = CIN;

  PMD_SendOutputReport(hid, 0, (__u8*) &counter, 1, FS_DELAY);
  usb_interrupt_read(hid->dev_handle, USB_ENDPOINT_IN | 1,
		     (char *) &counter, sizeof(counter), FS_DELAY);
  value =   counter.value[0] | (counter.value[1] << 8) |
    (counter.value[2] << 16) | (counter.value[3] << 24);

  return value;
}

/* blinks the LED of USB device */
void usbBlink_USB31XX(HIDInterface* hid, __u8 count)
{
  /* The command causes the LED to blink */

  __u8 cmd[2];
  
  cmd[0] = BLINK_LED;
  cmd[1] = count;      // the number of times to blink the LED.

  PMD_SendOutputReport(hid, 0, cmd, sizeof(cmd), FS_DELAY);
}

int usbReset_USB31XX(HIDInterface* hid)
{
  /* 
     This function causes the device to perform a reset.  The device 
     disconnects from the USB bus resets its microcontroller.
  */

  __u8 reportID = RESET;
  
  return PMD_SendOutputReport(hid, 0, &reportID, sizeof(reportID), FS_DELAY);
}

void usbSetSync_USB31XX(HIDInterface* hid, __u8 type)
{
  /*
    This command configures the sync signal.  The sync signal may be
    used to synchronize the analog output updates.  When multiple devices
    are to be used, one device is selected as the master and the rest as
    slaves.  The sync signal of all devices must be wired together.  The
    master will output a pulse when the AOutSync command is issued, and
    all of the devices will update their outputs simultaneously.  This may
    also be used to update one or more devices from an external TTL/CMOS
    signal.  The sync signal is rising edge triggered.
  */

  __u8 cmd[2];

  cmd[0] = SET_SYNC;
  cmd[1] = type;
  
  PMD_SendOutputReport(hid, 0, cmd, sizeof(cmd), FS_DELAY);
}

__u16 usbGetStatus_USB31XX(HIDInterface* hid)
{
  /* 
    This command retrives the status of the device. 
       Bit 0:    0 = Sync slave, 1 = Sync master
       Bit 1-7:  Unused.
  */
  __u16 status;
    
  struct statusReport_t {
  __u8 reportID;
  __u8 status[2];
  } statusReport;

  statusReport.reportID = GET_STATUS;

  PMD_SendOutputReport(hid, 0, &statusReport.reportID, 1, FS_DELAY);
  do {
    statusReport.reportID = 0;
    usb_interrupt_read(hid->dev_handle, USB_ENDPOINT_IN | 1,
		       (char *) &statusReport, sizeof(statusReport), FS_DELAY);
  } while (statusReport.reportID != GET_STATUS);
  status = (__u16) (statusReport.status[0] | (statusReport.status[1] << 8));

  return status;
}

void usbReadMemory_USB31XX(HIDInterface* hid, __u16 address, __u8 count, __u8* memory)
{
  /* 
     This command reads data from the configuration memory (EEPROM or FLASH).  
     All of the memory may be read:
        Addresses 0x000  - 0x0FF address  of EEPROM
        Addresses 0x0100 - 0x02FF address of FLASH
  */

  struct arg_t {
    __u8 reportID;
    __u8 address[2]; // the start address for the read.
    __u8 type;       // not used
    __u8 count;      // number of bytes to be read.  62 max.
  } arg;

  struct memRead_t {
    __u8 reportID;
    __u8 memory[62];
  } memRead;

  if ( count > 62 ) count = 62;
  arg.reportID = MEM_READ;
  arg.address[0] = address & 0xff;         // low byte
  arg.address[1] = (address >> 8) & 0xff;  // high byte
  arg.type = 0x0;
  arg.count = count;

  PMD_SendOutputReport(hid, 0, (__u8 *) &arg, sizeof(arg), FS_DELAY);
  if (usb_interrupt_read(hid->dev_handle, USB_ENDPOINT_IN | 1, (char *) &memRead, 63, FS_DELAY) < 0) {
    perror("usbReadMemory_USB31XX error on read.");
  }

  memcpy(memory, memRead.memory, count);
}

int usbWriteMemory_USB31XX(HIDInterface* hid, __u16 address, __u8 count, __u8 data[])
{
    /*
    This command writes to the non-volatile memory on the device.  The
    non-volatile memory is used to store calibration coefficients,
    system information, and user data.  There are 256 bytes of EEPROM
    (address 0x0000 - 0x00FF) available for general use and 512 bytes
    of FLASH (address 0x0100 - 0x2FF) used for calibration.

    The EEPROM may be written at any address 1 - 59 bytes of data in a
    write.

    The FLASH memory has stricter requirements for writing.  The
    memory is written in blocks of 32 bytes and erased in sectors of
    64 bytes.  The FLASH memory must be erased before being written.
    To ensure this, when writing to FLASH, the firmware will AND the
    start address with 0xFFE0.  If the bit 0x0020 is clear, this write
    is the first 32 bytes of a sector and the sector will be erased.
    Once erased, the first block will be written.  Do not have a
    starting address to the FLASH that does not begin on a 32-byte
    boundary.  The count should be 32 bytes when writing to FLASH, and
    any more than that will be truncated.

    The FLASH memory is reserved for calibration data and requires the unlock
    sequence prior to writing.

    Unlock sequence: Write 2 bytes with the value 0xaa55 to address
    0x500 to unlock.  The unlock status can be checked with GetStatus.
    The unlock will remain in effect until the device is powered off
    or reset.
  */

  struct mem_write_report_t {
    __u8 reportID;
    __u8 address[2];
    __u8 count;
    __u8 data[count];
  } arg;

  if (address <= 0x00ff) {  // EEPROM
    if (count > 59) {
      printf("Maximum of 59 bytes allow when writing to EEPROM\n");
      return -1;
    }
  } else if (address >= 0x0100 && address <= 0x02ff) {  // FLASH
    if (count != 32) {
      printf("Count must equal 32 bytes.\n");
      return -1;
    }
    if (address & ~0x1f) {
      printf("Address must be on a 32 bit boundary.\n");
      return -1;
    }
  } else {
    printf("Invalid address %#x\n", address);
    return -1;
  }
  
  arg.reportID = MEM_WRITE;
  arg.address[0] = address & 0xff;         // low byte
  arg.address[1] = (address >> 8) & 0xff;  // high byte

  arg.count = count;
  memcpy(arg.data, data, count);
  PMD_SendOutputReport(hid, 0, (__u8 *) &arg, sizeof(arg), FS_DELAY);
  return 0;
}

void usbPrepareDownload_USB31XX(HIDInterface* hid)
{
  /*
    This command puts the device into code update mode.  The unlock code must be correct as a
    further safety device.  Call this once before sending code with usbWriteCode.  If not in
    code update mode, any usbWriteCode will be ignored.  A usbReset command must be issued at
    the end of the code download in order to return the device to operation with the new code.
  */

   struct prepare_download_report_t {
    __u8 reportID;
    __u8 unlock_code;
  } prepare_download_report;

  prepare_download_report.reportID = PREPARE_DOWNLOAD;
  prepare_download_report.unlock_code = 0xad;

  PMD_SendOutputReport(hid, 0, (__u8 *) &prepare_download_report,
		       sizeof(prepare_download_report), FS_DELAY);
}

int usbWriteCode_USB31XX(HIDInterface* hid, __u32 address, __u8 count, __u8 data[])
{
  /*
    This command writes to the program memory in the device.  This command is not accepted
    unlesss the device is in update mode.  The command will normally be used when downloading a
    new hex file, so it supports the memory ranges tht may be found in the hex file.

    The address ranges are:

    0x000000 - 0x007AFF:  FLASH program memory
    0x200000 - 0x200007:  ID memory (serial number is stored here)
    0x300000 - 0x30000F:  CONFIG memory (processor configuration data)
    0xF00000 - 0xF03FFF:  EEPROM memory

    FLASH program memory: The device musht receive data in 64-byte segments that begin on
    a 64 byte boundary.  The data is sent in messages containing 32 bytes.  count must
    always equal 32.

    Other memory: Any number of bytes up to the maximum may be sent.
  */

  struct arg_t {
    __u8 reportID;
    __u8 address[3];
    __u8 count;        // 32 bytes max
    __u8 data[32];     // the program data, 32 bytes 
  } arg;

  if (count > 32) count = 32;
    if (address <= 0x007aff) {  // FLASH
    count = 32;
    if (address & ~0x1f) {
      printf("Address must be on a 32 bit boundary.\n");
      return -1;
    }
  }

  arg.reportID = WRITE_CODE;
  memcpy(&arg.address[0], &address, 3);   // 24 bit address
  arg.count = count;
  memcpy(&arg.data[0], data, count);      // 24 bit address
  PMD_SendOutputReport(hid, 0, (__u8 *) &arg, count+5, FS_DELAY);
  return count;
}

void usbWriteSerial_USB31XX(HIDInterface* hid, __u8 serial[8])
{
  /* 
   This command sends a new serial number to the device.  The serial number consists of
   8 bytes, typically ASCII numeric or hexadecimal digits (i.e. "00000001").

   Note: The new serial number will be programmed but not used until hardware reset.
  */

  struct write_serial_t {
    __u8 reportID;
    __u8 serial[8];
  } write_serial;

  write_serial.reportID = WRITE_SERIAL;
  memcpy(write_serial.serial, serial, 8);
  
  PMD_SendOutputReport(hid, 0, (__u8 *) &write_serial, sizeof(write_serial), FS_DELAY);
}


__u16 volts_USB31XX(HIDInterface* hid, __u8 range, float value)
{

  /* function to convert volts (or current) to integer value */

  __u16 k;

   switch (range) {
    case UP_10_00V:
      if (value >= 10.) {
	k = 0xffff;
      }	else if (value <= 0.0) {
	k = 0;
      } else {
	k = value*65536./10.;
      }
      break;
    case BP_10_00V:
      if (value >= 10.) {
	k = 0xffff;
      }	else if (value <= -10.) {
	k = 0;
      } else {
	k = value*32768./10. + 32768 ;
      }
      break;
    case I_0_20_mA:
      if (value >= 0.02) {  // 20 mA
	k = 0xffff;
      }	else if (value <= 0.0) {
	k = 0;
      } else {
	k = value*249*65536./5.;
      }
      break;
    default:
      k = 0;
   }
   return k;
}

