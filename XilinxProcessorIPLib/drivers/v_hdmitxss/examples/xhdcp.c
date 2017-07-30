/******************************************************************************
*
* Copyright (C) 2016 Xilinx, Inc.  All rights reserved.
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
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* XILINX CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
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
* @file xhdcp.c
*
* This file contains the main implementation of the Xilinx HDCP abstraction
* layer. The HDCP abstraction layer can support repeater topologies with a
* single upstream interface and up to 32 downstream interfaces. Both HDCP
* 1.4 and 2.2 protocols are supported. The interactions between the repeater
* upstream and downstream interface are implemented in the HDCP abstraction
* layer including: upstream topology propagation, and downstream stream
* management propagation.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date     Changes
* ----- ---- -------- -----------------------------------------------
* 1.00  MH   05/24/16 First Release
* 1.10  MG   10/21/16 Updated the Hdcp_Poll function
* 1.20  MG   10/26/16 Added interval in Hdcp_Poll function
*</pre>
*
*****************************************************************************/

/***************************** Include Files *********************************/
#include <string.h>
#include "xhdcp.h"
#include "xparameters.h"

/************************** Constant Definitions ****************************/
#if defined (XPAR_XHDCP_NUM_INSTANCES) || defined (XPAR_XHDCP22_RX_NUM_INSTANCES) || defined (XPAR_XHDCP22_TX_NUM_INSTANCES)
/* If HDCP 1.4 or HDCP 2.2 is in the system then use the HDCP abstraction layer */
#define USE_HDCP
#endif

/**************************** Type Definitions ******************************/

/***************** Macros (Inline Functions) Definitions ********************/

/************************** Variable Definitions ****************************/

/************************** Function Prototypes *****************************/
#ifdef USE_HDCP
static void XHdcp_AuthenticationRequestCallback(void *HdcpInstancePtr);
static void XHdcp_TopologyUpdateCallback(void *HdcpInstancePtr);
static void XHdcp_StreamManageRequestCallback(void *HdcpInstancePtr);
static void XHdcp_UpstreamAuthenticatedCallback(void *HdcpInstancePtr);
static void XHdcp_DownstreamAuthenticatedCallback(void *HdcpInstancePtr);
static void XHdcp_UpstreamUnauthenticatedCallback(void *HdcpInstancePtr);
static void XHdcp_DownstreamUnauthenticatedCallback(void *HdcpInstancePtr);
static void XHdcp_UpstreamEncryptionUpdateCallback(void *HdcpInstancePtr);
static void XHdcp_TopologyAvailableCallback(void *HdcpInstancePtr);
static void XHdcp_AssembleTopology(XHdcp_Repeater *InstancePtr);
static void XHdcp_DisplayTopology(XHdcp_Repeater *InstancePtr, u8 Verbose);
static void XHdcp_SetContentStreamType(XHdcp_Repeater *InstancePtr,
              XV_HdmiTxSs_HdcpContentStreamType StreamType);
static void XHdcp_EnforceBlank(XHdcp_Repeater *InstancePtr);
static int  XHdcp_Flag2Count(u32 Flag);

/*****************************************************************************/
/**
*
* This function is used to initialize the HDCP repeater instance.
*
* @param    InstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   - XST_SUCCESS or XST_FAILURE
*
* @note	    None.
*
******************************************************************************/
int XHdcp_Initialize(XHdcp_Repeater *InstancePtr)
{
  /* Verify arguments */
  Xil_AssertNonvoid(InstancePtr != NULL);

  /* Set default values */
  InstancePtr->UpstreamInstanceBinded = 0;
  InstancePtr->DownstreamInstanceBinded = 0;
  InstancePtr->UpstreamInstanceConnected = 0;
  InstancePtr->DownstreamInstanceConnected = 0;
  InstancePtr->DownstreamInstanceStreamUp = 0;
  InstancePtr->UpstreamInstanceStreamUp = 0;

  InstancePtr->StreamType = XV_HDMITXSS_HDCP_STREAMTYPE_0;
  InstancePtr->AuthenticationRequestEvent = 0;
  memset(&InstancePtr->Topology, 0, sizeof(XHdcp_Topology));

  /* Instance is ready only after upstream and at least one downstream
     has been binded */
  InstancePtr->IsReady = (FALSE);

  return (XST_SUCCESS);
}

/*****************************************************************************/
/**
*
* This function is used to bind an HDMI Receiver instance as
* the repeater upstream interface. This function should be called
* once per repeater topology to set the upstream interface.
*
* @param    InstancePtr is a pointer to the XHdcp_Repeater instance.
* @param    UpstreamInstancePtr is a pointer to the HDMI receiver instance.
*
* @return   - XST_SUCCESS if upstream interface registered successfully.
*           - XST_FAILURE if upstream interface could not be registered.
*
* @note	    None.
*
******************************************************************************/
int XHdcp_SetUpstream(XHdcp_Repeater *InstancePtr,
      XV_HdmiRxSs *UpstreamInstancePtr)
{
  int Status;

  /* Verify arguments */
  Xil_AssertNonvoid(InstancePtr != NULL);
  Xil_AssertNonvoid(UpstreamInstancePtr != NULL);

  /* Bind upstream interface */
  InstancePtr->UpstreamInstancePtr = UpstreamInstancePtr;

  /* Set callback functions */
  Status = XV_HdmiRxSs_SetCallback(UpstreamInstancePtr,
    XV_HDMIRXSS_HANDLER_HDCP_AUTHENTICATION_REQUEST,
	(void *)XHdcp_AuthenticationRequestCallback,
    (void *)InstancePtr);

  if (Status != XST_SUCCESS) {
    return (XST_FAILURE);
  }

  Status = XV_HdmiRxSs_SetCallback(UpstreamInstancePtr,
    XV_HDMIRXSS_HANDLER_HDCP_STREAM_MANAGE_REQUEST,
	(void *)XHdcp_StreamManageRequestCallback,
    (void *)InstancePtr);

  if (Status != XST_SUCCESS) {
    return (XST_FAILURE);
  }

  Status = XV_HdmiRxSs_SetCallback(UpstreamInstancePtr,
    XV_HDMIRXSS_HANDLER_HDCP_TOPOLOGY_UPDATE,
	(void *)XHdcp_TopologyUpdateCallback,
    (void *)InstancePtr);

  if (Status != XST_SUCCESS) {
    return (XST_FAILURE);
  }

  Status = XV_HdmiRxSs_SetCallback(UpstreamInstancePtr,
    XV_HDMIRXSS_HANDLER_HDCP_AUTHENTICATED,
	(void *)XHdcp_UpstreamAuthenticatedCallback,
    (void *)InstancePtr);

  if (Status != XST_SUCCESS) {
    return (XST_FAILURE);
  }

  Status = XV_HdmiRxSs_SetCallback(UpstreamInstancePtr,
    XV_HDMIRXSS_HANDLER_HDCP_UNAUTHENTICATED,
	(void *)XHdcp_UpstreamUnauthenticatedCallback,
    (void *)InstancePtr);

  if (Status != XST_SUCCESS) {
    return (XST_FAILURE);
  }

  Status = XV_HdmiRxSs_SetCallback(UpstreamInstancePtr,
    XV_HDMIRXSS_HANDLER_HDCP_ENCRYPTION_UPDATE,
	(void *)XHdcp_UpstreamEncryptionUpdateCallback,
    (void *)InstancePtr);

  if (Status != XST_SUCCESS) {
    return (XST_FAILURE);
  }

  /* Indicate upstream interface has been binded */
  InstancePtr->UpstreamInstanceBinded = (TRUE);

  /* Set ready when the upstream interface and at least one downstream interface is binded */
  if (InstancePtr->DownstreamInstanceBinded > 0) {
    InstancePtr->IsReady = (TRUE);
  }

  return (XST_SUCCESS);
}

/*****************************************************************************/
/**
*
* This function is used to bind an HDMI transmitter instance as
* a repeater downstream interface. This function should be called
* for each downstream interface in a repeater topology. The maximum
* downstream interfaced is indicated by XHDCP_MAX_DOWNSTREAM_INTERFACES.
*
* @param    InstancePtr is a pointer to the XHdcp_Repeater instance.
* @param    UpstreamInstancePtr is a pointer to the HDMI receiver instance.
*
* @return   - XST_SUCCESS if downstream interface registered successfully.
*           - XST_FAILURE if downstream interface could not be registered.
*
* @note	    None.
*
******************************************************************************/
int XHdcp_SetDownstream(XHdcp_Repeater *InstancePtr,
      XV_HdmiTxSs *DownstreamInstancePtr)
{
  int Status;

  /* Verify arguments */
  Xil_AssertNonvoid(InstancePtr != NULL);
  Xil_AssertNonvoid(DownstreamInstancePtr != NULL);

  /* Bind downstream interface */
  if (InstancePtr->DownstreamInstanceBinded <= XHDCP_MAX_DOWNSTREAM_INTERFACES) {
    InstancePtr->DownstreamInstancePtr[InstancePtr->DownstreamInstanceBinded] =
      DownstreamInstancePtr;
  } else {
    return (XST_FAILURE);
  }

  /* Set callback functions */
  Status = XV_HdmiTxSs_SetCallback(DownstreamInstancePtr,
    XV_HDMITXSS_HANDLER_HDCP_DOWNSTREAM_TOPOLOGY_AVAILABLE,
	(void *)XHdcp_TopologyAvailableCallback,
    (void *)InstancePtr);

  if (Status != XST_SUCCESS) {
    return (XST_FAILURE);
  }

  Status = XV_HdmiTxSs_SetCallback(DownstreamInstancePtr,
    XV_HDMITXSS_HANDLER_HDCP_UNAUTHENTICATED,
	(void *)XHdcp_DownstreamUnauthenticatedCallback,
    (void *)InstancePtr);

  if (Status != XST_SUCCESS) {
    return (XST_FAILURE);
  }

  Status = XV_HdmiTxSs_SetCallback(DownstreamInstancePtr,
    XV_HDMITXSS_HANDLER_HDCP_AUTHENTICATED,
	(void *)XHdcp_DownstreamAuthenticatedCallback,
    (void *)InstancePtr);

  if (Status != XST_SUCCESS) {
    return (XST_FAILURE);
  }

  /* Increment downstream interface count */
  (InstancePtr->DownstreamInstanceBinded)++;

  /* Set ready when the upstream interface and
     at least one downstream interface is binded */
  if ((InstancePtr->UpstreamInstanceBinded == TRUE) &&
      (InstancePtr->DownstreamInstanceBinded > 0)) {
    InstancePtr->IsReady = (TRUE);
  }

  return (XST_SUCCESS);
}

/*****************************************************************************/
/**
*
* This function is responsible for executing the state machine for the
* upstream interface and each connected downstream interface. The
* state machines are executed using round robin scheduling. Interface
* poll functions are non-blocking, so starvation should not occur, but
* fairness is not guaranteed. Authentication requests are serviced
* in this function for each downstream interface.
*
* @param    InstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
void XHdcp_Poll(XHdcp_Repeater *InstancePtr)
{
  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  /* This counter is used as an interval timer */
  /* Some HDCP source devices might not delay the HDPC capabilities after the video stream has started */
  /* As a result the link isn't authenticated at stream up */
  /* When there is a authentication request and the HDMI TX SS driver doesn't detect a HDCP capable sink, */
  /* then the authentication request remains pending the interval counter is set. */
  /* When the interval counter expires the sink is checked again for HDCP capabilities */
  static u32 IntervalCounter = 0;

  if (InstancePtr->IsReady) {
    /* Call the upstream interface Poll function */
    XV_HdmiRxSs_HdcpPoll(InstancePtr->UpstreamInstancePtr);

    /* Call each downstream interface Poll function */
    for (int i = 0; i < InstancePtr->DownstreamInstanceBinded; i++) {
      XV_HdmiTxSs_HdcpPoll(InstancePtr->DownstreamInstancePtr[i]);

      /* Initiate authentication when request flag has been set */
      if (InstancePtr->AuthenticationRequestEvent & (0x1 << i)) {

        /* Delay until downstream interface stream is up and the counter has expired */
        if ((InstancePtr->DownstreamInstanceStreamUp & (0x1 << i)) &&
            (InstancePtr->DownstreamInstanceConnected & (0x1 << i)) &&
			(IntervalCounter == 0)) {

          /* Authenticate when HDCP is support for connected device, otherwise enforce blank */
          if (XV_HdmiTxSs_IsSinkHdcp14Capable(InstancePtr->DownstreamInstancePtr[i]) ||
              XV_HdmiTxSs_IsSinkHdcp22Capable(InstancePtr->DownstreamInstancePtr[i])) {
		  XV_HdmiTxSs_HdcpPushEvent(InstancePtr->DownstreamInstancePtr[i],
              XV_HDMITXSS_HDCP_AUTHENTICATE_EVT);

		  /* Clear the authentication request */
		  InstancePtr->AuthenticationRequestEvent &= ~(0x1 << i);

          } else {
		  /* Force blank, but leave the authentication request pending */
		  XHdcp_EnforceBlank(InstancePtr);
		  /* Load counter */
		  /* With the processor running at 100 MHz, this counter value will give roughly a 500 ms interval */
		  IntervalCounter = 100000;
          }
        } else {
		/* Decrement counter */
		if (IntervalCounter > 0) {
			IntervalCounter--;
		}
        }
      }
    }
  }
}

/*****************************************************************************/
/**
*
* This function is called to trigger authentication for each downstream
* interface.
*
* @param    InstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
void XHdcp_Authenticate(XHdcp_Repeater *InstancePtr)
{
  int HdcpProtocol;

  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  if (InstancePtr->IsReady) {
    /* Check the upstream protocol */
    HdcpProtocol = XV_HdmiRxSs_HdcpGetProtocol(InstancePtr->UpstreamInstancePtr);

    /* Set authentication request flag for each connected downstream interface */
    for (int i = 0; (i < InstancePtr->DownstreamInstanceBinded); i++) {

      if (InstancePtr->DownstreamInstanceConnected & (0x1 << i)) {

        /* If downstream is already authenticated or busy then don't trigger authentication */
        if (!(XV_HdmiTxSs_HdcpIsAuthenticated(InstancePtr->DownstreamInstancePtr[i])) &&
            !(XV_HdmiTxSs_HdcpIsInProgress(InstancePtr->DownstreamInstancePtr[i]))) {

          InstancePtr->AuthenticationRequestEvent |= (0x1 << i);
        }

        /* Toggle */
        else if (XV_HdmiTxSs_IsStreamToggled(InstancePtr->DownstreamInstancePtr[i])) {
          InstancePtr->AuthenticationRequestEvent |= (0x1 << i);
        }

        /* HDCP 1.4 only */
        else if(XV_HdmiTxSs_HdcpIsAuthenticated(InstancePtr->DownstreamInstancePtr[i]) &&
                (XV_HdmiRxSs_HdcpIsInComputations(InstancePtr->UpstreamInstancePtr) ||
                XV_HdmiRxSs_HdcpIsInWaitforready(InstancePtr->UpstreamInstancePtr))) {

          if (HdcpProtocol == XV_HDMIRXSS_HDCP_14) {
            InstancePtr->AuthenticationRequestEvent |= (0x1 << i);
          }
        }
      }

      /* When the upstream protocol is HDCP 1.4 set the default stream
         type to zero for all downstream interfaces */
      if (HdcpProtocol == XV_HDMIRXSS_HDCP_14) {

        InstancePtr->StreamType = XV_HDMITXSS_HDCP_STREAMTYPE_0;
        XV_HdmiTxSs_HdcpSetContentStreamType(InstancePtr->DownstreamInstancePtr[i],
          XV_HDMITXSS_HDCP_STREAMTYPE_0);
      }
    }
  }
}

/*****************************************************************************/
/**
*
* This function displays the repeater layer instance information.
*
* @param    InstancePtr is a pointer to the XHdcp_Repeater instance.
* @param    Verbose can be set to TRUE to display device list.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
void XHdcp_DisplayInfo(XHdcp_Repeater *InstancePtr, u8 Verbose)
{
  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  xil_printf("HDCP Repeater Info\n\r");
  xil_printf("Upstream Binded: %d\n\r", InstancePtr->UpstreamInstanceBinded);
  xil_printf("Upstream Connected: %d\n\r", InstancePtr->UpstreamInstanceConnected);
  xil_printf("Upstream Stream-Up: %d\n\r", InstancePtr->UpstreamInstanceStreamUp);
  xil_printf("Downstream Binded: %d\n\r", InstancePtr->DownstreamInstanceBinded);
  xil_printf("Downstream Connected: 0x%08x\n\r", InstancePtr->DownstreamInstanceConnected);
  xil_printf("Downstream Stream-Up: 0x%08x\n\r", InstancePtr->DownstreamInstanceStreamUp);
  xil_printf("Downstream Authentication Request: 0x%08x\n\r", InstancePtr->AuthenticationRequestEvent);
  if (XV_HdmiRxSs_HdcpIsRepeater(InstancePtr->UpstreamInstancePtr))
    xil_printf("StreamType: %d\n\r", InstancePtr->StreamType);
  XHdcp_DisplayTopology(InstancePtr, Verbose);
}

/*****************************************************************************/
/**
*
* This function enables encryption for each authenticated downstream
* interface.
*
* @param    InstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
void XHdcp_EnableEncryption(XHdcp_Repeater *InstancePtr)
{
  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  if (InstancePtr->IsReady) {
    for (int i = 0; i < InstancePtr->DownstreamInstanceBinded; i++) {
      XV_HdmiTxSs_HdcpEnableEncryption(InstancePtr->DownstreamInstancePtr[i]);
    }
  }
}

/*****************************************************************************/
/**
*
* This function disables encryption for each downstream interface.
*
* @param    InstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
void XHdcp_DisableEncryption(XHdcp_Repeater *InstancePtr)
{
  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  if (InstancePtr->IsReady) {
    for (int i = 0; i < InstancePtr->DownstreamInstanceBinded; i++) {
      XV_HdmiTxSs_HdcpDisableEncryption(InstancePtr->DownstreamInstancePtr[i]);
    }
  }
}

/*****************************************************************************/
/**
*
* This function sets the repeater mode for each interface. HPD is toggled
* after the repeater mode is changed.
*
* @param    InstancePtr is a pointer to the XHdcp_Repeater instance.
* @param    Set is TRUE to enable or FALSE to disable repeater.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
void XHdcp_SetRepeater(XHdcp_Repeater *InstancePtr, u8 Set)
{
  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  if (InstancePtr->IsReady) {
    /* Set upstream */
    XV_HdmiRxSs_HdcpSetRepeater(InstancePtr->UpstreamInstancePtr, Set);

    /* Set downstream */
    for (int i = 0; i < InstancePtr->DownstreamInstanceBinded; i++) {
      XV_HdmiTxSs_HdcpSetRepeater(InstancePtr->DownstreamInstancePtr[i], Set);
    }

    /* Toggle HPD if upstream is connected */
    if (InstancePtr->UpstreamInstanceConnected) {
      XV_HdmiRxSs_ToggleHpd(InstancePtr->UpstreamInstancePtr);
    }
  }
}

/*****************************************************************************/
/**
*
* This function is called by the stream up event for an
* interface. The function initiates authentication with each
* connected downstream interface that is not in the authenticated
* state. The function is registered with the connect event.
* This function also sets the default content stream management
* type to zero when the upstream interface is HDCP 1.4.
*
* @param    HdcpInstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
void XHdcp_StreamUpCallback(void *HdcpInstancePtr)
{
  XHdcp_Repeater *InstancePtr =  (XHdcp_Repeater *)HdcpInstancePtr;

  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  /* Upstream interface stream up */
  if (XV_HdmiRxSs_IsStreamUp(InstancePtr->UpstreamInstancePtr)) {
    /* Clear topology */
    if (!InstancePtr->UpstreamInstanceStreamUp) {
      memset(&InstancePtr->Topology, 0, sizeof(XHdcp_Topology));
    }

    InstancePtr->UpstreamInstanceStreamUp = TRUE;
  }

  /* Downstream interface stream up */
  for (int i = 0; i < InstancePtr->DownstreamInstanceBinded; i++) {
    if (XV_HdmiTxSs_IsStreamUp(InstancePtr->DownstreamInstancePtr[i])) {
      /* Trigger authentication */
      if (!(InstancePtr->DownstreamInstanceStreamUp & (0x1 << i))) {
        XHdcp_Authenticate(InstancePtr);
      }

      InstancePtr->DownstreamInstanceStreamUp |= (0x1 << i);
    }
  }

  /* Enforce blanking */
  XHdcp_EnforceBlank(InstancePtr);
}

/*****************************************************************************/
/**
*
* This function is called by the stream down event for an interface.
*
* @param    HdcpInstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
void XHdcp_StreamDownCallback(void *HdcpInstancePtr)
{
  XHdcp_Repeater *InstancePtr =  (XHdcp_Repeater *)HdcpInstancePtr;

  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  /* Upstream interface stream down */
  if (!XV_HdmiRxSs_IsStreamUp(InstancePtr->UpstreamInstancePtr)) {
    InstancePtr->UpstreamInstanceStreamUp = FALSE;
  }

  /* Downstream interface stream down */
  for (int i = 0; i < InstancePtr->DownstreamInstanceBinded; i++) {
    if (!XV_HdmiTxSs_IsStreamUp(InstancePtr->DownstreamInstancePtr[i])) {
      InstancePtr->DownstreamInstanceStreamUp &= ~(0x1 << i);
    }
  }

  /* Enforce blanking */
  XHdcp_EnforceBlank(InstancePtr);
}

/*****************************************************************************/
/**
*
* This function is called by the stream connect event for an interface.
*
* @param    HdcpInstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
void XHdcp_StreamConnectCallback(void *HdcpInstancePtr)
{
  XHdcp_Repeater *InstancePtr =  (XHdcp_Repeater *)HdcpInstancePtr;
  u32 DownstreamInstanceConnected = 0;
  int IsRepeater;

  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  /* Check if upstream interface is a repeater */
  IsRepeater = XV_HdmiRxSs_HdcpIsRepeater(InstancePtr->UpstreamInstancePtr);

  /* Check if upstream interface is connected */
  if (XV_HdmiRxSs_IsStreamConnected(InstancePtr->UpstreamInstancePtr)) {
    if (!(InstancePtr->UpstreamInstanceConnected)) {
      InstancePtr->UpstreamInstanceStreamUp = FALSE;
    }
    InstancePtr->UpstreamInstanceConnected = TRUE;
  }

  /* Check if downstream interface is connected */
  for (int i = 0; i < InstancePtr->DownstreamInstanceBinded; i++) {
    if (XV_HdmiTxSs_IsStreamConnected(InstancePtr->DownstreamInstancePtr[i])) {
      if (!(InstancePtr->DownstreamInstanceConnected & (0x1 << i))) {
        InstancePtr->DownstreamInstanceStreamUp &= ~(0x1 << i);
      }
      DownstreamInstanceConnected |= (0x1 << i);
    }
  }

  /* Set HPD high when first active device is connected for repeater mode. */
  if (DownstreamInstanceConnected && !(InstancePtr->DownstreamInstanceConnected)) {
    if (IsRepeater) {
      if (XV_HdmiRxSs_IsStreamConnected(InstancePtr->UpstreamInstancePtr)) {
        XV_HdmiRxSs_SetHpd(InstancePtr->UpstreamInstancePtr, (TRUE));
      }
    }
  }

  /* Set HPD low when no active downstream devices are connected */
  if (!(DownstreamInstanceConnected) && IsRepeater) {
    XV_HdmiRxSs_SetHpd(InstancePtr->UpstreamInstancePtr, (FALSE));
  }

  /* Update the connected flag */
  InstancePtr->DownstreamInstanceConnected = DownstreamInstanceConnected;
}

/*****************************************************************************/
/**
*
* This function is called by the stream disconnect event for an interface.
*
* @param    HdcpInstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
void XHdcp_StreamDisconnectCallback(void *HdcpInstancePtr)
{
  XHdcp_Repeater *InstancePtr =  (XHdcp_Repeater *)HdcpInstancePtr;
  u32 DownstreamInstanceConnected = 0;
  int IsRepeater;

  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  /* Check if upstream interface is a repeater */
  IsRepeater = XV_HdmiRxSs_HdcpIsRepeater(InstancePtr->UpstreamInstancePtr);

  /* Check if upstream interface is disconnected */
  if (!(XV_HdmiRxSs_IsStreamConnected(InstancePtr->UpstreamInstancePtr))) {
    InstancePtr->DownstreamInstanceStreamUp = 0;
    InstancePtr->UpstreamInstanceStreamUp = FALSE;
    InstancePtr->UpstreamInstanceConnected = FALSE;
  }

  /* Check if downstream interface is disconnected */
  for (int i = 0; i < InstancePtr->DownstreamInstanceBinded; i++) {
    if (XV_HdmiTxSs_IsStreamConnected(InstancePtr->DownstreamInstancePtr[i])) {
      DownstreamInstanceConnected |= (0x1 << i);
    } else {
      InstancePtr->DownstreamInstanceStreamUp &= ~(0x1 << i);
    }
  }

  /* When no downstream interfaces are connected drive the HPD low */
  if (!(DownstreamInstanceConnected) && IsRepeater) {
    XV_HdmiRxSs_SetHpd(InstancePtr->UpstreamInstancePtr, (FALSE));
  }

  /* Update the connected flag */
  InstancePtr->DownstreamInstanceConnected = DownstreamInstanceConnected;
}

/*****************************************************************************/
/**
*
* This function sets the content stream type for each downstream interface.
* If the Type is 1 and the downstream protocol is HDCP 1.4, then cipher
* blank is enabled.
*
* @param    InstancePtr is a pointer to the XHdcp_Repeater instance.
* @param    StreamType can be either zero or one.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
static void XHdcp_SetContentStreamType(XHdcp_Repeater *InstancePtr,
       XV_HdmiTxSs_HdcpContentStreamType StreamType)
{
  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  if (InstancePtr->IsReady) {
    for (int i = 0; i < InstancePtr->DownstreamInstanceBinded; i++) {
      xdbg_printf(XDBG_DEBUG_GENERAL, "HDCP StreamType: %d\n\r", InstancePtr->StreamType);
      XV_HdmiTxSs_HdcpSetContentStreamType(InstancePtr->DownstreamInstancePtr[i],
        StreamType);
    }
  }
}

/*****************************************************************************/
/**
*
* This function is called when the upstream interface transitions to the
* authenticated state. Low value content output is set for downstream
* interfaces that are not in the authenticated state.
*
* @param    HdcpInstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
static void XHdcp_UpstreamAuthenticatedCallback(void *HdcpInstancePtr)
{
  XHdcp_Repeater *InstancePtr =  (XHdcp_Repeater *)HdcpInstancePtr;
  int HdcpProtocol;

  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  /* Print message */
  HdcpProtocol = XV_HdmiRxSs_HdcpGetProtocol(InstancePtr->UpstreamInstancePtr);
  switch (HdcpProtocol) {
    case XV_HDMIRXSS_HDCP_22:
      xdbg_printf(XDBG_DEBUG_GENERAL, "HDCP 2.2 upstream authenticated\n\r");
      break;
    case XV_HDMIRXSS_HDCP_14:
      xdbg_printf(XDBG_DEBUG_GENERAL, "HDCP 1.4 upstream authenticated\n\r");
      break;
  }

  /* Enforce blanking */
  XHdcp_EnforceBlank(InstancePtr);
}

/*****************************************************************************/
/**
*
* This function is called when the downstream interface transitions to the
* authenticated state. Encryption will be enabled for a downstream
* interface based on the downstream interface protocol and content stream
* type setting.
*
* @param    HdcpInstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
static void XHdcp_DownstreamAuthenticatedCallback(void *HdcpInstancePtr)
{
  XHdcp_Repeater *InstancePtr =  (XHdcp_Repeater *)HdcpInstancePtr;
  int HdcpProtocol;

  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  /* After authentication start encryption */
  for (int i = 0; (i < InstancePtr->DownstreamInstanceBinded); i++) {

    if (XV_HdmiTxSs_HdcpIsAuthenticated(InstancePtr->DownstreamInstancePtr[i])) {

      /* Check the downstream interface protocol */
      HdcpProtocol = XV_HdmiTxSs_HdcpGetProtocol(InstancePtr->DownstreamInstancePtr[i]);

      switch (HdcpProtocol) {
        /* HDCP 2.2 */
        case XV_HDMITXSS_HDCP_22:
          xdbg_printf(XDBG_DEBUG_GENERAL, "HDCP 2.2 downstream authenticated\n\r");
          XV_HdmiTxSs_HdcpEnableEncryption(InstancePtr->DownstreamInstancePtr[i]);
          break;

        /* HDCP 1.4 */
        case XV_HDMITXSS_HDCP_14:
          xdbg_printf(XDBG_DEBUG_GENERAL, "HDCP 1.4 downstream authenticated\n\r");
          XV_HdmiTxSs_HdcpEnableEncryption(InstancePtr->DownstreamInstancePtr[i]);
          break;
      }
    }
  }

  /* Enforce blanking */
  XHdcp_EnforceBlank(InstancePtr);
}

/*****************************************************************************/
/**
*
* This function is called when the upstream interface transitions from
* an authenticated state to an unauthenticated state.
*
* @param    HdcpInstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
static void XHdcp_UpstreamUnauthenticatedCallback(void *HdcpInstancePtr)
{
  XHdcp_Repeater *InstancePtr =  (XHdcp_Repeater *)HdcpInstancePtr;

  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  /* Clear topology and stream management information */
  memset(&InstancePtr->Topology, 0, sizeof(XHdcp_Topology));
  InstancePtr->StreamType = XV_HDMITXSS_HDCP_STREAMTYPE_0;

  /* Enforce blanking */
  XHdcp_EnforceBlank(InstancePtr);
}

/*****************************************************************************/
/**
*
* This function is called when the downstream interface transitions from
* an authenticated state to an unauthenticated state.
*
* @param    HdcpInstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
static void XHdcp_DownstreamUnauthenticatedCallback(void *HdcpInstancePtr)
{
  XHdcp_Repeater *InstancePtr =  (XHdcp_Repeater *)HdcpInstancePtr;
  int HdcpProtocol;

  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  HdcpProtocol = XV_HdmiRxSs_HdcpGetProtocol(InstancePtr->UpstreamInstancePtr);

  /* HDCP 1.4 Only.
     When the upstream interface is HDCP 1.4 repeater, unauthenticate the upstream interface when
     the downstream transitions to unauthenticated. */
  if (HdcpProtocol == XV_HDMIRXSS_HDCP_14 &&
      XV_HdmiRxSs_HdcpIsRepeater(InstancePtr->UpstreamInstancePtr)) {

    /* If hdcp rx is enabled, then push disconnect event */
    if(XV_HdmiRxSs_HdcpIsEnabled(InstancePtr->UpstreamInstancePtr)) {

      for (int i = 0; i < InstancePtr->DownstreamInstanceBinded; i++) {

        if (InstancePtr->DownstreamInstanceConnected & (0x1 << i)) {

          if (!XV_HdmiTxSs_HdcpIsAuthenticated(InstancePtr->DownstreamInstancePtr[i])) {

		  /* The disconnect event calls a reset wrapper on hdcp state
		   * machine, which in turn calls reset (disable then enable)
		   * and then disable on the hdcp rx state machine */
		  XV_HdmiRxSs_HdcpPushEvent(InstancePtr->UpstreamInstancePtr,
                XV_HDMIRXSS_HDCP_DISCONNECT_EVT);
              break;
          }
        }
      }
    }
  }
  /* HDCP 1.4 Only.
     Trigger re-authentication for HDCP 1.4 downstream interfaces.
     This is required because the HDCP 1.4 state machine does not automatically
     re-authenticate upon failure. */
  else if (!XV_HdmiRxSs_HdcpIsRepeater(InstancePtr->UpstreamInstancePtr)) {
    XHdcp_Authenticate(InstancePtr);
  }

  /* Enforce blanking */
  XHdcp_EnforceBlank(InstancePtr);
}

/*****************************************************************************/
/**
*
* This function is called when the upstream interface encryption status
* has changed.
*
* @param    HdcpInstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
static void XHdcp_UpstreamEncryptionUpdateCallback(void *HdcpInstancePtr)
{
  XHdcp_Repeater *InstancePtr =  (XHdcp_Repeater *)HdcpInstancePtr;

  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  /* Enforce blanking */
  XHdcp_EnforceBlank(InstancePtr);
}

/*****************************************************************************/
/**
*
* This function is called when the upstream interface receives an
* authentication request. The function initiates authentication with
* each connected downstream interface that is not in the authenticated
* state. The function is registered with authentication request event.
* This function also sets the default content stream management type
* to zero when the upstream interface is HDCP 1.4.
*
* @param    HdcpInstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
static void XHdcp_AuthenticationRequestCallback(void *HdcpInstancePtr)
{
  XHdcp_Repeater *InstancePtr =  (XHdcp_Repeater *)HdcpInstancePtr;

  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  /* Clear topology */
  memset(&InstancePtr->Topology, 0, sizeof(XHdcp_Topology));

  /* Trigger authentication */
  XHdcp_Authenticate(InstancePtr);
}

/*****************************************************************************/
/**
*
* This function is called when the HDCP 2.2 upstream interface receives
* a RepeaterAuth_Stream_Manage message. The function gets the content
* stream TYPE and sets the TYPE for each connected HDCP 2.2 downstream
* interface. For HDCP 1.4 downstream interfaces, the
* RepeaterAuth_Steam_Manage message is not supported. When a Type 1
* stream is received this function configures the HDCP 1.4 downstream
* interfaces to produce a blue screen (or some other low value content).
* This function is registered with the HDCP 2.2 stream management request
* callback. When the upstream interface is HDCP 1.4, the
* RepeaterAuth_Steam_Manage message will not be received and there will
* be no stream management request event triggered. In such case, the
* stream type is set to a default value of zero in the Authentication
* Request Event.
*
* @param    HdcpInstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
static void XHdcp_StreamManageRequestCallback(void *HdcpInstancePtr)
{
  XHdcp_Repeater *InstancePtr = (XHdcp_Repeater *)HdcpInstancePtr;

  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  if (InstancePtr->IsReady) {
    /* Get the stream type from the HDCP 2.2 upstream interface */
    InstancePtr->StreamType = XV_HdmiRxSs_HdcpGetContentStreamType(InstancePtr->UpstreamInstancePtr);

    /* Set the stream type for each downstream interface */
    XHdcp_SetContentStreamType(InstancePtr, InstancePtr->StreamType);
  }
}

/*****************************************************************************/
/**
*
* This function is called when the HDCP upstream interface is ready to
* propagate the topology upstream.
*
* @param    HdcpInstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
static void XHdcp_TopologyUpdateCallback(void *HdcpInstancePtr)
{
  XHdcp_Repeater *InstancePtr = (XHdcp_Repeater *)HdcpInstancePtr;

  /* Verify arguments */
  Xil_AssertVoid(HdcpInstancePtr != NULL);

  /* Assemble topology */
  if (InstancePtr->DownstreamInstanceStreamUp) {
    XHdcp_AssembleTopology(InstancePtr);
  }
}

/*****************************************************************************/
/**
*
* This function is called when the HDCP downstream interface has topology
* information available.
*
* @param    HdcpInstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
static void XHdcp_TopologyAvailableCallback(void *HdcpInstancePtr)
{
  XHdcp_Repeater *InstancePtr = (XHdcp_Repeater *)HdcpInstancePtr;

  /* Verify arguments */
  Xil_AssertVoid(HdcpInstancePtr != NULL);

  /* Assemble topology */
  XHdcp_AssembleTopology(InstancePtr);
}

/*****************************************************************************/
/**
*
* This function is called every time topology information is available for
* each downstream interface. On each call to this function it checks for
* available topology information for all connected downstream interfaces
* and assembles the aggregate topology table. After the topology for all
* the downstream interfaces have been resolved, the final call to this
* function passes the aggregate topology to the upstream interface for
* propagation. The assembled topology should have a DEVICE_COUNT that is
* the sum of all the downstream topology device counts, and a DEPTH that
* is the maximum of all the downstream topologies plus one. Conversion
* between HDCP 1.4 and HDCP 2.2 is handled by this function. Topology
* change and the unauthenticated event will trigger this function. The
* unauthenticated event is triggered when a downstream interface
* transitions from the authenticated to unauthenticated state. The
* function is registered with the HDCP 2.2 and HDCP 1.4 callbacks for
* the following events: topology available, and unauthenticated.
* When the downstream interfaces are connected to endpoint receivers
* the topology available event will also be triggered.
*
* @param    HdcpInstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
static void XHdcp_AssembleTopology(XHdcp_Repeater *InstancePtr)
{
  int Status;
  void *TopologyPtr;
  int DeviceCnt;
  int Depth;
  int HdcpProtocol;
  int DownstreamCnt = 0;
  XHdcp_Topology Topology;

  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  /* Clear topology */
  memset(&Topology, 0, sizeof(XHdcp_Topology));

  if (InstancePtr->IsReady) {
    HdcpProtocol = XV_HdmiRxSs_HdcpGetProtocol(InstancePtr->UpstreamInstancePtr);

    for (int i = 0; i < InstancePtr->DownstreamInstanceBinded; i++) {
      /* Check if downstream interface is active.
         If not then skip downstream interface */
      Status = XV_HdmiTxSs_HdcpIsEnabled(InstancePtr->DownstreamInstancePtr[i]);
      if (Status == FALSE) {
        continue;
      }

      /* Increment downstream inferface connected count */
      DownstreamCnt++;

      /* Check if downstream interface has topology information available.
         If not then downstream topology cannot be assembled. */
      TopologyPtr = XV_HdmiTxSs_HdcpGetTopology(InstancePtr->DownstreamInstancePtr[i]);
      if (TopologyPtr == NULL) {
        break;
      }

      /* Check for flags */
      Topology.MaxDevsExceeded |=
        XV_HdmiTxSs_HdcpGetTopologyField(InstancePtr->DownstreamInstancePtr[i],
          XV_HDMITXSS_HDCP_TOPOLOGY_MAXDEVSEXCEEDED);
      Topology.MaxCascadeExceeded |=
        XV_HdmiTxSs_HdcpGetTopologyField(InstancePtr->DownstreamInstancePtr[i],
          XV_HDMITXSS_HDCP_TOPOLOGY_MAXCASCADEEXCEEDED);
      Topology.Hdcp20RepeaterDownstream |=
        XV_HdmiTxSs_HdcpGetTopologyField(InstancePtr->DownstreamInstancePtr[i],
          XV_HDMITXSS_HDCP_TOPOLOGY_HDCP20REPEATERDOWNSTREAM);
      Topology.Hdcp1DeviceDownstream |=
        XV_HdmiTxSs_HdcpGetTopologyField(InstancePtr->DownstreamInstancePtr[i],
          XV_HDMITXSS_HDCP_TOPOLOGY_HDCP1DEVICEDOWNSTREAM);

      /* Get the downstream interface device count and depth */
      DeviceCnt = XV_HdmiTxSs_HdcpGetTopologyField(InstancePtr->DownstreamInstancePtr[i],
                    XV_HDMITXSS_HDCP_TOPOLOGY_DEVICECNT);
      Depth = XV_HdmiTxSs_HdcpGetTopologyField(InstancePtr->DownstreamInstancePtr[i],
                XV_HDMITXSS_HDCP_TOPOLOGY_DEPTH);

      /* Calculate device count by taking sum of all downstream interface device counts.
         Calculate depth by finding the maximum downstream depth and then adding one. */
      Topology.DeviceCnt += DeviceCnt;
      if (Depth > Topology.Depth) {
        Topology.Depth = Depth;
      }

      /* Check for topology maximums */
      switch (HdcpProtocol) {

        /* HDCP 2.2 */
        case XV_HDMIRXSS_HDCP_22:
          if (Topology.DeviceCnt > XHDCP_MAX_DEVICE_CNT_HDCP22) {
            Topology.MaxDevsExceeded = TRUE;
          }
          if (Topology.Depth > (XHDCP_MAX_DEPTH_HDCP22-1)) {
            Topology.MaxCascadeExceeded = TRUE;
          }
          break;

        /* HDCP 1.4 */
        case XV_HDMIRXSS_HDCP_14:
          if (Topology.DeviceCnt > XHDCP_MAX_DEVICE_CNT_HDCP14) {
            Topology.MaxDevsExceeded = TRUE;
          }
          if (Topology.Depth > (XHDCP_MAX_DEPTH_HDCP14-1)) {
            Topology.MaxCascadeExceeded = TRUE;
          }
          break;
      }

      /* Append to the device list */
      if (!(Topology.MaxDevsExceeded)) {
        memcpy(&Topology.DeviceList[Topology.DeviceCnt - DeviceCnt],
          XV_HdmiTxSs_HdcpGetTopologyReceiverIdList(InstancePtr->DownstreamInstancePtr[i]),
          5*DeviceCnt);
      }

      /* Propagate topology upstream on final call */
      if (DownstreamCnt == XHdcp_Flag2Count(InstancePtr->DownstreamInstanceConnected)) {
        Topology.Depth++;

        /* Set upstream topology information */
        XV_HdmiRxSs_HdcpSetTopologyField(InstancePtr->UpstreamInstancePtr,
          XV_HDMIRXSS_HDCP_TOPOLOGY_MAXDEVSEXCEEDED, Topology.MaxDevsExceeded);
        XV_HdmiRxSs_HdcpSetTopologyField(InstancePtr->UpstreamInstancePtr,
          XV_HDMIRXSS_HDCP_TOPOLOGY_MAXCASCADEEXCEEDED, Topology.MaxCascadeExceeded);
        XV_HdmiRxSs_HdcpSetTopologyField(InstancePtr->UpstreamInstancePtr,
          XV_HDMIRXSS_HDCP_TOPOLOGY_HDCP20REPEATERDOWNSTREAM, Topology.Hdcp20RepeaterDownstream);
        XV_HdmiRxSs_HdcpSetTopologyField(InstancePtr->UpstreamInstancePtr,
          XV_HDMIRXSS_HDCP_TOPOLOGY_HDCP1DEVICEDOWNSTREAM, Topology.Hdcp1DeviceDownstream);
        if (!(Topology.MaxDevsExceeded)) {
          if (Topology.DeviceCnt > 0) {
            XV_HdmiRxSs_HdcpSetTopologyField(InstancePtr->UpstreamInstancePtr,
              XV_HDMIRXSS_HDCP_TOPOLOGY_DEVICECNT, Topology.DeviceCnt);
          } else {
            xdbg_printf(XDBG_DEBUG_GENERAL, "Error: Attempted to trigger topology update with device count of zero.\n\r");
            break;
          }
        }
        if (!(Topology.MaxCascadeExceeded)) {
          XV_HdmiRxSs_HdcpSetTopologyField(InstancePtr->UpstreamInstancePtr,
            XV_HDMIRXSS_HDCP_TOPOLOGY_DEPTH, Topology.Depth);
        }
        if (!(Topology.MaxDevsExceeded) && !(Topology.MaxCascadeExceeded)) {
          XV_HdmiRxSs_HdcpSetTopologyReceiverIdList(InstancePtr->UpstreamInstancePtr,
            Topology.DeviceList[0], Topology.DeviceCnt);
        }

        /* Trigger topology update only when the topology has changed */
        if (memcmp(&InstancePtr->Topology, &Topology, sizeof(XHdcp_Topology)) != 0) {
          memcpy(&InstancePtr->Topology, &Topology, sizeof(XHdcp_Topology));
          XV_HdmiRxSs_HdcpSetTopologyUpdate(InstancePtr->UpstreamInstancePtr);
#ifdef DEBUG
          /* Display topology */
          XHdcp_DisplayTopology(InstancePtr, FALSE);
#endif
        }
      }
    }
  }
}

/*****************************************************************************/
/**
*
* This function displays the repeater topology.
*
* @param    InstancePtr is a pointer to the XHdcp_Repeater instance.
* @param    Verbose can be set to TRUE to display device list.
*
* @return   None.
*
* @note	    None.
*
******************************************************************************/
static void XHdcp_DisplayTopology(XHdcp_Repeater *InstancePtr, u8 Verbose)
{
  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  if (XV_HdmiRxSs_HdcpIsRepeater(InstancePtr->UpstreamInstancePtr)) {
    xil_printf("HDCP Topology : ");
    if (InstancePtr->Topology.MaxDevsExceeded)
      xil_printf("MaxDevsExceeded, ");
    if (InstancePtr->Topology.MaxCascadeExceeded)
      xil_printf("MaxCascadeExceeded, ");
    if (InstancePtr->Topology.Hdcp20RepeaterDownstream)
      xil_printf("Hdcp20RepeaterDownstream, ");
    if (InstancePtr->Topology.Hdcp1DeviceDownstream)
      xil_printf("Hdcp1DeviceDownstream, ");
    xil_printf("Depth=%d, ", InstancePtr->Topology.Depth);
    xil_printf("DeviceCnt=%d\n\r", InstancePtr->Topology.DeviceCnt);

    if (Verbose) {
      for (int i=0; i<InstancePtr->Topology.DeviceCnt; i++) {
        xil_printf("DeviceList[%i]=0x%02x%02x%02x%02x%02x\n\r", i,
          InstancePtr->Topology.DeviceList[i][0],
          InstancePtr->Topology.DeviceList[i][1],
          InstancePtr->Topology.DeviceList[i][2],
          InstancePtr->Topology.DeviceList[i][3],
          InstancePtr->Topology.DeviceList[i][4]);
      }
    }
  }
}

/*****************************************************************************/
/**
*
* This function enforces downstream content blocking based on the upstream
* encryption status and stream type information. When the content is required
* to be blocked, cipher output blanking is enabled.
*
* @param    HdcpInstancePtr is a pointer to the XHdcp_Repeater instance.
*
* @return   - XST_SUCCESS if blocking enforced successfully.
*           - XST_FAILURE if blocking could not be enforced.
*
* @note	    None.
*
******************************************************************************/
static void XHdcp_EnforceBlank(XHdcp_Repeater *InstancePtr)
{
  u8 IsEncrypted;
  int Status;

  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);

  /* Update encryption status */
  IsEncrypted = XV_HdmiRxSs_HdcpIsEncrypted(InstancePtr->UpstreamInstancePtr);

  /* Enforce downstream content blocking */
  for (int i = 0; (i < InstancePtr->DownstreamInstanceBinded); i++) {
    if (IsEncrypted) {
      if (XV_HdmiTxSs_HdcpIsAuthenticated(InstancePtr->DownstreamInstancePtr[i])) {
        /* Check the downstream interface protocol */
        Status = XV_HdmiTxSs_HdcpGetProtocol(InstancePtr->DownstreamInstancePtr[i]);

        switch (Status) {

          /* HDCP 2.2
             Allow content under the following conditons:
             - Stream type is 0
             - Stream type is 1, and no HDCP 1.x devices are downstream,
               and no HDCP 2.0 repeaters are downstream. */
          case XV_HDMITXSS_HDCP_22:
            Status = XV_HdmiTxSs_HdcpGetTopologyField(
                       InstancePtr->DownstreamInstancePtr[i],
                       XV_HDMITXSS_HDCP_TOPOLOGY_HDCP20REPEATERDOWNSTREAM);
            Status |= XV_HdmiTxSs_HdcpGetTopologyField(
                        InstancePtr->DownstreamInstancePtr[i],
                        XV_HDMITXSS_HDCP_TOPOLOGY_HDCP1DEVICEDOWNSTREAM);
            if ((InstancePtr->StreamType == XV_HDMITXSS_HDCP_STREAMTYPE_0) ||
                (Status == FALSE)) {
              XV_HdmiTxSs_HdcpDisableBlank(InstancePtr->DownstreamInstancePtr[i]);
            } else {
              Status = XV_HdmiTxSs_HdcpEnableBlank(InstancePtr->DownstreamInstancePtr[i]);
              if (Status != XST_SUCCESS) {
                xdbg_printf(XDBG_DEBUG_GENERAL, "Error: Problem blocking downstream content.\n\r");
              }
            }
            break;

          /* HDCP 1.4
             Allow content when the stream type is 0 */
          case XV_HDMITXSS_HDCP_14:
            if (InstancePtr->StreamType == XV_HDMITXSS_HDCP_STREAMTYPE_0) {
              XV_HdmiTxSs_HdcpDisableBlank(InstancePtr->DownstreamInstancePtr[i]);
            } else {
              Status = XV_HdmiTxSs_HdcpEnableBlank(InstancePtr->DownstreamInstancePtr[i]);
              if (Status != XST_SUCCESS) {
                xdbg_printf(XDBG_DEBUG_GENERAL, "Error: Problem blocking downstream content.\n\r");
              }
            }
            break;
        }
      } else {
        Status = XV_HdmiTxSs_HdcpEnableBlank(InstancePtr->DownstreamInstancePtr[i]);
        if (Status != XST_SUCCESS) {
          xdbg_printf(XDBG_DEBUG_GENERAL, "Error: Problem blocking downstream content.\n\r");
        }
      }
    } else {
      XV_HdmiTxSs_HdcpDisableBlank(InstancePtr->DownstreamInstancePtr[i]);
    }
  }
}

/*****************************************************************************/
/**
*
* This function counts the number of bits set in the input flag bitmap.
*
* @param    Flag is an unsigned 32 bit value.
*
* @return   The number of bits set in the flag.
*
* @note	    None.
*
******************************************************************************/
static int XHdcp_Flag2Count(u32 Flag)
{
  int Count = 0;

  /* Check if bit is set and increment count */
  for (int i=0; i<32; i++) {
    if ((Flag & (0x1 << i))) {
      Count++;
    }
  }

  return Count;
}

#endif // USE_HDCP
