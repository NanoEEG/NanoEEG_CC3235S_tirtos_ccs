//*****************************************************************************
//! \file       cc1310_Sync.c
//! \brief      System Timer of CC3235S synchronize with CC1310 RAT
//! \version    V0.0.1
//! \date       12/15/2021
//! \author     gjmsilly
//! \copy
//!
//! Copyright (c)  2021, gjmsilly
//! All rights reserved.
//*****************************************************************************

/* TI-DRIVERS Header files */
#include "ti_drivers_config.h"

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>

/* User defined Header files */
#include "cc1310_Sync.h"


static void SyncOutput(Timer_Handle handle, int_fast16_t status);

//*****************************************************************************
//
//! \brief Initializate the hardware, create 1s SyncTimer.
//!
//! \param None
//!
//! \return Timer_Handle
//
//*****************************************************************************
Timer_Handle Sync_Init()
{
    Timer_Handle    handle;
    Timer_Params    params;

    // Initialize Timer parameters
    Timer_Params_init(&params);
    params.periodUnits = Timer_PERIOD_US;
    params.period = 1000000; // 1s
    params.timerMode  = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = SyncOutput;
    // Open Timer instance
    handle = Timer_open(Sync_Timer, &params);

    return handle;
}


//*****************************************************************************
//
//! \brief  This function is called when Sync_Timer overflows.
//!         In this design, we just toggle the CC1310_Sync_PWM.
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void SyncOutput(Timer_Handle handle, int_fast16_t status)
{

    GPIO_toggle(CC1310_Sync_PWM); // CC1310 RAT should set both edge as input
}

