# SMS File Format Specification

SMS 文件格式文档。

---

## 1. 总体架构

SMS 文件是一个嵌套的 **CChunk** 树形结构。每个 CChunk 具有统一的头部格式：

```
[Magic  4B]  ASCII 标识符 (如 "SMS2", "GEN ", "FRM2")
[Size   4B]  uint32, 从 Magic 开始到数据末尾的总字节数 (含头部 8 字节)
[Data   ?B]  由各 CChunk 子类的 specific reader 读取
```

CChunk::Read (sub_1004CC90) 负责读取头部，然后通过 vtable[14] 调用子类的 specific reader。

### 文件层级

```
SMS2 (CSMSCollection)
├── metadata_size: uint32 (通常为 0)
├── track_count: uint32
└── GEN  (CSMSGeneric) × track_count
    ├── track_type: uint8
    └── GTRK (CSMSGenericTrack)
        ├── track_index: uint32
        ├── track_flags: uint32
        ├── [条件字段: sampleRate, duration, frameRate, precision]
        ├── region_count: uint32
        └── RGN  (CSMSRegion) × region_count
            ├── duration: double (8B)
            ├── region_type: uint8
            ├── flags1: uint8
            ├── flags2: uint8
            ├── frame_count: uint32
            └── FRM2 (CSMSFrame) × frame_count
                ├── [帧数据 - 见下文详细描述]
                ├── ENV  (CEnvelope) × 0~3
                └── [其他可选块]
```

---

## 2. CSMSCollection (SMS2)

**Magic**: `"SMS2"` (0x32534D53)
**Reader**: sub_1005FF20
**Writer**: sub_10060100

| 偏移 | 类型 | 说明 |
|------|------|------|
| +0 | uint32 | metadata_buffer_size (0 = 无元数据) |
| +4 | byte[] | metadata (仅当 size > 0) |
| +N | uint32 | track_count |
| +N+4 | CChunk[] | GEN chunks × track_count |

---

## 3. CSMSGeneric (GEN)

**Magic**: `"GEN "` (0x204E4547)
**Reader**: sub_1006A9A0

| 偏移 | 类型 | 说明 |
|------|------|------|
| +0 | uint8 | track_type (1 = 单轨道) |
| +1 | CChunk | GTRK chunk |

---

## 4. CSMSGenericTrack (GTRK)

**Magic**: `"GTRK"` (0x4B525447)
**Reader**: sub_1006B860
**Writer**: sub_1006BCE0

| 偏移 | 类型 | 条件 | 说明 |
|------|------|------|------|
| +0 | uint32 | 总是 | track_index |
| +4 | uint32 | 总是 | track_flags (位掩码) |
| +8 | uint32 | flags & 0x01 | sampleRate (如 44100) |
| +12 | double | flags & 0x02 | duration (秒, 8 字节原始写入) |
| +20 | uint32 | flags & 0x04 | frameRate (如 172) |
| +24 | uint8 | flags & 0x08 | precision (如 11) |
| +25 | uint32 | 总是 | region_count |
| +29 | CChunk[] | | RGN chunks × region_count |

**track_flags 位定义**:
| 位 | 值 | 说明 |
|---|-----|------|
| 0 | 0x01 | 包含 sampleRate |
| 1 | 0x02 | 包含 duration |
| 2 | 0x04 | 包含 frameRate |
| 3 | 0x08 | 包含 precision |

---

## 5. CSMSRegion (RGN)

**Magic**: `"RGN "` (0x204E4752)
**Reader**: sub_1006D4E0
**Writer**: sub_1006DC30

| 偏移 | 类型 | 说明 |
|------|------|------|
| +0 | double | region duration (秒, 8B 原始写入) |
| +8 | uint8 | region_type |
| +9 | uint8 | flags1 (可选区域字段) |
| +10 | uint8 | flags2 (可选帧集合) |
| +11 | uint32 | frame_count |
| +15 | CChunk[] | FRM2 chunks × frame_count |

帧仅在 context[3] != 0 时读取 (文件加载时总是为 1)。

---

## 6. CSMSFrame (FRM2) — 核心数据结构

**Magic**: `"FRM2"` (0x324D5246)
**Reader**: sub_10067850
**Writer**: sub_10069310
**对象大小**: 624 字节 (0x270)

### 6.1 帧头部

| 文件偏移 | 对象偏移 | 类型 | 说明 |
|---------|---------|------|------|
| +0 | 312 | uint32 | version (FRM2 格式固定为 1) |
| +4 | 320 | double | **帧时间位置** (秒, 绝对值, 非持续时间!) |
| +12 | 328+332 | uint64 | **帧标志** (低32位 + 高32位) |

帧时间位置应单调递增: `frame[i].time = i / frameRate`

读取器检查 CChunk magic: 如果是旧格式 `"FRM "` (0x204D5246)，标志读取 4 字节; `"FRM2"` 格式读取 8 字节。

### 6.2 帧标志位定义

**低 32 位** (offset 328):

| 位 | 值 | 数据 | 说明 |
|---|------|------|------|
| 0 | 0x01 | float[] | 谐波振幅 (dB) |
| 1 | 0x02 | float[] | 谐波频率 (Hz) |
| 2 | 0x04 | float[] | 谐波相位 (弧度) |
| 4 | 0x10 | float[] | 噪声振幅 (dB) |
| 5 | 0x20 | float[] | 噪声相位 (弧度) |
| 6 | 0x40 | uint16[] | 瞬态数据 |
| 7 | **0x80** | **CEnvelope** | **谐波包络 (ENV1)** — growl 合成必需 |
| 8 | 0x100 | CEnvelope | 第二包络 (ENV2) |
| 9 | 0x200 | float | 基频 F0 |
| 10 | 0x400 | float | 振幅范围上限 |
| 11 | 0x800 | float | 噪声振幅范围上限 |
| 12 | 0x1000 | float | (未知字段 offset 412) |
| 13 | **0x2000** | 结构体 | **Growl 参数块** (0x228 字节) |
| 14 | 0x4000 | struct | 扩展数据 (0x23 字节) |
| 16 | 0x10000 | float[] | 额外频率数据 |
| 17 | **0x20000** | **CEnvelope** | **第三包络 (ENV3)** — 谱包络 |
| 18 | **0x40000** | uint8 | 额外字节参数 |
| 19 | **0x80000** | float | 额外浮点参数 |
| 20 | 0x100000 | float | (offset 400) |
| 21 | 0x200000 | 复杂 | 频率包络集合 |
| 22 | 0x400000 | 复杂 | 频率包络集合 |
| 23 | 0x800000 | 复杂 | 频率包络集合 |
| 27 | 0x8000000 | struct | 扩展矩阵数据 (0x174+0x4C+0x4C) |
| 30 | 0x40000000 | — | 压缩标志 (影响谐波/噪声读取方式) |
| 31 | **0x80000000** | — | **F0 已为 cents 格式** (若未设置, 引擎自动将 Hz 转为 cents) |

**高 32 位** (offset 332):

| 位 | 值 | 说明 |
|---|-----|------|
| 0 | 0x01 (bit32) | 扩展频率数组 |
| 1 | 0x02 (bit33) | 噪声扩展参数 |
| 2 | 0x04 (bit34) | 额外数组数据 |
| 3 | 0x08 (bit35) | 固定结构 (0x34 字节) |
| 5 | 0x20 (bit37) | 动态包络对象 |

### 6.3 VOCALOID Growl 标准帧格式

真实 VOCALOID growl 数据使用的标志: **0x000E2287**

```
0x01 | 0x02 | 0x04 | 0x80 | 0x200 | 0x2000 | 0x20000 | 0x40000 | 0x80000
```

按读取顺序排列的帧数据:

```
┌─ 帧头部 ─────────────────────────────────────────┐
│ version: uint32 = 1                                │
│ timePosition: double (8B)                          │
│ flags: uint64 = 0x000E2287                         │
├─ 谐波数据 (flags & 0x07) ─────────────────────────┤
│ harmonicCount: uint32 = 350                        │
│ frequencies[350]: float[] (Hz, 谐波倍数 f0*n)      │
│ amplitudes[350]: float[] (dB, 无声 = -100)         │
│ phases[350]: float[] (弧度)                        │
├─ F0 (flags & 0x200) ─────────────────────────────-┤
│ f0: float (Hz, 引擎自动转 cents 并设 0x80000000)   │
├─ Growl 参数块 (flags & 0x2000) ───────────────────┤
│ [548 字节, 详见 §7]                                │
├─ Marker (0x2000 块后) ───────────────────────────-┤
│ marker: uint8 = 0x00                               │
├─ Extra byte (flags & 0x40000) ───────────────────-┤
│ extraByte: uint8                                   │
├─ Extra float (flags & 0x80000) ──────────────────-┤
│ extraFloat: float                                  │
├─ ENV1: 谐波包络 (flags & 0x80) ──────────────────-┤
│ [CEnvelope CChunk, 详见 §8]                       │
├─ ENV3: 谱包络 (flags & 0x20000) ─────────────────-┤
│ [CEnvelope CChunk, 详见 §8]                       │
└───────────────────────────────────────────────────┘
```

### 6.4 谐波数据详解

标准 VOCALOID SMS 使用 **350 个谐波插槽**。实际非零谐波数取决于 F0 和采样率:

```
activeHarmonics = floor(nyquist / f0)
```

例如 f0=322Hz, sr=44100: `22050/322 ≈ 68` 个有效谐波。

**频率数组**: 严格谐波序列 `freq[i] = f0 * (i+1)`，未使用的插槽为 0。

**振幅数组**: 单位 dB。无声/未使用插槽为 -100.0 dB。

**相位数组**: 单位弧度 (-π 到 π)。未使用插槽为 0。

### 6.5 F0 与 Cents 编码

F0 在文件中以 **Hz** 存储 (不设 0x80000000 位)。CSMSFrame::Read 检测到此情况后自动转换:

```c
// sub_10067850 中的 F0 处理 (offset 0x10068118)
if (flags >= 0) {  // bit 31 未设置 = Hz 格式
    float cents = log(f0) * 1200.0 / ln(2);  // Hz → cents
    frame.f0_field = cents;
    flags |= 0x80000000;  // 标记为已转换
}
```

Cents 编码公式: `cents = 1200 * log2(f0 / 440.0)`
引擎常量: 440.0 (0x100867F0), 1200.0 (0x100867F8), ln(2) (0x10086790)

---

## 7. Growl 参数块 (flag 0x2000)

**Reader**: sub_100674A0
**对象偏移**: CSMSFrame + 432 (指针)
**分配大小**: 0x228 (552 字节)

这是 growl 合成引擎的核心参数集。结构由一个位掩码 DWORD 控制，指示哪些字段存在。

### 7.1 布局

```
┌─────────────────────────────────────────────────┐
│ subFlags: uint32  (标准值: 0x001FFFFF)           │
├─ bit 0  ── float  (buf+4)   模型参数 0          │
│ bit 1  ── float  (buf+8)   模型参数 1           │
│ bit 2  ── float  (buf+12)  模型参数 2           │
│ bit 3  ── float  (buf+16)  模型参数 3           │
│ bit 4  ── float  (buf+20)  模型参数 4           │
│ bit 5  ── float  (buf+24)  模型参数 5           │
│ bit 6  ── float  (buf+28)  模型参数 6           │
│ bit 7  ── float  (buf+32)  模型参数 7           │
│ bit 8  ── float  (buf+36)  模型参数 8           │
│ bit 9  ── float  (buf+40)  模型参数 9           │
├─ bit 10 ── float[40] (buf+44,  160B)  数组 A    │
│ bit 11 ── float[40] (buf+204, 160B)  数组 B     │
│ bit 12 ── float[20] (buf+364,  80B)  数组 C     │
│ bit 13 ── float[20] (buf+444,  80B)  数组 D     │
├─ bit 14 ── float  (buf+524)  模型参数 10        │
│ bit 15 ── float  (buf+528)  模型参数 11         │
│ bit 16 ── float  (buf+532)  模型参数 12         │
│ bit 17 ── float  (buf+536)  模型参数 13         │
│ bit 18 ── float  (buf+540)  模型参数 14         │
│ bit 19 ── float  (buf+544)  模型参数 15         │
└─────────────────────────────────────────────────┘
```

### 7.2 参考值 (VY2V5 GROWL)

| 字段 | 值 | 说明 |
|------|-----|------|
| bit 0 | 0.0667 | 调制速率相关 |
| bit 1 | 0.0118 | |
| bit 2 | 0.0043 | |
| bit 3 | 1.0000 | 增益/缩放 |
| bit 4 | 0.1323 | |
| bit 5 | -23.65 | dB 偏移量 |
| bit 6 | 0.0318 | |
| bit 7 | -0.0158 | |
| bit 8 | 0.0000 | |
| bit 9 | -1.1022 | |
| 数组 A-D | 全零 | (在某些帧中可能非零) |
| bit 14 | 2.9332 | |
| bit 15 | 0.0058 | |
| bit 16 | -4.0132 | |
| bit 17 | 3.1577 | |
| bit 18 | 4.2983 | |
| bit 19 | 0.9031 | |

Growl 合成核心函数 (sub_10003530) 使用这些参数控制振幅调制和频率抖动，产生"吼叫"效果。

---

## 8. CEnvelope (ENV)

**Magic**: `"ENV "` (0x20564E45)
**对象大小**: 0x1A0 (416 字节)
**Constructor**: sub_10054D20 (type=0), sub_10054E10 (type=0, 不同初始化)
**Reader**: sub_10058780
**Writer**: sub_10059320
**Vtable**: 0x1034768C

### 8.1 CChunk 内的数据格式

CEnvelope 作为 CChunk 嵌套在 FRM2 中:

```
[Magic "ENV " 4B][ChunkSize 4B][Specific Data...]
```

Specific Data (由 CEnvelope::Read sub_10058780 读取):

| 偏移 | 对象偏移 | 类型 | 说明 |
|------|---------|------|------|
| +0 | 380 (this+95) | uint32 | **env_flags** — 控制包络类型和压缩 |
| +4 | 384 (this+96) | uint32 | **data_length** — 后续数据的字节数 |
| +8 | 388 (this+97) | float | **param1** — 范围最小值 |
| +12 | 392 (this+98) | float | **param2** — 范围最大值 |
| +16 | 396 (this+99) | float | **param3** — 默认值 |
| +20 | 缓冲区 | byte[] | **data[data_length]** — 控制点数据 |

### 8.2 env_flags 解析

```
env_flags 决定两个属性:
  1. 包络类型 (type): 根据 env_flags 的高位
  2. 压缩模式 (compression): 根据 env_flags 的中间位

类型判定逻辑 (sub_10058780):
  if (env_flags & 0x800000) || (env_flags & 0x10000):
      type = 3  (样条/控制点)
  else:
      type = 2  (采样)
```

**VOCALOID 标准值**: `0x01820002`
- bit 1 设置 → (env_flags & 3) = 2
- bit 23 (0x800000) 设置 → type = 3 (样条控制点)
- 压缩字节 (offset 376) = 0 → 无压缩, 原始 float 对

### 8.3 数据格式 (type=3, 无压缩)

控制点数 = `data_length / 8`

每个控制点为一对 float:

```
[position: float]  归一化位置 (0.0 ~ 1.0)
[value: float]     值 (如 dB 振幅)
```

对于谱包络 (用于 growl):
- **position** = 归一化频率 = `harmonic_freq / nyquist`
- **value** = 该频率处的振幅 (dB)
- **param1** = -20000.0 (范围下限)
- **param2** = 20000.0 (范围上限)
- **param3** = 0.0 (默认值)

### 8.4 包络类型行为 (sub_100044C0)

包络插值函数 sub_100044C0(this, time) 的行为:

| type | 说明 | 数据存储 |
|------|------|---------|
| 0 | 未初始化 | 直接返回 0.0 |
| 1 | 控制点 (间接) | offset 348 → 指针数组 → {double time, float value} |
| 2 | 采样数据 | offset 368 → float 数组, 均匀采样 |
| 3 | 样条控制点 (直接) | offset 356 → {double time, float value, ...} × 16B |

Type 3 支持三种插值模式 (offset 314):
- 1: 线性插值
- 2: 阶梯 (最近邻)
- 3: 三次样条插值

### 8.5 帧中的包络用途

| 标志位 | 对象偏移 | 构造器 | 用途 |
|-------|---------|--------|------|
| 0x80 | this+416 | sub_10054D20(0) | **谐波谱包络** — sub_10006740 使用, growl 合成必需 |
| 0x100 | this+420 | sub_10054D20(3) | 第二谱包络 |
| 0x20000 | this+468 | sub_10054E10 | **第三谱包络** — 平滑版本 |

**关键**: Growl 合成路径 (sub_10003530 → sub_10006740) **必须**存在 offset 416 的 CEnvelope 对象。如果 flag 0x80 未设置, sub_100607E0 不会创建该对象, 导致 NULL 指针解引用崩溃:

```asm
; sub_10006740 @ 0x100067D7
mov  ecx, [edi+1A0h]    ; ecx = frame->envelope_ptr (NULL!)
call sub_100044C0        ; CRASH: this=NULL
```

---

## 9. Growl 合成流程

```
CVQMGrowl::Synthesize (sub_10005620)
  │
  ├─ sub_10002870: 创建合成缓冲区
  │   ├─ buffer+8:   目标帧 (CSMSFrame)
  │   ├─ buffer+12:  CEnvelope
  │   ├─ buffer+152: 嵌入帧 1
  │   └─ buffer+776: 嵌入帧 2
  │
  ├─ sub_10008600: 帧设置
  │   ├─ 从 SMS 加载帧到 buffer+152, buffer+776
  │   ├─ sub_10063DD0: 帧插值 (按时间混合两帧)
  │   │   └─ sub_100607E0: 帧分配/调整大小
  │   └─ byte+8==0 (CVQMGrowl): 调用 sub_10001F80
  │
  ├─ sub_10006740: 谐波包络构建 ← 需要 CEnvelope!
  │   ├─ 读取 frame+0x1A0 的 CEnvelope
  │   └─ 调用 sub_100044C0 进行包络插值
  │
  └─ sub_10003530: 主合成
      ├─ 读取 growl 参数 (frame+432)
      ├─ 应用调制 (振幅/频率抖动)
      └─ 生成音频样本
```

---

## 10. 帧读取顺序速查表

CSMSFrame::Read (sub_10067850) 按以下严格顺序读取数据:

```
 1. version (4B)
 2. timePosition (8B double)
 3. flags (4B 或 8B, 取决于 FRM 版本)
 4. [flags & 0x07]     harmonicCount (4B) + freq[] + amp[] + phase[]
 5. [flags & 0x30]     noiseCount (4B) + noiseAmp[] + noisePhase[]
 6. [flags & 0x40]     transientCount (4B) + transient[]
 7. [flags & 0x200]    f0 (4B float)
 8. [flags & 0x100000] field_400 (4B)
 9. [flags & 0x400]    field_404 (4B)
10. [flags & 0x800]    field_408 (4B)
11. [flags & 0x1000]   field_412 (4B)
12. [flags & 0x2000]   growl_block (sub_100674A0) + marker_byte
13. [hi & 0x08]        fixed_struct (0x34B)
14. [hi & 0x20]        dynamic_envelope (CChunk read)
15. [flags & 0x8000000] matrix_data (0x174+0x4C+0x4C)
16. [flags & 0x4000]   extended_data (0x23B)
17. [hi & 0x01]        extended_freq_array
18. [hi & 0x02]        noise_extended (arrays + 0x60 + 4 + 4)
19. [hi & 0x04]        extra_array
20. [flags & 0x10000]  extra_freq_count + float[]
21. [flags & 0x40000]  extra_byte (1B)
22. [flags & 0x80000]  extra_float (4B)
23. [flags & 0x200000] freq_envelope_set_1
24. [flags & 0x400000] freq_envelope_set_2
25. [flags & 0x80]     **ENV1** (CEnvelope CChunk)
26. [flags & 0x100]    ENV2 (CEnvelope CChunk)
27. [flags & 0x20000]  **ENV3** (CEnvelope CChunk)
28. [flags & 0x800000] ENV4 (CEnvelope CChunk)
```

---

## 11. IDA 地址索引

| 函数 | 地址 | 说明 |
|------|------|------|
| CSMSCollection::ctor | 0x1005FB40 | |
| CSMSCollection::Load | 0x1005FE50 | |
| CSMSCollection::Read | 0x1005FF20 | |
| CSMSCollection::Write | 0x10060100 | |
| CSMSGeneric::ctor | 0x1006A6F0 | |
| CSMSGeneric::Read | 0x1006A9A0 | |
| CSMSGenericTrack::ctor | 0x1006AC50 | |
| CSMSGenericTrack::Read | 0x1006B860 | |
| CSMSGenericTrack::Write | 0x1006BCE0 | |
| CSMSRegion::ctor | 0x1006BED0 | |
| CSMSRegion::Read | 0x1006D4E0 | |
| CSMSRegion::Write | 0x1006DC30 | |
| CSMSFrame::ctor | 0x100606B0 | |
| CSMSFrame::init | 0x100638A0 | |
| CSMSFrame::Read | 0x10067850 | 帧数据读取主函数 |
| CSMSFrame::Write | 0x10069310 | |
| CSMSFrame::allocate | 0x100607E0 | 帧内存分配/调整 |
| CSMSFrame::copy | 0x10061AE0 | 帧拷贝 |
| CSMSFrame::interpolate | 0x10063DD0 | 帧插值 |
| CChunk::ctor | 0x1004C790 | |
| CChunk::Read | 0x1004CC90 | 读 magic+size, 调 vtable[14] |
| CChunk::Write | 0x1004CDE0 | 写 magic+size, 调 vtable[13] |
| CChunk::readHeader | 0x1004CD20 | |
| CChunk::writeHeader | 0x1004CEC0 | |
| CEnvelope::ctor (type=0) | 0x10054D20 | |
| CEnvelope::ctor (alt) | 0x10054E10 | |
| CEnvelope::Read | 0x10058780 | |
| CEnvelope::Write | 0x10059320 | |
| CEnvelope::setup | 0x10056C30 | 设置类型/分配控制点 |
| CEnvelope::interpolate | 0x100044C0 | 按时间插值取值 |
| CEnvelope vtable | 0x1034768C | |
| GrowlBlock::Read | 0x100674A0 | 0x2000 growl 参数读取 |
| GrowlSynth::main | 0x10003530 | Growl 合成主函数 |
| GrowlSynth::entry | 0x10005620 | CVQMGrowl 入口 |
| GrowlSynth::harmonicEnv | 0x10006740 | 谐波包络构建 (崩溃点) |
| GrowlSynth::frameSetup | 0x10008600 | 合成帧设置 |
| FrameLookup | 0x100040D0 | 时间→帧 二分查找 |
| ContextInit | 0x10060400 | |
| VQM ini load | 0x1000A2A0 | |

---

## 12. 关键注意事项

1. **QDataStream::SinglePrecision 会截断 double 为 4 字节!** 所有 double 字段 (如 duration, timePosition) 必须用 `writeRawData` 写入 8 字节。

2. **noiseCount 不能为 0!** 如果使用 noise flags (0x10/0x20), noiseCount 必须 > 0。合成函数 (sub_10003530) 计算 `v96 = noiseCount - 1`, 当 noiseCount=0 时 v96=-1 导致数组越界崩溃。标准值: 25 (Bark 频率尺度)。

3. **CChunk size 包含头部!** size 字段 = magic(4) + size(4) + data = 总字节数。

4. **帧时间是绝对位置, 非持续时间!** offset 320 存储的是帧在区域内的绝对时间位置。帧查找 (sub_100040D0) 对此字段做二分搜索。

5. **CVQMGrowl 总是走复杂路径!** 构造器 (sub_10002350) 将 byte+8 硬编码为 0, 意味着合成总是需要 CEnvelope 对象。

6. **F0 存储为 Hz, 不是 cents!** 真实 VOCALOID 数据不设置 0x80000000 位。引擎在 Read 时自动转换并设置该位。
