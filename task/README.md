`@task/cc1310_Sync`
================
cc1310_Sync任务用来处理事件标签数据，包含以下功能：
- 与NanoEEG板载cc1310的I2C通信功能：当板载cc1310接收到TrigerBox发送的事件标签数据并处理完毕后，通过`CC1310_WAKEUP` (TODO) IO中断通知cc3235S接收事件标签数据，两者通过I2C通信。
cccc
- 与NanoEEG板载cc1310的RAT时钟同步功能：cc3235S通过`CC1310_Sync_PWM` IO引脚与板载cc1310相连，引脚的输出作为cc1310的RAT某一通道的输入捕获信号。**在脑电采集开始后**，cc3235S的同步时钟（`Sync_Timer`）每1s通过翻转`CC1310_Sync_PWM` IO电平完成板载cc1310的同步,每1s触发电平翻转的同时记录下当前系统时间（`@ref service/timestamp`）作为同步时间戳（`SyncTimestamp of cc3235S`，T*soc*）。

    > 板载cc1310的RAT对`CC1310_Sync_PWM`上下边沿捕获，并保存捕获值为同步时间戳(`SyncTimestamp of cc1310`, T*sor*)，当接收到一包TrigerBox事件标签数据包时，添加数据包接收时间点时间戳（`RecvTimestamp of rat`, T*ror*） 和同步时间（`SyncTimestamp of cc1310`, T*sor*），通知cc3235S接收。数据传输格式如下：
    > | 序号 | 接收时间戳 | 同步时间戳 | 标签类型 |
    > | :-: | :--: | :--: | :--: |
    > | idx | T*ror* | T*sor* | type |
    > | uint8_t | uint32_t | uint32_t | uint8_t |
    >   


- 事件标签的时间回溯：因为事件标签数据包的接收时点总在两次同步之间，在已知RAT时钟为4MHz，溢出值为$2^{32}$的情况下，通过下式可以求出事件标签接收时点与最近一次同步时点的时间差：

    $ ∆t=\begin{cases}
    (Tror-Tsor)/(4\times\;10^6), \qquad \qquad \qquad Tror\geq\; Tsor\\
    (Tsor + Nr - 2^{32} -Tror)/(4\times\;10^6), \quad Tror < Tsor
    \end{cases} $

    其中，N*r*是1s同步周期对应的RAT的计数值。由于RAT在一次脑电实验的数据采集过程中会发生溢出，导致最近一次同步时间戳值小于事件标签接收时间戳，在计算时间差时需要考虑溢出的情况。在已知系统时间戳精度为10us，通过下式可以求出以cc3235S的时钟为基准的事件标签接收时间戳（`RecvTimestamp of cc32325s`, T*roc*) ）：
    
    $\qquad \qquad \qquad \qquad \qquad  Troc=∆t \times\;10^5 +Tsoc $



> 板载cc1310
