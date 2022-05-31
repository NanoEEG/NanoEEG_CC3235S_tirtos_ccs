/**
 * @file    ads1299.c
 * @author  gjmsilly
 * @brief   ads1299 Firmware for CC3235S
 * @version 1.0.0
 * @date    2021-01-26
 *
 * @copyright (c) 2020 gjmsilly
 *
 */

/*********************************************************************
 * INCLUDES
 */

/* TI-DRIVERS Header files */
#include <ti/drivers/SPI.h>
#include <ti/drivers/GPIO.h>

#include <ti/devices/cc32xx/driverlib/utils.h>

#include "ti_drivers_config.h"

#include "ads1299.h"

/*********************************************************************
 * GLOBAL VARIABLES
 */
static uint8_t  DummyByte=0x00;
SPI_Handle      masterSpi;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void ADS1299_Reset(uint8_t dev);
static void ADS1299_PowerOn(uint8_t dev);
static void WaitUs(unsigned long  iWaitUs);
static void ADS1299_SendCommand(uint8_t command);
static void ADS1299_WriteREG(uint8_t dev, uint8_t address, uint8_t value);
static uint8_t ADS1299_ReadREG(uint8_t dev, uint8_t address);

/****************************************************************/
/*  WaitUs                                                      */
/** Operation:
 *      - cpu delay in us The loop takes 3 cycles/loop.
 *
 * Parameters:
 *      -  iWaitUs: delay time in us
 *
 * Return value:
 *     - None
 *
 * Globals modified:
 *     - None
 *
 * Resources used:
 *     - None
 */
/****************************************************************/
static void WaitUs(unsigned long iWaitUs)
{
    // delay 1us means 1*80/3 `= 27
    UtilsDelay(27*iWaitUs);
}

/****************************************************************/
/*  ADS1299_Reset                                               */
/** Operation:
 *      - Reset ADS1299 chip
 *
 * Parameters:
 *      -  dev:ADS1299 chip number
 *
 * Return value:
 *     - None
 *
 * Globals modified:
 *     - None
 *
 * Resources used:
 *     - None
 */
/****************************************************************/
static void ADS1299_Reset(uint8_t dev)
{
    Mod_RESET_L;
    WaitUs(6);
    Mod_RESET_H;
    Mod_CS_Disable;
    WaitUs(200);        // wait for 18 tclk then start using device

}

/****************************************************************/
/*  ADS1299_PowerOn                                             */
/** Operation:
 *      - PowerOn ADS1299 chip
 *
 * Parameters:
 *      -  dev:ADS1299 chip number
 *
 * Return value:
 *     - None
 *
 * Globals modified:
 *     - None
 *
 * Resources used:
 *     - None
 */
/****************************************************************/
static void ADS1299_PowerOn(uint8_t dev)
{
    Mod_PDWN_H
    Mod_RESET_H

    // wait for at least tPOR = 128ms
    WaitUs(200);
}


/****************************************************************/
/*  ADS1299_SendCommand                                         */
/** Operation:
 *      - Send command to the ADS1299 chip
 *
 * Parameters:
 *      - dev:ADS1299 chip number
 *      - command:command to the ADS1299 chip
 *
 * Return value:
 *
 * Globals modified:
 *     - None
 *
 * Resources used:
 *     - None
 */
/****************************************************************/
void ADS1299_SendCommand(uint8_t command)
{
    uint8_t         transmitBuffer = command;
    uint8_t         receiveBuffer;
    SPI_Transaction transaction;
    bool            transferOK;


    Mod_CS_Enable
    WaitUs(6);

    transaction.count = 1;
    transaction.txBuf = &transmitBuffer;
    transaction.rxBuf = &receiveBuffer;

    transferOK = SPI_transfer(masterSpi, &transaction);
    if (!transferOK)
    {
      //  while(1);               // error
    }

    WaitUs(10);                 // Delay time, final SCLK falling edge to CS high
    Mod_CS_Disable
    WaitUs(10);                 // Pulse duration, CS high
}

/****************************************************************/
/*  ADS1299_WriteREG                                            */
/** Operation:
 *      - Configuring the ADS1299 register
 *
 * Parameters:
 *      - dev:ADS1299 chip number
 *      - address:Destination register address
 *      - value:The value of destination register
 *
 * Return value:
 *     - None
 *
 * Globals modified:
 *     - None
 *
 * Resources used:
 *     - None
 */
/****************************************************************/
void ADS1299_WriteREG (uint8_t dev, uint8_t address, uint8_t value)
{
    address += 0x40;
    uint8_t         i;
    uint8_t         transmitBuffer[3] = {address,0x00,value}; // address / reg number-1 =0x00 / value
    SPI_Transaction transaction;
    bool            transferOK;

    Mod_CS_Enable

    for (i= 0; i < 3; i++)
    {
        transaction.count = 1;
        transaction.txBuf = (void *)(transmitBuffer+i);
        transaction.rxBuf = (void *)NULL;

        transferOK = SPI_transfer(masterSpi, &transaction);
        if (transferOK)
            WaitUs(10);
    }

    Mod_CS_Disable
}

/****************************************************************/
/*  ADS1299_ReadREG                                             */
/** Operation:
 *      - Configuring the ADS1299 register
 *
 * Parameters:
 *      - dev:ADS1299 chip number
 *      - address:Destination register address
 *
 * Return value:
 *      - value:The value of destination register
 *
 * Globals modified:
 *     - None
 *
 * Resources used:
 *     - None
 */
/****************************************************************/
uint8_t ADS1299_ReadREG (uint8_t dev, uint8_t address)
{
    address += 0x20;
    uint8_t         i;
    uint8_t         transmitBuffer[3] = {address,0x00,DummyByte}; // address / reg number-1 =0x00 / DummyByte=0xaa
    uint8_t         receiveBuffer[3];
    SPI_Transaction transaction;
    bool            transferOK;

    Mod_CS_Enable

    for (i= 0; i < 3; i++)
    {
        transaction.count = 1;
        transaction.txBuf = (void *)(transmitBuffer+i);
        transaction.rxBuf = (void *)(receiveBuffer+i);

        transferOK = SPI_transfer(masterSpi, &transaction);
        if (transferOK)
            WaitUs(10);
    }

    Mod_CS_Disable

  return receiveBuffer[2];
}

/*********************************************************************
 * FUNCTIONS
 */

/****************************************************************/
/*  ADS1299_init                                                */
/** Operation:
 *      - initial the SPI module and DReady interrupts
 *
 * Parameters:
 *      - None
 *
 * Return value:
 *     - None
 *
 * Globals modified:
 *     - None
 *
 * Resources used:
 *     - None
 */
/****************************************************************/
void ADS1299_Init(uint8_t dev)
{
    SPI_Params      spiParams;

    /* Open SPI as master (default) */
    SPI_Params_init(&spiParams);
    spiParams.dataSize = 8;
    spiParams.frameFormat = SPI_POL0_PHA1;
    spiParams.bitRate = 10000000; //!< 10MHz

    masterSpi = SPI_open(CONFIG_SPI_0, &spiParams);
    if (masterSpi == NULL) {
        while (1);
    }

    /* Initial the ads1299 */
    Mod_DRDY_INT_Disable

    ADS1299_PowerOn(dev);
    ADS1299_Reset(dev);


    GPIO_write(Mod_START, 1);
    while(GPIO_read(Mod_nDRDY)); // wait until nready
    GPIO_write(Mod_START, 0);
    GPIO_clearInt(Mod_nDRDY);

    ADS1299_Reset(0);
    ADS1299_SendCommand(ADS1299_CMD_RESET);
    WaitUs(10);
    ADS1299_SendCommand(ADS1299_CMD_SDATAC);

}

/****************************************************************/
/*  ADS1299_Channel_Config                                      */
/** Operation:
 *      - Configuring ADS1299 parameters
 *
 * Parameters:
 *      - dev: ADS1299 chip number
 *      - channel : ADS1299 channel number
 *      - Para : CHnSET value
 * Return value:
 *
 * Globals modified:
 *     - None
 *
 * Resources used:
 *     - None
 */
/****************************************************************/
void ADS1299_Channel_Config(uint8_t dev, uint8_t channel, TADS1299CHnSET Para)
{
	ADS1299_WriteREG (0, (ADS1299_REG_CH1SET + channel), Para.value );
}

/****************************************************************/
/*  ADS1299_Parameter_Config                                    */
/** Operation:
 *      - Configuring ADS1299 parameters
 *
 * Parameters:
 *      - div:ADS1299 chip number
 *      - gain:The gain of ADS1299
 *      - sample:The sampling rate of ADS1299
 *
 * Return value:
 *
 * Globals modified:
 *     - None
 *
 * Resources used:
 *     - None
 */
/****************************************************************/
void ADS1299_Parameter_Config(uint8_t ADS1299_ParaGroup, uint8_t sample,uint8_t gain)
{
    uint8_t i = 0;
    TADS1299CHnSET     ChVal;
    TADS1299CONFIG1    CFG1;
    TADS1299CONFIG2    CFG2;
    TADS1299CONFIG3    CFG3;
    TADS1299CONFIG4    CFG4;
    TADS1299LOFF       LOFF;
    TADS1299BIASSENSP  BIASSP;
    TADS1299MISC1      MISC1;
    ChVal.control_bit.pd = 0;
    ChVal.control_bit.gain = gain;   // Gain = 24x


    switch (ADS1299_ParaGroup)
  {
    case ADS1299_ParaGroup_ACQ:
    {
            CFG1.control_bit.dr = 6;    //Sample rate 250Hz
            CFG1.control_bit.res7 = 1;
            CFG1.control_bit.rsv4 = 2;

            CFG2.value = 0xC0;

            CFG3.value = 0x60;
            CFG3.control_bit.pdbrefbuf = 1;
            CFG3.control_bit.pdbbias = 1;
            CFG3.control_bit.biasrefint = 1;

            CFG4.value = 0x00;

            LOFF.value = 0x00;


            ChVal.control_bit.gain = 6;
            ChVal.control_bit.pd = 0;
            ChVal.control_bit.mux = 0;


            BIASSP.value = 0xFF;
            MISC1.control_bit.srb1 = 1;

            break;
        }
    case ADS1299_ParaGroup_IMP:
        break;
        case ADS1299_ParaGroup_STBY:
        break;
        case ADS1299_ParaGroup_TSIG:
    {
            CFG1.control_bit.dr = 6;    //Sample rate 250Hz
            CFG1.control_bit.res7 = 1;
            CFG1.control_bit.rsv4 = 2;

            CFG2.value = 0xC0;
            CFG2.control_bit.inttest = 1;
            CFG2.control_bit.testamp = 0;
            CFG2.control_bit.testfreq = 1;

            CFG3.value = 0x60;
            CFG3.control_bit.pdbrefbuf = 1;
            CFG3.control_bit.pdbbias = 1;
            CFG3.control_bit.biasrefint = 1;

            CFG4.value = 0x00;

            LOFF.value = 0x00;


            ChVal.control_bit.gain = 1;
            ChVal.control_bit.pd = 0;
            ChVal.control_bit.mux = 5;

            BIASSP.value = 0xFF;
            MISC1.control_bit.srb1 = 1;

            break;
        }
    default:
        break;
  }



    switch(sample)
    {
        case 1:     //250Hz
        {
            ADS1299_WriteREG(0,ADS1299_REG_CONFIG1,0xF6);
            break;
        }
        case 2:
            //500Hz
        {
            ADS1299_WriteREG(0,ADS1299_REG_CONFIG1,0xF5);
            break;
        }
        case 3:     //1000Hz
        {
            ADS1299_WriteREG(0,ADS1299_REG_CONFIG1,0xF4);
            break;
        }

        default:
            break;
    }
    WaitUs(2);


    ADS1299_WriteREG(0,ADS1299_REG_BIASSENSP,0xFF);
    WaitUs(2);
    ADS1299_WriteREG(0,ADS1299_REG_BIASSENSN,0xFF);
    WaitUs(2);
    ADS1299_WriteREG(0,ADS1299_REG_MISC1,0x20);     // SRB1统一锟轿匡拷
    WaitUs(2);
    ADS1299_WriteREG(0,ADS1299_REG_LOFF,0x00);
    WaitUs(2);
    ADS1299_WriteREG(0,ADS1299_REG_LOFFSENSP,0x00);
    WaitUs(2);
    ADS1299_WriteREG(0,ADS1299_REG_LOFFSENSN,0x00);
    WaitUs(2);
    ADS1299_WriteREG(0,ADS1299_REG_BIASSENSP,0xFF);
    WaitUs(2);
    ADS1299_WriteREG(0,ADS1299_REG_BIASSENSN,0xFF);
    WaitUs(2);


    for(i=0;i<8;i++)
    {
        ADS1299_Channel_Config(0,i,ChVal);
        WaitUs(2);
    }


}
/****************************************************************/
/*  ADS1299_Mode_Config()                                       */
/** Operation:
 *      - Configuring ADS1299 Mode Parameters
 *
 * Mode:
 *      - EEG_Acq
 *      - IMP_Meas
 *
 * Return value:
 *      - 0 Configuration Done
 *      - 1 Configuration Failed
 *
 * Globals modified:
 *     - None
 *
 * Resources used:
 *     - None
 */
/****************************************************************/
void ADS1299_Mode_Config(uint8_t Mode)
{
    uint8_t i;
    uint8_t ReadResult;

    switch (Mode)
  {
        case EEG_ACQ://EEG_Acq
        {
            ADS1299_SetSamplerate(0,1000); // samplerate
            ADS1299_WriteREG(0,ADS1299_REG_CONFIG2,0xC0);
            ADS1299_WriteREG(0,ADS1299_REG_CONFIG3,0xEC);


            ADS1299_WriteREG(0,ADS1299_REG_LOFF,0x00);
            ADS1299_WriteREG(0,ADS1299_REG_LOFFSENSN,0x00);
            ADS1299_WriteREG(0,ADS1299_REG_LOFFSENSP,0x00);
            ADS1299_WriteREG(0,ADS1299_REG_BIASSENSN,0x00);
            ADS1299_WriteREG(0,ADS1299_REG_BIASSENSP,0xFF);
            do
            {
                ADS1299_WriteREG(0,ADS1299_REG_MISC1,0x20);

                ReadResult = ADS1299_ReadREG(0,ADS1299_REG_MISC1);


            } while(ReadResult!=0x20);

            ADS1299_SetGain(0,24); // gain

            break;
        }

        case IMP_MEAS://IMP_Meas
        {
            ADS1299_WriteREG(1,ADS1299_REG_LOFF,0x09);              //[3:2]=00(6nA),01(24nA),10(6uA),11(24uA); [1:0]=01(7.8Hz),10(31.2Hz)
            ADS1299_WriteREG(1,ADS1299_REG_LOFFSENSN,0xFF);
            ADS1299_WriteREG(1,ADS1299_REG_LOFFSENSP,0xFF);
            ADS1299_WriteREG(1,ADS1299_REG_BIASSENSN,0x00);
            ADS1299_WriteREG(1,ADS1299_REG_BIASSENSP,0x00);


            for(i=0;i<8;i++)
            {
                // Gain = 1
                ADS1299_WriteREG(0,ADS1299_REG_CH1SET+i,0x00);
                WaitUs(2);
                WaitUs(10);
            }

            break;
        }

  }
}
/****************************************************************/
/*  ADS1299_ReadResult                                          */
/** Operation:
 *      - Read ADS1299 output data by DMA
 *
 * Parameters:
 *      - result:point to the buffer to store result
 *
 * Return value:
 *
 * Globals modified:
 *     - None
 *
 * Resources used:
 *     - None
 */
/****************************************************************/
void ADS1299_ReadResult(uint8_t *result)
{
    SPI_Transaction transaction;
    bool            transferOK;

    //DMA
    #ifdef Dev_Ch32
    transaction.count = 108;
    #endif
    #ifdef Dev_Ch24
    transaction.count = 81;
    #endif
    #ifdef Dev_Ch16
    transaction.count = 54;
    #endif
    #ifdef Dev_Ch8
    transaction.count = 27;
    #endif

    transaction.txBuf = (void *)NULL;
    transaction.rxBuf = (void *)result;

    transferOK = SPI_transfer(masterSpi, &transaction);
    if (!transferOK) 
	{
		while(1);
	}
}

/****************************************************************/
/*  ADS1299_Sampling_Control                                    */
/** Operation:
 *      - Control the ads1299 module sampling state
 *
 * Parameters:
 *      - Sampling:the sampling state need to set
 *
 * Globals modified:
 *     - None
 *
 * Resources used:
 *     - None
 */
/****************************************************************/
void ADS1299_Sampling_Control(uint8_t Sampling)
{
    switch(Sampling)
    {
        case 0:
            ADS1299_SendCommand(ADS1299_CMD_STOP);
            ADS1299_SendCommand(ADS1299_CMD_SDATAC);;
            //Mod_DRDY_INT_Disable
            GPIO_clearInt(Mod_nDRDY);
            Mod_CS_Disable;
        break;

        case 1:
            ADS1299_SendCommand(ADS1299_CMD_START);
            ADS1299_SendCommand(ADS1299_CMD_RDATAC);
            Mod_DRDY_INT_Enable
            Mod_CS_Enable;
        break;

    }
}

/****************************************************************/
/*  ADS1299_SetSamplerate                                       */
/** Operation:
 *      - Set the ads1299 module sample rate
 *
 * Parameters:
 *      - dev: dev to set //TODO
 *      - Samplerate:the sampling rate need to set
 *
 * Globals modified:
 *     - None
 *
 * Resources used:
 *     - None
 */
/****************************************************************/
bool ADS1299_SetSamplerate(uint8_t dev, uint16_t Samplerate){

    bool ret = true;
    uint8_t valset = 0;
    uint8_t valget = 0;

    switch(Samplerate)
    {
        case 250:
            valset = 0x96;
        break;

        case 500:
            valset = 0x95;
        break;

        case 1000:
            valset = 0x94;
        break;

        case 2000:
            valset = 0x93;
        break;

        default:
            valset = 0x94; //default 1kHz
        break;
    }

    /* 尝试配置 */
    ADS1299_WriteREG(0,ADS1299_REG_CONFIG1,valset);
    /* 回读一次 */
    valget = ADS1299_ReadREG(0,ADS1299_REG_CONFIG1);

    if(valget!=valset)
        ret = false;

    return ret;

}

/****************************************************************/
/*  ADS1299_SetGain                                             */
/** Operation:
 *      - Set the ads1299 module gain
 *
 * Parameters:
 *      - dev: dev to set //TODO
 *      - gain:the gain need to set
 *
 * Globals modified:
 *     - None
 *
 * Resources used:
 *     - None
 */
/****************************************************************/
bool ADS1299_SetGain(uint8_t dev, uint8_t gain){

    uint8_t valget,i;
    TADS1299CHnSET ChVal;

    switch(gain)
     {
         case 1:
             ChVal.control_bit.gain = 0;
         break;

         case 2:
             ChVal.control_bit.gain = 1;
         break;

         case 4:
             ChVal.control_bit.gain = 2;
         break;

         case 6:
             ChVal.control_bit.gain = 3;
         break;

         case 8:
             ChVal.control_bit.gain = 4;
         break;

         case 12:
             ChVal.control_bit.gain = 5;
         break;

         case 24:
             ChVal.control_bit.gain = 6;
         break;

         default:
             ChVal.control_bit.gain = 6; //default x24
         break;
     }
     ChVal.control_bit.pd = 0;
     ChVal.control_bit.mux = 0;
     ChVal.control_bit.srb2 = 0;

     for(i=0;i<8;i++)
     {
         /* 尝试配置 */
         ADS1299_Channel_Config(0,i,ChVal);
         /* 回读一次 */
         valget = ADS1299_ReadREG(0,ADS1299_REG_CH1SET+i);

         if(valget!=ChVal.value) return false;
     }

     return true;
}


