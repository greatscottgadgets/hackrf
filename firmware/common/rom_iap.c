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

 #include "hackrf_core.h"
#include <stdint.h>

#include "rom_iap.h"
#include "w25q80bv.h"

#define ROM_IAP_ADDR (0x10400100)
#define ROM_IAP_UNDEF_ADDR (0x12345678)

#define ROM_OTP_PART_ID_ADDR (0x40045000)

typedef void (* IAP_t)(uint32_t [],uint32_t[]);

typedef	struct {
   const IAP_t IAP;	/* If equal to 0x12345678 IAP not implemented */
   /* Other TBD */
} *pENTRY_ROM_API_t; 
#define pROM_API ((pENTRY_ROM_API_t)ROM_IAP_ADDR)

/* 
 See Errata sheet ES_LPC43X0_A.pdf (LPC4350/30/20/10 Rev A)
 3.5 IAP.1: In-Application Programming API not present on flashless parts
 Introduction:
 The LPC43x0 microcontrollers contain an APIfor In-Application Programming of flash 
 memory. This API also allows identification of the part.
 Problem:
 On the LPC43x0 microcontrollers, the IAP API is not present. The ISP interface is present 
 which allows the part to be identified externally (via the UART) but part identification is not 
 possible internally using the IAP call because it is not implemented.
 The first word of the Part ID can be read directly from OTP at 0x40045000. The second word of the Part ID is always 
'0' on flashless parts. */

bool iap_is_implemented(void)
{
	bool res;
	if( *((uint32_t*)ROM_IAP_ADDR) != ROM_IAP_UNDEF_ADDR )
	{
		res = true;
	}else
	{
		res = false;
	}
	return res;
}

isp_iap_ret_code_t iap_cmd_call(iap_cmd_res_t* iap_cmd_res) 
{
	uint32_t* p_u32_data;
	
	if( iap_is_implemented() )
	{
		pROM_API->IAP( (uint32_t*)&iap_cmd_res->cmd_param, (uint32_t*)&iap_cmd_res->status_res);
	}else
	{
		/* 
		  Alternative way to retrieve Part Id on MCU with no IAP 
		  Read Serial No => Read Unique ID in SPIFI (only compatible with W25Q80BV
		*/
		w25q80bv_setup();

		switch(iap_cmd_res->cmd_param.command_code)
		{
			case IAP_CMD_READ_PART_ID_NO:
				p_u32_data = (uint32_t*)ROM_OTP_PART_ID_ADDR;
				iap_cmd_res->status_res.iap_result[0] = p_u32_data[0];
				iap_cmd_res->status_res.iap_result[1] = p_u32_data[1];
				iap_cmd_res->status_res.status_ret = CMD_SUCCESS;
			break;
			
			case IAP_CMD_READ_SERIAL_NO:
			/* Only 64bits used */
			iap_cmd_res->status_res.iap_result[0] = 0;
			iap_cmd_res->status_res.iap_result[1] = 0;
			w25q80bv_get_unique_id( (w25q80bv_unique_id_t*)&iap_cmd_res->status_res.iap_result[2] );
				iap_cmd_res->status_res.status_ret = CMD_SUCCESS;
			break;
			
			default:
				iap_cmd_res->status_res.status_ret = ERROR_IAP_NOT_IMPLEMENTED;
			break;
		}
	}
	return iap_cmd_res->status_res.status_ret;
}
