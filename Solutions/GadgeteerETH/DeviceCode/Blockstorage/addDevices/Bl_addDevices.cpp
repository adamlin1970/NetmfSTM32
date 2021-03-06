////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
//
//  Copyright (c) Microsoft Corporation. All rights reserved.
//  Implementation for the GadgeteerETH board (STM32F2): Copyright (c) Oberon microsystems, Inc.
//
//  *** GadgeteerETH Block Storage AddDevice Configuration ***
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <tinyhal.h>


extern struct BlockStorageDevice  g_STM32F2_BS;
extern struct IBlockStorageDevice g_STM32F2_Flash_DeviceTable;
extern struct BLOCK_CONFIG        g_STM32F2_BS_Config;


void BlockStorage_AddDevices() {
    BlockStorageList::AddDevice( &g_STM32F2_BS,
                                 &g_STM32F2_Flash_DeviceTable,
                                 &g_STM32F2_BS_Config, FALSE );
}

