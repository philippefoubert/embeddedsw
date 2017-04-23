/******************************************************************************
*
* Copyright (C) 2016 - 17 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/
/*****************************************************************************/
/**
*
* @file xilskey_puf_registration.h
*
* This file contains header interface related information for PUF device
* and macros used in the driver
*
* @note
*
*               User configurable parameters for PUF
*------------------------------------------------------------------------------
*
*	#define	XSK_PUF_INFO_ON_UART			FALSE
*	TRUE will display syndrome data on UART com port
*	FALSE will display any data on UART com port.
*
*	#define XSK_PUF_PROGRAM_EFUSE			FALSE
*	TRUE will programs the generated syndrome data, CHash and Auxilary values,
*	Black key.
*	FALSE will not program data into eFUSE.
*
*	#define XSK_PUF_IF_CONTRACT_MANUFATURER		FALSE
*	This should be enabled when application is hand over to contract
*	manufacturer.
*	TRUE will allow only authenticated application.
*	FALSE authentication is not mandatory.
*
*	#define XSK_PUF_REG_MODE			XSK_PUF_MODE4K
*	PUF registration is performed in 4K mode. For only understanding it is
*	provided in this file, but user is not supposed to modify this.
*
*	#define XSK_PUF_READ_SECUREBITS			FALSE
*	TRUE will read status of the puf secure bits from eFUSE and will be
*	displayed on UART.
*	FALSE will not read secure bits.
*
*	#define XSK_PUF_PROGRAM_SECUREBITS		FALSE
*	TRUE will program PUF secure bits based on the user input provided
*	at XSK_PUF_SYN_INVALID, XSK_PUF_SYN_WRLK and XSK_PUF_REGISTER_DISABLE
*	FALSE will not program any PUF secure bits.
*
*	#define XSK_PUF_SYN_INVALID			FALSE
*	TRUE will permanently invalidates the already programmed syndrome data.
*	FALSE will not modify anything
*
*	#define XSK_PUF_SYN_WRLK			FALSE
*	TRUE will permanently disables programming syndrome data into eFUSE.
*	FALSE will not modify anything.
*
*	#define XSK_PUF_REGISTER_DISABLE		FALSE
*	TRUE permanently does not allows PUF syndrome data registration.
*	FALSE will not modify anything.
*
*	#define XSK_PUF_RESERVED				FALSE
*	TRUE programs this reserved eFUSE bit.
*	FALSE will not modify anything.
*
*	#define		XSK_PUF_AES_KEY
*	"0000000000000000000000000000000000000000000000000000000000000000"
*	The value mentioned in this will be converted to hex buffer and encrypts
*	this with PUF helper data and generates a black key and written
*	into the ZynqMP PS eFUSE array when XSK_PUF_PROGRAM_EFUSE macro is TRUE
*	This value should be given in string format. It should be 64 characters
*	long, valid characters are 0-9,a-f,A-F. Any other character is
*	considered as invalid string and will not burn AES Key.
*	Note: Provided here should be red key and application calculates the
*	black key and programs into eFUSE if XSK_PUF_PROGRAM_EFUSE macro is
*	TRUE.
*	To avoid programming eFUSE results can be displayed on UART com port
*	by making XSK_PUF_INFO_ON_UART to TRUE.
*
*	#define		XSK_PUF_IV	"000000000000000000000000"
*	The value mentioned here will be converted to hex buffer.
*	This is Initialization vector(IV) which is used to generated black key
*	with provided AES key and generated PUF key.
*	This value should be given in string format. It should be 24 characters
*	long, valid characters are 0-9,a-f,A-F. Any other character is
*	considered as invalid string.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date        Changes
* ----- ---- -------- -------------------------------------------------------
* 6.1   rp   17/10/16 First release.
* 6.2   vns  03/10/17 Added support for programming and reading one reserved
*                     bit
* </pre>
*
*
******************************************************************************/
#ifndef XILSKEY_PUF_REGISTRATION_H_
#define XILSKEY_PUF_REGISTRATION_H_

#ifdef __cplusplus
extern "C" {
#endif

/************************** Include Files ************************************/

#include "xsecure_aes.h"
#include "xilskey_eps_zynqmp_puf.h"

/************************** Constant Definitions ****************************/

#ifdef	XSK_ZYNQ_ULTRA_MP_PLATFORM
#define XSK_CSUDMA_DEVICE_ID		XPAR_XCSUDMA_0_DEVICE_ID
#endif

#define 	XSK_PUF_MODE4K		(0U)

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/* Following parameters should be configured by user */

#define XSK_PUF_INFO_ON_UART			FALSE
#define XSK_PUF_PROGRAM_EFUSE			FALSE
#define XSK_PUF_IF_CONTRACT_MANUFATURER		FALSE

/* For programming/reading secure bits of PUF */
#define XSK_PUF_READ_SECUREBITS			FALSE
#define XSK_PUF_PROGRAM_SECUREBITS		FALSE

#if (XSK_PUF_PROGRAM_SECUREBITS == TRUE)
#define	XSK_PUF_SYN_INVALID			FALSE
#define	XSK_PUF_SYN_WRLK			FALSE
#define	XSK_PUF_REGISTER_DISABLE		FALSE
#define	XSK_PUF_RESERVED			FALSE
#endif

#define	XSK_PUF_AES_KEY		"0000000000000000000000000000000000000000000000000000000000000000"
#define	XSK_PUF_IV		"000000000000000000000000"

#define XSK_PUF_REG_MODE			XSK_PUF_MODE4K
						/**< Registration Mode
						  *  XPUF_MODE4K */

/***************************End of configurable parameters********************/

#if (XSK_PUF_INFO_ON_UART == TRUE)
#define	XPUF_INFO_ON_UART		/**< If defined, sends  information
					  *  on UART */
#define XPUF_DEBUG_GENERAL		1
#else
#define XPUF_DEBUG_GENERAL		0
#endif


#if (XSK_PUF_PROGRAM_EFUSE == TRUE)
#define	XPUF_FUSE_SYN_DATA		/**< If defined, writes syndrome data,
					  *  black key, Aux and Chash
					  *  values into eFUSE */
#endif

#if (XSK_PUF_IF_CONTRACT_MANUFATURER == TRUE)
#define	XPUF_CONTRACT_MANUFACTURER	/**< If defined, additional checks
					  *  will be made to verify that app
					  *  is authenticated before running*/
#endif

/************************** Type Definitions **********************************/

/* All the instances used for this application */
XilSKey_Puf PufInstance;
XSecure_Aes AesInstance;
XCsuDma CsuDma;
XilSKey_ZynqMpEPs EfuseInstance;

/************************** Function Prototypes ******************************/

#ifdef __cplusplus
}
#endif

#endif /* XILSKEY_PUF_REGISTRATION_H_ */
