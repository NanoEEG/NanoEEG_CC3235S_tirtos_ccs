#ifndef __PROTOCOL_WIFI_H__
#define __PROTOCOL_WIFI_H__

/*********************************************************************
 * INCLUDES
 */
#include <string.h>
#include <stdint.h>
#include <service/NanoEEG_misc.h>

/*********************************************************************
 * Macros
 */
/* TCP����ͨ������ */

// ��λ��->�豸 ָ�����
#define TCP_Recv_FH                 0xAC    //!< TCP����֡ͷ
#define TCP_Recv_FT                 0xCC    //!< TCP����֡β
// �豸->��λ�� �ظ�
#define TCP_Send_FH                 0xA2    //!< TCP����֡ͷ
#define TCP_Send_FT                 0xC2    //!< TCP����֡β
// ָ����
#define DummyIns                    0x00    //!< ��ָ��
#define CAttr_Read                  0x01    //!< ��һ����ͨ����
#define CAttr_Write                 0x10    //!< дһ����ͨ����
#define ChxAttr_Read                0x02    //!< ��һ��ͨ������
#define ChxAttr_Write               0x20    //!< дһ��ͨ������

// ͨѶ�շ�����������
#define TCP_Rx_Buff_Size            16
#define TCP_Tx_Buff_Size            16

/* UDP����ͨ������*/
#define UDP_SAMPLE_FH               0x23    //!< UDP֡������ ����ʼ�ָ���
#define UDP_SAMPLENUM               10      //!< UDPÿ����ad������

// ͨѶ���ͻ���������
#ifdef Dev_Ch32
#define UDP_Tx_Buff_Size            1173    //!< ����֡ͷ��23 + ������10 x��������ͷ��7 + (����ͨ��״̬3 + ��ͨ��8 x ÿͨ�������ֽ���3��x ͨ������4)�ֽ�
#define UDP_SampleValSize           108     //!< ͨ������4 - 108�ֽ�״̬+����ֵ
#endif
#ifdef Dev_Ch24
#define UDP_Tx_Buff_Size            903     //!< ����֡ͷ��23 + ������10 x��������ͷ��7 + (����ͨ��״̬3 + ��ͨ��8 x ÿͨ�������ֽ���3��x ͨ������3)�ֽ�
#define UDP_SampleValSize           81      //!< ͨ������3 - 81�ֽ�״̬+����ֵ
#endif
#ifdef Dev_Ch16
#define UDP_Tx_Buff_Size            633     //!< ����֡ͷ��23 + ������10 x��������ͷ��7 + (����ͨ��״̬3 + ��ͨ��8 x ÿͨ�������ֽ���3��x ͨ������2)�ֽ�
#define UDP_SampleValSize           54      //!< ͨ������2 - 51�ֽ�״̬+����ֵ
#endif
#ifdef Dev_Ch8
#define UDP_Tx_Buff_Size            363     //!< ����֡ͷ��23 + ������10 x��������ͷ��7 + (����ͨ��״̬3 + ��ͨ��8 x ÿͨ�������ֽ���3��x ͨ������1)�ֽ�
#define UDP_SampleValSize           27      //!< ͨ������1 - 27�ֽ�״̬+����ֵ
#endif

/*******************************************************************
 * TYPEDEFS
 */

/*!
 *  @def    ���� �����Իص�����ԭ��
 *  @param  InsAttrNum - �������Ա��
 *          CHxNum - ͨ����ţ�ͨ������ר�ã�Ĭ�ϲ���   0xFF��
 *          pValue - ����ֵ ��to be returned��
 *          pLen - ����ֵ��С��to be returned��
 */
typedef uint8_t (*pfnReadAttrCB_t)( uint8_t InsAttrNum, uint8_t CHxNum,
                                    uint8_t *pValue, uint8_t *pLen );

/*!
 *  @def    ���� д���Իص�����ԭ��
 *  @param  InsAttrNum - ��д�����Ա��
 *          CHxNum - ͨ����ţ�ͨ������ר�ã�Ĭ�ϲ���   0xFF��
 *          pValue - ��д�����ݵ�ָ��
 *          pLen - ��д�����ݴ�С
 */
typedef uint8_t (*pfnWriteAttrCB_t)( uint8_t InsAttrNum, uint8_t CHxNum,
                                     uint8_t *pValue, uint8_t len );

/*!
 *  @def    ���Զ�д�ص����� �ṹ��
 */
typedef struct
{
  pfnReadAttrCB_t   pfnReadAttrCB;                  //!< �����Իص�����ָ��
  pfnWriteAttrCB_t  pfnWriteAttrCB;                 //!< д���Իص�����ָ��
} AttrCBs_t;

/*!
 *  @def    TCP����ͨ��֡ �ṹ��
 *
 *  @brief  ���ṹ�嶨��TCP����ͨ��һ��֡�շ�������
 */
 typedef struct
{
    uint8_t FrameHeader;    //!< ֡ͷ
    uint8_t FrameLength;    //!< ��Ч֡��
    uint8_t DataLength;     //!< ����֡��
    uint8_t InsNum;         //!< ָ����
    uint8_t InsAttrNum;     //!< ָ���������Ա��
    uint8_t ChxNum;         //!< ָ������ͨ��
    uint8_t ERR_NUM;        //!< ������
    uint8_t *pDataLength;   //!< ����֡��ָ�루���ص��ã�
    uint8_t *_OP_;          //!< ����������ָ�루д�ص��ã�
} TCPFrame_t;

/*!
 *  @def    UDP֡ͷ���� �ṹ��
 *
 *  @brief  �����ṹ����������ͨ�����Ա��ȡ
 */
 typedef struct
{
     uint8_t  DevID[4];             //!< �豸ID
     uint8_t  UDPNum[4];            //!< UDP���ۼӹ�����
     uint8_t  UDPSampleNum[2];      //!< ��UDP��������
     uint8_t  UDP_ChannelNum;       //!< ��UDP����Чͨ������
     uint8_t  UNIXTimeStamp[8];     //!< Unixʱ�����δ�ã�
     uint8_t  ReservedNum[4];       //!< ������
} UDPHeader_t;

/*!
 *  @def    UDP����ͨ�� ����֡������ṹ�� - һ��ͨ���飨8ͨ����
 *
 *  @brief  ���ṹ�嶨��UDP����ͨ�� ����֡�������ʽ
 */
typedef struct
{
    uint8_t     FrameHeader;                        //!< ��ʼ�ָ���
    uint8_t     Index[2];                           //!< �������
    uint8_t     Timestamp[4];                       //!< ������ʱ���
    uint8_t     ChannelVal[UDP_SampleValSize];      //!< 8ͨ��״̬+����ֵ
}UDPData_t;

/*!
 *  @def    UDP����ͨ�����ͻ����� ������
 *
 *  @brief  �������嶨��UDP����ͨ���ķ��ͻ�����֡��ʽ
 */
typedef //union
//{
   //uint8_t UDP_Tx_Buff[UDP_Tx_Buff_Size];
   struct
   {
       /* ����֡ͷ��     - 23�ֽ�*/
       UDPHeader_t sampleheader;

       /*����֡������  */
       UDPData_t sampledata[UDP_SAMPLENUM];

   //} UDPframe;
} UDPFrame_t;

/**********************************************************************
 * FUNCTIONS
 */
void TCP_ProcessFSMInit(void);
bool TCP_ProcessFSM(uint8_t *pdata);

bool UDP_DataGet(uint8_t SampleIndex,uint8_t Procesflag);
bool UDP_DataProcess(SampleTime_t *pSampleTime,uint8_t Procesflag);

bool protocol_RegisterAttrCBs(AttrCBs_t *pAttrcallbacks);

#endif  /* __PROTOCOL_WIFI_H__ */
