# NanoEEG_CC3235S 开发简介
## 开发环境
1. [Code Composer Studio™ Desktop IDE 最新版本](https://www.ti.com.cn/zh-cn/design-resources/embedded-development/ccs-development-tools.html#ide-desktop)
2. TI SimpleLink CC32XX SDK (5.30.00.08)

> 如遇到中文乱码问题，请修改中文字体编码为UTF-8 通过`Properties->Resource->Text file encoding` 修改

## 软件架构
![如ccs内无法查看，请点击imgs/架构图.png](./imgs/架构图.png)

## 版本说明
|	版本号	|	功能描述	| 版本贡献 |
| :-------: | :-------	| :-------: |
| Version1.0.0 | <br>**脑电采集基本功能**</br> <br>- 支持全局采样率修改</br> <br>- 支持全局增益修改</br> | gjmsilly |
| Version1.0.1 | <br>**事件标签功能**</br> <br>- 事件标签子系统完成</br> | gjmsilly |