`@service`
================
I/O层建立在TI SimpleLink CC3235S SDK Driver/Driverlib（驱动层）之上，板载主控芯片CC3235S片内外设和板载各类传感器通过该层抽象，对外以服务的形式供上层调用。本层的编码规范如下：

1. 对于板载传感器（sensor），均需要提供`hw_sensor.c`和`sensor.c`文件。

    - 采用面向对象的思想，每类传感器均需要定义传感器对象（sensor_t sensor），传感器驱动函数通过传感器对象实现CC3235S与传感器的交互；
    - `hw_sensor.c`生成传感器对象，包括实现传感器与板载主控芯片CC3235S建立通信；
    - `sensor.c`提供基于传感器对象的驱动函数。

2. 对于主控芯片CC3235S的片内外设，只需要提供`service.c`文件。
    
    - `service.c` 生成外设服务对象供上层直接调用。

`@service\bq27441-g1`
================
1. [BQ27441-G1 Datasheet](http://www.ti.com/lit/ds/symlink/bq27441-g1.pdf)
2. [BQ27441-G1 Technical Reference Manual](https://www.ti.com.cn/cn/lit/ug/sluuac9a/sluuac9a.pdf?ts=1646275394089&ref_url=https%253A%252F%252Fwww.ti.com.cn%252Fproduct%252Fcn%252FBQ27441-G1)

`@service\ads1299`
1. [ADS1299 Datasheet](https://www.ti.com.cn/cn/lit/ds/symlink/ads1299.pdf?ts=1646205655715&ref_url=https%253A%252F%252Fwww.ti.com.cn%252Fproduct%252Fcn%252FADS1299)


## 代码移植
若板载主控芯片更换，`sensor.c`文件保持不动，`hw_sensor.c`根据更换后的主控芯片覆写即可。`service.c`需要整体重新覆写。