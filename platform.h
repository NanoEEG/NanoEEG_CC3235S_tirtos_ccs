#ifndef __PLATFORM_H
#define __PLATFORM_H

/*******************************************************************
 * CONSTANTS
 */
#define APPLICATION_NAME                      ("NanoEEG")
#define APPLICATION_VERSION                   ("1.0.0")
#define DEVICE_ERROR                          ("Device error, please refer \"DEVICE ERRORS CODES\" section in errors.h")
#define WLAN_ERROR                            ("WLAN error, please refer \"WLAN ERRORS CODES\" section in errors.h")
#define SL_STOP_TIMEOUT                       (200)

#define SPAWN_TASK_PRIORITY                   (9)
#define SAMPLE_TASK_PRIORITY                  (5)
#define CONTROL_TASK_PRIORITY                 (2)
#define TCP_WORKER_PRIORITY                   (4)
#define SOCKET_TASK_PRIORITY                  (1)
#define UDP_TASK_STACK_SIZE                   (1024)
#define CONTROL_STACK_SIZE                    (1024)
#define SAMPLE_STACK_SIZE                     (1024)
#define SYNC_STACK_SIZE                       (512)
#define TASK_STACK_SIZE                       (4096)
#define SLNET_IF_WIFI_PRIO                    (5)
#define SLNET_IF_WIFI_NAME                    "CC3235S"

//!< Router Param
#define SSID_NAME                             "NanoEEG"              /* AP SSID */
#define SECURITY_TYPE                         SL_WLAN_SEC_TYPE_WPA_WPA2 /* Security type could be SL_WLAN_SEC_TYPE_OPEN */
#define SECURITY_KEY                          "TUNERL2022"              /* Password of the secured AP */

//!< Socket Param
#define TCPPORT                               (7001)                    
#define UDP1PORT                              (7002)    // for eeg data                     
#define UDP2PORT                              (7003)    // for event data                    

/*******************************************************************
 * TYPEDEFS
 */
typedef struct TaskArgs {
    void (*arg0)();
    uintptr_t arg1;
    uintptr_t arg2;
} TaskArgs;



#endif /* __PLATFORM_H */
