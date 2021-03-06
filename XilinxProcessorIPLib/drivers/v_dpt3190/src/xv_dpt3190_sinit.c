// ==============================================================
// File generated by Vivado(TM) HLS - High-Level Synthesis from C, C++ and SystemC
// Version: 2016.1
// Copyright (C) 1986-2016 Xilinx, Inc. All Rights Reserved.
//
// ==============================================================

#ifndef __linux__

#include "xstatus.h"
#include "xparameters.h"
#include "xv_dpt3190.h"

extern XV_dpt3190_Config XV_dpt3190_ConfigTable[];

XV_dpt3190_Config *XV_dpt3190_LookupConfig(u16 DeviceId) {
    XV_dpt3190_Config *ConfigPtr = NULL;

    int Index;

    for (Index = 0; Index < XPAR_XV_DPT3190_NUM_INSTANCES; Index++) {
        if (XV_dpt3190_ConfigTable[Index].DeviceId == DeviceId) {
            ConfigPtr = &XV_dpt3190_ConfigTable[Index];
            break;
        }
    }

    return ConfigPtr;
}

int XV_dpt3190_Initialize(XV_dpt3190 *InstancePtr, u16 DeviceId) {
    XV_dpt3190_Config *ConfigPtr;

    Xil_AssertNonvoid(InstancePtr != NULL);

    ConfigPtr = XV_dpt3190_LookupConfig(DeviceId);
    if (ConfigPtr == NULL) {
        InstancePtr->IsReady = 0;
        return (XST_DEVICE_NOT_FOUND);
    }

    return XV_dpt3190_CfgInitialize(InstancePtr, ConfigPtr);
}

#endif
