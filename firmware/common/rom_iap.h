/*
 * Copyright 2013 Benjamin Vernoux <titanmkd@gmail.com>
 *
 * This file is part of HackRF.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __ROM_IAP__
#define __ROM_IAP__

#include <stdint.h>

typedef enum 
{
	/* TODO define other commands */

	IAP_CMD_INIT_IAP = 49,
	/* Command Init IAP
	Input Command code: 49 (decimal)
	Return Code CMD_SUCCESS
	Result None
	Description Initializes and prepares the flash for erase and write operations.
	Stack usage 88 B */

	IAP_CMD_READ_PART_ID_NO	= 54,
	/* Read part identification number
	Command Read part identification number
	Input 	Command code: 54 (decimal)
			Parameters:None
	Return 	Code CMD_SUCCESS |
	Result 	Result0:Part Identification Number.
			Result1:Part Identification Number.
	Description This command is used to read the part identification number. See Table 1082 
	“LPC43xx part identification numbers”.
	The command returns two words: word0 followed by word1. Word 0 corresponds 
	to the part id and word1 indicates the flash configuration or contains 0x0 for 
	flashless parts. 
	Stack usage 8 B	*/

	IAP_CMD_READ_SERIAL_NO = 58
	/* Read device serial number
	Input Command code: 58 (decimal)
	Parameters: None
	Return Code CMD_SUCCESS
	Result	Result0:First 32-bit word of Device Identification Number (at the lowest address)
			Result1:Second 32-bit word of Device Identification Number
			Result2:Third 32-bit word of Device Identification Number
			Result3:Fourth 32-bit word of Device Identification Number
	Description This command is used to read the device identification number. The serial number 
	may be used to uniquely identify a single unit among all LPC43xx devices.
	Stack usage 8 B	*/
} iap_cmd_code_t;

/* ISP/IAP Return Code */
typedef enum 
{
	CMD_SUCCESS					   = 0x00000000, /* CMD_SUCCESS Command is executed successfully. Sent by ISP handler only when command given by the host has been completely and successfully executed. */
	INVALID_COMMAND				   = 0x00000001, /* Invalid command. */
	SRC_ADDR_ERROR 				   = 0x00000002, /*  Source address is not on word boundary. */
	DST_ADDR_ERROR				   = 0x00000003, /* Destination address not on word or 256 byte boundary. */
	SRC_ADDR_NOT_MAPPED			   = 0x00000004, /* Source address is not mapped in the memory map. Count value is taken into consideration where applicable. */
	DST_ADDR_NOT_MAPPED			   = 0x00000005, /* Destination address is not mapped in the memory map. Count value is taken into consideration where applicable.*/
	COUNT_ERROR                    = 0x00000006, /* Byte count is not multiple of 4 or is not a permitted value. */
	INVALID_SECTOR                 = 0x00000007, /* Sector number is invalid or end sector number is greater than start sector number. */
	SECTOR_NOT_BLANK               = 0x00000008, /* Sector is not blank. */
	SECTOR_NOT_PREP_WRITE_OP       = 0x00000009, /* Command to prepare sector for write operation was not executed. */
	COMPARE_ERROR                  = 0x0000000A, /* Source and destination data not equal. */
	BUSY                    	   = 0x0000000B, /* Flash programming hardware interface is busy. */
	PARAM_ERROR                    = 0x0000000C, /* Insufficient number of parameters or invalid parameter. */
	ADDR_ERROR                     = 0x0000000D, /* Address is not on word boundary. */
	ADDR_NOT_MAPPED				   = 0x0000000E, /* Address is not mapped in the memory map. Count value is taken in to consideration where applicable. */
	CMD_LOCKED                     = 0x0000000F, /* Command is locked. */
	INVALID_CODE                   = 0x00000010, /* Unlock code is invalid. */
	INVALID_BAUD_RATE			   = 0x00000011, /* Invalid baud rate setting. */
	INVALID_STOP_BIT			   = 0x00000012, /* Invalid stop bit setting. */
	CODE_READ_PROTECTION_ENABLED   = 0x00000013, /* Code read protection enabled. */
	INVALID_FLASH_UNIT 			   = 0x00000014, /* Invalid flash unit. */
	USER_CODE_CHECKSUM 			   = 0x00000015, 
	ERROR_SETTING_ACTIVE_PARTITION = 0x00000016,
	
	/* Special Error */
	ERROR_IAP_NOT_IMPLEMENTED      = 0x00000100 /* IAP is not implemented in this part */
} isp_iap_ret_code_t;

typedef struct
{
	/* Input Command/Param */
	struct
	{
		iap_cmd_code_t command_code;
		uint32_t iap_param[5];
	} cmd_param;
	
	/* Output Status/Result */
	struct
	{
		isp_iap_ret_code_t status_ret; 
		uint32_t iap_result[4];
	} status_res;
} iap_cmd_res_t;

/* Check if IAP is implemented */
bool iap_is_implemented(void);

isp_iap_ret_code_t iap_cmd_call(iap_cmd_res_t* iap_cmd_res);

#endif//__ROM_IAP__
