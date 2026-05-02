# GREE AC Remote (格力空调遥控器)

[中文](README_CN.md) | [English](README.md)

适用于 Flipper Zero 的格力空调红外遥控应用。

## 简介

本应用允许你使用 Flipper Zero 设备通过红外信号控制格力（GREE）品牌空调。支持常见的空调功能，包括电源开关、模式选择（制冷/除湿/送风/制热/自动）、温度调节、风速控制，以及摆风、强劲、自清洁等特殊功能。

## 功能特性

- **模式选择**：制冷、除湿、送风、制热、自动
- **温度控制**：16°C - 30°C
- **风速控制**：自动、1、2、3 档
- **摆风**：切换垂直摆风
- **LED**：切换显示设定温度或环境温度
- **强劲模式**：启用快速制冷/制热
- **自清洁**：启用空调自清洁功能

## 按钮操作

按钮右上角的黑色三角形表示该按钮支持长按操作：

* **"Swing"（摆风）按钮**：发送切换垂直摆风命令（而非水平摆风，如支持）
* **"LED"（显示）按钮**：切换显示设定温度或环境温度（如支持）
* **"Fan"（风速）按钮**：切换静音模式（如支持），在关机、模式或风速改变或按下"强劲"/"自清洁"后重置

## 构建方法

```shell
# 克隆仓库
git clone https://github.com/CreeperAWA/FlipperZero-GREE-AC-Remote_YAPOF20.git
cd FlipperZero-GREE-AC-Remote_YAPOF20

# 构建应用
ufbt build

# 构建并运行应用
ufbt launch
```

## 致谢

本项目基于并参考了以下项目：

- [Midea AC Remote](https://github.com/xakep666/flipperzero-midea-ac-remote) by [@xakep666](https://github.com/xakep666) - Flipper Zero 空调遥控应用的原始实现
- [Mitsubishi AC Remote](https://github.com/achistyakov/flipperzero-mitsubishi-ac-remote) by [@achistyakov](https://github.com/achistyakov) - 基础代码结构

感谢以上项目的所有贡献者为本项目奠定的基础。
