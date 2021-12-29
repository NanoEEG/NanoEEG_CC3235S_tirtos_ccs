#ifndef __PLATFORM_H
#define __PLATFORM_H

/*******************************************************************
 * CONSTANTS
 */
#define APPLICATION_NAME                      ("Nano EEG")
#define APPLICATION_VERSION                   ("1.0.0")
#define DEVICE_ERROR                          ("Device error, please refer \"DEVICE ERRORS CODES\" section in errors.h")
#define WLAN_ERROR                            ("WLAN error, please refer \"WLAN ERRORS CODES\" section in errors.h")
#define SL_STOP_TIMEOUT                       (200)

#define SPAWN_TASK_PRIORITY                   (9)
#define SAMPLE_TASK_PRIORITY                  (5)
#define CONTROL_TASK_PRIORITY                 (2)
#define SOCKET_TASK_PRIORITY                  (1)
#define TASK_STACK_SIZE                       (4096)
#define CONTROL_STACK_SIZE                    (1024)
#define SAMPLE_STACK_SIZE                     (1024)
#define SLNET_IF_WIFI_PRIO                    (5)
#define SLNET_IF_WIFI_NAME                    "CC3235S"

//!< 路由参数
#define SSID_NAME                             "TUNERL-306"              /* AP SSID */
#define SECURITY_TYPE                         SL_WLAN_SEC_TYPE_WPA_WPA2 /* Security type could be SL_WLAN_SEC_TYPE_OPEN */
#define SECURITY_KEY                          "TUNERL2021"              /* Password of the secured AP */

//!< Socket参数
#define TCPPORT                               (7001)                    /* TCP控制通道端口号 */
#define UDP1PORT                              (7002)                    /* UDP数据通道端口号 */
#define UDP2PORT                              (7002)                    /* UDP标签通道端口号 */
/*******************************************************************
 * TYPEDEFS
 */
typedef struct TaskArgs {
    void (*arg0)();
    uintptr_t arg1;
    uintptr_t arg2;
} TaskArgs;



#endif /* __PLATFORM_H */
