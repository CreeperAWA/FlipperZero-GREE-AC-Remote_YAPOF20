#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
根据CSDN文章中的格力空调红外协议文档，完整解码67位帧结构
协议: 起始码 + 35位数据 + 连接码 + 32位数据
"""

def decode_gree_ac_signal(data_str):
    """解码格力空调IR信号为67位数据结构"""
    values = [int(x) for x in data_str.split()]
    
    ZERO_SPACE_THRESHOLD = 1000
    
    frames = []
    idx = 0
    frame_count = 0
    
    # 格力协议: 35位 + 连接码 + 32位 = 67位数据
    # 需要: header(2) + 67位*2 + end mark(2) = 至少138个值
    while idx < len(values) - 130 and frame_count < 2:
        # 寻找header: mark~9000, space~4500
        if values[idx] >= 8500 and values[idx+1] >= 4000:
            idx += 2
            bits = []
            # 读取67位(8字节+3位)
            for _ in range(67):
                if idx + 1 >= len(values):
                    break
                mark = values[idx]
                space = values[idx + 1]
                idx += 2
                bit_val = 1 if space >= ZERO_SPACE_THRESHOLD else 0
                bits.append(bit_val)
            
            frames.append(bits)
            frame_count += 1
            # 跳过end mark和gap
            if idx + 1 < len(values):
                idx += 2
        else:
            idx += 1
    
    return frames


def analyze_frame_bits(name, bits):
    """分析帧的每一位"""
    if len(bits) < 35:
        print(f"  帧数据不完整({len(bits)}位)")
        return
    
    print(f"\n  {'='*60}")
    print(f"  信号: {name}")
    print(f"  {'='*60}")
    
    # 按照CSDN协议文档解析前35位
    print(f"\n  前35位数据码(按协议文档):")
    print(f"  Bit1-6 (固定): {bits[0:6]}")
    
    # 模式标志 (Bit 7-9, 3 bits)
    mode_bits = bits[6:9]
    mode_val = mode_bits[0] | (mode_bits[1] << 1) | (mode_bits[2] << 2)
    mode_map = {0: "自动", 1: "制冷", 2: "加湿", 3: "送风", 4: "制热"}
    print(f"  Bit7-9 (模式): {mode_bits} = {mode_val} -> {mode_map.get(mode_val, '未知')}")
    
    # 开关 (Bit 10, 1 bit)
    power = bits[9]
    print(f"  Bit10 (开关): {power} -> {'开' if power else '关'}")
    
    # 风速 (Bit 11-12, 2 bits)
    fan_bits = bits[10:12]
    fan_val = fan_bits[0] | (fan_bits[1] << 1)
    fan_map = {0: "自动", 1: "1档", 2: "2档", 3: "3档"}
    print(f"  Bit11-12 (风速): {fan_bits} = {fan_val} -> {fan_map.get(fan_val, '未知')}")
    
    # 扫风 (Bit 13, 1 bit)
    swing = bits[12]
    print(f"  Bit13 (扫风): {swing} -> {'开' if swing else '关'}")
    
    # 温度 (Bit 14-17, 4 bits)
    temp_bits = bits[13:17]
    temp_val = temp_bits[0] | (temp_bits[1] << 1) | (temp_bits[2] << 2) | (temp_bits[3] << 3)
    temperature = temp_val + 16
    print(f"  Bit14-17 (温度): {temp_bits} = {temp_val} -> {temperature}°C")
    
    # 打印前35位的完整序列
    print(f"\n  前35位完整序列:")
    for i in range(35):
        if i > 0 and i % 8 == 0:
            print()
        print(f"  {i+1}:{bits[i]}", end="")
    print()
    
    # 按字节显示
    bytes_val = []
    for byte_idx in range(5):
        byte_val = 0
        for bit in range(8):
            if byte_idx * 8 + bit < len(bits):
                byte_val |= (bits[byte_idx * 8 + bit] << bit)
        bytes_val.append(byte_val)
    
    print(f"\n  按字节显示(LSB first):")
    print(f"  Byte0: 0x{bytes_val[0]:02X} = {bytes_val[0]:08b}")
    print(f"  Byte1: 0x{bytes_val[1]:02X} = {bytes_val[1]:08b}")
    print(f"  Byte2: 0x{bytes_val[2]:02X} = {bytes_val[2]:08b}")
    print(f"  Byte3: 0x{bytes_val[3]:02X} = {bytes_val[3]:08b}")
    if len(bytes_val) > 4:
        print(f"  Byte4: 0x{bytes_val[4]:02X} = {bytes_val[4]:08b} (前3位)")


# 直接从 code.ir 读取完整数据
import re

def read_ir_file():
    """读取 code.ir 文件并解析所有信号"""
    signals = {}
    try:
        with open('code.ir', 'r', encoding='utf-8') as f:
            lines = f.readlines()
        
        current_name = None
        current_data = None
        
        for line in lines:
            line = line.strip()
            if line.startswith('name:'):
                current_name = line.split(':', 1)[1].strip()
            elif line.startswith('data:'):
                current_data = line.split(':', 1)[1].strip()
                if current_name and current_data:
                    signals[current_name] = current_data
                    current_name = None
                    current_data = None
    except Exception as e:
        print(f"读取文件失败: {e}")
    
    return signals


print("=" * 80)
print("格力空调IR信号完整解码 (基于CSDN协议文档)")
print("=" * 80)

# 读取完整IR文件
signals = read_ir_file()
print(f"从 code.ir 读取到 {len(signals)} 个信号")

# 只分析包含"风速"的信号
for name, data in signals.items():
    if '风速' not in name:
        continue
        
    print(f"\n调试: 解码 {name}...")
    frames = decode_gree_ac_signal(data)
    print(f"  解码到 {len(frames)} 帧")
    if frames:
        print(f"  第一帧有 {len(frames[0])} 位")
        analyze_frame_bits(name, frames[0])
    else:
        print(f"  数据长度: {len(data.split())} 个timing值")

print("\n" + "=" * 80)
print("风速编码总结")
print("=" * 80)
print("""
根据CSDN协议文档:
  Bit11-12 (2 bits): 风速
  - 00: 自动
  - 01: 1档
  - 10: 2档
  - 11: 3档

需要验证捕获数据中是否正确编码了4个挡位
""")
