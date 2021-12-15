//*****************************************************************************
//! \file       cc1310_Sync.h
//! \brief      System Timer of CC3235S synchronize with CC1310 RAT
//! \version    V0.0.1
//! \date       12/15/2021
//! \author     gjmsilly
//! \copy
//!
//! Copyright (c)  2021, gjmsilly
//! All rights reserved.
//*****************************************************************************

#ifndef __CC1310_SYNC_H
#define __CC1310_SYNC_H

/* Driver Header files */
#include <ti/drivers/I2C.h>
#include <ti/drivers/Timer.h>

//*****************************************************************************
//
//! \brief Initializate the hardware, create 1s SyncTimer.
//!
//! \param None
//!
//! \return Timer_Handle
//
//*****************************************************************************
Timer_Handle Sync_Init();

#endif /* __CC1310_SYNC_H */
