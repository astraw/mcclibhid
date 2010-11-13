/*
 *
 *  Copyright (c) 2004-2005  Warren Jasper <wjasper@tx.ncsu.edu>
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
 * mikee@signalscape.com
 *
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

#include "pmd.h"
#include "usb-1608FS.h"

#define FS_DELAY 2000

/* configures digital port */
void usbDConfigPort_USB1608FS(HIDInterface* hid, __u8 direction)
{
  struct t_config_port {
    __u8 reportID;
    __u8 direction;
  } config_port;

  config_port.reportID = DCONFIG;
  config_port.direction = direction;

  PMD_SendOutputReport(hid, DCONFIG, (__u8*) &config_port, sizeof(config_port), FS_DELAY);
}

/* configures digital bit */
void usbDConfigBit_USB1608FS(HIDInterface* hid, __u8 bit_num, __u8 direction)
{
  struct t_config_bit {
    __u8 reportID;
    __u8 bit_num;      
    __u8 direction;
  } config_bit;

  config_bit.reportID = DCONFIG_BIT;
  config_bit.bit_num = bit_num;
  config_bit.direction = direction;

  PMD_SendOutputReport(hid, DCONFIG_BIT, (__u8*) &config_bit, sizeof(config_bit), FS_DELAY);
}

/* reads digital port  */
void usbDIn_USB1608FS(HIDInterface* hid, __u8* value)
{
  __u8 reportID = DIN;
  struct t_read_port {
    __u8 reportID;
    __u8 value;
  } read_port;

  PMD_SendOutputReport(hid, DIN, &reportID, 1, FS_DELAY);
  usb_interrupt_read(hid->dev_handle, USB_ENDPOINT_IN | 2,
		     (char *) &read_port, sizeof(read_port), FS_DELAY);
  *value = read_port.value;
  return;
}

/* reads digital bit  */
void usbDInBit_USB1608FS(HIDInterface* hid, __u8 bit_num, __u8* value)
{
  struct t_read_bit {
    __u8 reportID;
    __u8 value;  // contains bit_number on send and value on receive.
  } read_bit;

  read_bit.reportID = DBIT_IN;
  read_bit.value = bit_num;

  PMD_SendOutputReport(hid, DBIT_IN, (__u8*) &read_bit, 2, FS_DELAY);
  usb_interrupt_read(hid->dev_handle, USB_ENDPOINT_IN | 2,
		     (char *) &read_bit, sizeof(read_bit), FS_DELAY);
  *value = read_bit.value;
  return;
}

/* writes digital port */
void usbDOut_USB1608FS(HIDInterface* hid, __u8 value)
{
  struct t_write_port {
    __u8 reportID;
    __u8 value;
  } write_port;

  write_port.reportID = DOUT;
  write_port.value = value;

  PMD_SendOutputReport(hid, DOUT, (__u8*) &write_port, sizeof(write_port), FS_DELAY);
}

/* writes digital bit  */
void usbDOutBit_USB1608FS(HIDInterface* hid, __u8 bit_num, __u8 value)
{
  struct t_write_bit {
    __u8 reportID;
    __u8 bit_num;
    __u8 value;
  } write_bit;

  write_bit.reportID = DBIT_OUT;
  write_bit.bit_num = bit_num;
  write_bit.value = value;

  PMD_SendOutputReport(hid, DBIT_OUT, (__u8*) &write_bit, sizeof(write_bit), FS_DELAY);
  return;
}

/* reads from analog in */
signed short usbAIn_USB1608FS(HIDInterface* hid, __u8 channel, __u8 range)
{
  __u16 data;
  __s16 value;
  __u8 report[3];

  struct t_ain {
    __u8 reportID;
    __u8 channel;
    __u8 range;
  } ain;

  ain.reportID = AIN;
  ain.channel = channel;
  ain.range = range;
  if (channel > 7 ) {
    printf("usbAIN: channel out of range for differential mode.\n");
    return -1;
  }

  PMD_SendOutputReport(hid, AIN, (__u8*) &ain, sizeof(ain), FS_DELAY);
  usb_interrupt_read(hid->dev_handle, USB_ENDPOINT_IN | 2,
		     (char *) report, sizeof(report), FS_DELAY);

  data = (__u16) ( report[1] | (report[2] << 8));
  
  if ( data >= 0x8000 ) {
    value = (data - 0x8000);
  } else {
    value = (0x8000 - data);
    value *= (-1);
  }

  return value;
}

void usbAInStop_USB1608FS(HIDInterface* hid)
{
  __u8 reportID = AIN_STOP;;

  PMD_SendOutputReport(hid, AIN_STOP, &reportID, sizeof(reportID), FS_DELAY);
}

int usbAInScan_USB1608FS(HIDInterface* hid[], __u8 lowchannel, __u8 highchannel, __u32 count,
			  float *frequency, __u8 options, __s16 sdata[])
{
  int num_samples;
  int i, j, k;
  __u16 scan_index = 0;
  
  __u32 preload;

  struct t_data{
    __u16 value[31];        // 31 16-bit samples
    __u16 scan_index;       //  1 16 bit scan count
  } data;
    
  struct arg {
    __u8  reportID;
    __u8  lowchannel;
    __u8  highchannel;
    __u8  count[4];
    __u8  prescale;
    __u8  preload[2];
    __u8  options;
  } arg;

  if ( highchannel > 7 ) {
    printf("usbAInScan: highchannel out of range.\n");
    return -1;
  }
  if ( lowchannel > 7 ) {
    printf("usbAInScan: lowchannel out of range.\n");
    return -1;
  }
  if (highchannel >= lowchannel) {
    num_samples = count*(highchannel - lowchannel + 1);
  } else {
    num_samples = count*((8-highchannel) + lowchannel + 1);
  }

  arg.reportID = AIN_SCAN;
  arg.lowchannel = lowchannel;
  arg.highchannel = highchannel;
  arg.count[0] = (__u8) count & 0xff;           // low byte
  arg.count[1] = (__u8) (count >>  8) & 0xff;
  arg.count[2] = (__u8) (count >> 16) & 0xff;
  arg.count[3] = (__u8) (count >> 24) & 0xff;   // high byte
  arg.options = options;                        

  for ( arg.prescale = 0; arg.prescale <= 8; arg.prescale++ ) {
    preload = 10e6/((*frequency) * (1<<arg.prescale));
    if ( preload <= 0xffff ) {
      arg.preload[0] = (__u8) preload & 0xff;          // low byte
      arg.preload[1] = (__u8) (preload >> 8) & 0xff;   // high byte
      break;
    }
  }

  *frequency = 10.e6/(preload*(1<<arg.prescale));

  //printf("AInScan: actual frequency = %f\n", *frequency);

  if ( arg.prescale == 9 || preload == 0) {
    printf("usbAInScan_USB1608FS: frequency out of range.\n");
    return -1;
  }
  count = num_samples;  // store value of samples.
  
  PMD_SendOutputReport(hid[0], AIN_SCAN, (__u8 *) &arg, sizeof(arg), FS_DELAY);
  i = 0;

  while ( num_samples > 0 ) {         
    for ( j = 1; j <= 6; j++ ) {         // cycle through the ADC interfaces/endpoints
      do { 
	usb_interrupt_read(hid[j]->dev_handle, USB_ENDPOINT_IN |(j+2),
			   (char *) &data, sizeof(data), FS_DELAY);
      } while (data.scan_index != scan_index);
      scan_index++;

      if ( num_samples > 31 ) {
	for ( k = 0; k < 31;  k++ ) {
  	  if (data.value[k] >= 0x8000) {
	    sdata[i+k] = data.value[k] - 0x8000;
	  } else {
	    sdata[i+k] = 0x8000 - data.value[k];
	    sdata[i+k] *= (-1);
	  }
	}
        num_samples -= 31;
	i += 31;
      } else {   // only copy in a partial scan
	for ( k = 0; k < num_samples;  k++ ) {
	  if (data.value[k] >= 0x8000) {
	    sdata[i+k] = data.value[k] - 0x8000;
	  } else {
	    sdata[i+k] = 0x8000 - data.value[k];
	    sdata[i+k] *= (-1);
	  }
	}
        num_samples -= 31;
	i += num_samples;
        break;
      }
      printf("Scan %d\tnumber samples left = %d\n", data.scan_index, num_samples);
    }
  }
  return count;
}

void usbAInLoadQueue_USB1608FS(HIDInterface* hid, __u8 gainArray[8])
{
  struct {
    __u8 reportID;
    __u8 gainArray[8];
  } in;

  in.reportID = ALOAD_QUEUE;
  memcpy(in.gainArray, gainArray, 8);
  
  PMD_SendOutputReport(hid, ALOAD_QUEUE, (__u8 *) &in, sizeof(in), FS_DELAY);
}

/* Initialize the counter */
void usbInitCounter_USB1608FS(HIDInterface* hid)
{
  __u8 cmd[1];

  cmd[0] = CINIT;

  PMD_SendOutputReport(hid, CINIT, cmd, sizeof(cmd), FS_DELAY);
}

__u32 usbReadCounter_USB1608FS(HIDInterface* hid)
{
   __u32 value;
  struct t_counter {
    __u8 reportID;
    __u8 value[4];
  } counter;

  counter.reportID = CIN;

  PMD_SendOutputReport(hid, CIN, (__u8*) &counter, 1, FS_DELAY);
  usb_interrupt_read(hid->dev_handle, USB_ENDPOINT_IN | 2,
		     (char *) &counter, sizeof(counter), FS_DELAY);
  value =   counter.value[0] | (counter.value[1] << 8) |
    (counter.value[2] << 16) | (counter.value[3] << 24);

  return value;
}


/* blinks the LED of USB device */
void usbBlink_USB1608FS(HIDInterface* hid)
{
  __u8 reportID = BLINK_LED;

  PMD_SendOutputReport(hid, BLINK_LED, &reportID, sizeof(reportID), FS_DELAY);
}

int usbReset_USB1608FS(HIDInterface* hid)
{
  __u8 reportID = RESET;

  return PMD_SendOutputReport(hid, RESET, &reportID, sizeof(reportID), FS_DELAY);
}

void usbSetTrigger_USB1608FS(HIDInterface* hid, __u8 type)
{
  __u8 cmd[2];
  
  cmd[0] = SET_TRIGGER;
  cmd[1] = type;
  
  PMD_SendOutputReport(hid, SET_TRIGGER, cmd, sizeof(cmd), FS_DELAY);
}

void usbSetSync_USB1608FS(HIDInterface* hid, __u8 type)
{
  __u8 cmd[2];

  cmd[0] = SET_SYNC;
  cmd[1] = type;
  
  PMD_SendOutputReport(hid, SET_SYNC, cmd, sizeof(cmd), FS_DELAY);
}

__u16 usbGetStatus_USB1608FS(HIDInterface* hid)
{
  __u16 status;
    
  struct t_statusReport {
  __u8 reportID;
  __u8 status[2];
  } statusReport;

  statusReport.reportID = GET_STATUS;

  PMD_SendOutputReport(hid, GET_STATUS, &statusReport.reportID, 1, FS_DELAY);
  do {
    statusReport.reportID = 0;
    usb_interrupt_read(hid->dev_handle, USB_ENDPOINT_IN | 2,
		       (char *) &statusReport, sizeof(statusReport), FS_DELAY);
  } while ( statusReport.reportID != GET_STATUS);
  status = (__u16) (statusReport.status[0] | (statusReport.status[1] << 8));

  return status;
}

void usbReadMemory_USB1608FS( HIDInterface* hid, __u16 address, __u8 count, __u8 memory[])
{
  // Addresses 0x000 - 0x07F are reserved for firmware data
  // Addresses 0x080 - 0x3FF are available for use as calibraion or user data

  struct arg {
    __u8 reportID;
    __u8 address[2];
    __u8 type;
    __u8 count;
  } arg;

  if ( count > 62 ) count = 62;
  arg.reportID = MEM_READ;
  arg.address[0] = address & 0xff;         // low byte
  arg.address[1] = (address >> 8) & 0xff;  // high byte
  arg.count = count;

  PMD_SendOutputReport(hid, MEM_READ, (__u8 *) &arg, sizeof(arg), FS_DELAY);
  usb_interrupt_read(hid->dev_handle, USB_ENDPOINT_IN | 2,
		     (char *) &memory, sizeof(memory), FS_DELAY);
}

int usbWriteMemory_USB1608FS(HIDInterface* hid, __u16 address, __u8 count, __u8* data)
{
  // Locations 0x00-0x7F are reserved for firmware and my not be written.
  int i;
  struct t_mem_write_report {
    __u8 reportID;
    __u8 address[2];
    __u8 count;
    __u8 data[count];
  } arg;

  if ( address <=0x7f ) return -1;
  if ( count > 59 ) count = 59;

  arg.reportID = MEM_WRITE;
  arg.address[0] = address & 0xff;         // low byte
  arg.address[1] = (address >> 8) & 0xff;  // high byte

  arg.count = count;
  for ( i = 0; i < count; i++ ) {
    arg.data[i] = data[i];
  }
  PMD_SendOutputReport(hid, MEM_WRITE, (__u8 *) &arg, sizeof(arg), FS_DELAY);
  return 0;
}

void usbGetAll_USB1608FS(HIDInterface* hid, __u8 data[])
{
  __u8 reportID = GET_ALL;
  struct t_get_all {
    __u8 reportID;
    __u8 values[19];
  } get_all;

  PMD_SendOutputReport(hid, GET_ALL, &reportID, sizeof(reportID), FS_DELAY);
  usb_interrupt_read(hid->dev_handle, USB_ENDPOINT_IN | 2,
		     (char *) &get_all, sizeof(get_all), FS_DELAY);
  memcpy(data, get_all.values, 19);
  return;
}

float volts_1608FS( const int gain, const signed short num )
{
  float volt;
  const float correction = 1.17;
  
  switch( gain ) {
    case BP_10_00V:
      volt = num * correction * 10.0 / 0x7fff;
      break;
    case BP_5_00V:
      volt = num * correction * 5.00 / 0x7fff;
      break;
    case BP_2_50V:
      volt = num * correction * 2.93 / 0x7fff;
      break;
    case BP_2_00V:
      volt = num * correction * 2.34 / 0x7fff;
      break;
    case BP_1_25V:
      volt = num * correction * 1.46 / 0x7fff;
      break;
    case BP_1_00V:
      volt = num * correction * 1.0 / 0x7fff;
      break;
    case BP_0_625V:
      volt = num * correction * 0.625 / 0x7fff;
      break;
    case BP_0_3125V:
      volt = num * correction * 1.0 / 0x7fff;
      break;
  }

  return volt;
}
