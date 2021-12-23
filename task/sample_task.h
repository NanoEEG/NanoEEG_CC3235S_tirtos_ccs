/**
 * @file    sample_task.c
 * @author  gjmsilly
 * @brief   NanoEEG 采样线程 由控制线程创建的线程，用以处理AD采样
 * @version 1.0.0
 * @date    2021-12-23
 *
 * @copyright (c) 2021 gjmsilly
 *
 */

#ifndef TASK_SAMPLE_TASK_H_
#define TASK_SAMPLE_TASK_H_

/*******************************************************************
 * CONSTANTS
 */

/* 采集状态指示 */
#define EEG_DATA_START_EVT              ( 1 << 0 )  //!< 一包AD数据开始采集
#define EEG_DATA_ACQ_EVT                ( 1 << 1 )  //!< 一包AD数据采集中
#define EEG_DATA_CPL_EVT                ( 1 << 2 )  //!< 一包AD数据采集完成
#define EEG_STOP_EVT                    ( 1 << 3 )  //!< AD数据暂停采集



#endif /* TASK_SAMPLE_TASK_H_ */
