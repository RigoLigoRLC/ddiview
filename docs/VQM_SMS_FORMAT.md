# VQM 和 SMS 文件格式分析

## 1. VQM.ini 配置文件

VQM.ini 是标准的 Windows INI 配置文件，DSE4 从以下位置读取：
- `C:\VQM\vqm.ini` (硬编码路径)
- `{歌手目录}\vqm.ini`

### INI 文件结构

```ini
[section_name]
begin=0.0        ; 起始时间 (秒, float)
end=1.0          ; 结束时间 (秒, float)
pitch=60.0       ; 音高 (MIDI note, float)
file=growl.wav   ; 对应的WAV文件名
```

每个 section 代表一个 VQM 样本，DSE4 处理流程：
1. 读取 WAV 文件路径，替换 `vqm.ini` 为 `VQM\` 得到完整路径
2. 将 `.wav` 替换为 `.sms` 得到对应的 SMS 文件路径
3. 加载 SMS 频谱数据

## 2. SMS 文件格式 (Spectral Modeling Synthesis)

SMS 文件是 DSE4 的频谱建模合成数据格式。

### 2.1 文件魔数

| Magic | 十六进制 | 说明 |
|-------|----------|------|
| `SMS2` | 0x32534d53 | SMS版本2 (当前版本) |
| `SMS ` | 0x20534d53 | SMS版本1 |
| `FORM` | 0x4d524f46 | 旧版FORM格式 |

### 2.2 层次结构

```
SMS文件
├── 文件头
│   ├── Magic: "SMS2" (4字节)
│   └── 数据大小 (4字节)
│
├── CSMSCollection (容器)
│   ├── Track数量 (4字节)
│   └── CSMSGeneric[] (轨道数组)
│
└── CSMSGeneric (GEN , 0x204e4547)
    ├── 轨道类型标志 (1字节): 1=单轨, 2=双轨, 3=四轨
    └── CSMSGenericTrack[] (GTRK, 0x4b525447)
        ├── TrackIndex (4字节)
        ├── Flags (4字节)
        ├── SampleRate (4字节, 如果flags&1)
        ├── Duration (8字节 double, 如果flags&2)
        ├── FrameRate (4字节, 如果flags&4)
        ├── Precision (1字节, 如果flags&8)
        ├── RegionCount (4字节)
        └── CSMSRegion[] (RGN , 0x204e4752)
```

### 2.3 CSMSRegion 结构

```
CSMSRegion (RGN )
├── TimeOffset (8字节 double)
├── RegionType (1字节)
├── Flags1 (1字节)
│   ├── 0x01: 包含ADSR包络数据
│   ├── 0x02: 包含VoiceType
│   ├── 0x04: 包含ScoringNoteIndex
│   ├── 0x08: 包含SegmentData
│   ├── 0x10: 包含PitchContour
│   ├── 0x20: 包含TimbreParams
│   ├── 0x40: 包含ExtraParams
│   └── 0x80: 包含帧数据
├── [条件字段...]
├── Flags2 (1字节)
└── CSMSFrame[] (FRM2)
```

### 2.4 CSMSFrame 帧数据结构

帧签名: `FRM2` (0x324d5246)

| 字段 | 条件 | 大小 | 说明 |
|------|------|------|------|
| FrameIndex | 总是 | 4 | 帧索引 |
| TimePosition | 总是 | 8 | 帧的绝对时间位置 (double, 秒) |
| Flags | 总是 | 8 | 64位标志位 |
| HarmonicCount | flags&7 | 4 | 谐波数量 |
| Frequencies | flags&2 | N×4 | 谐波频率 (float[]) |
| Amplitudes | flags&1 | N×4或N×1 | 谐波幅度 |
| Phases | flags&4 | N×4或N×1 | 谐波相位 |
| NoiseCount | flags&0x30 | 4 | 噪声带数量 |
| NoiseAmplitudes | flags&0x10 | M×4 | 噪声幅度 |
| NoisePhases | flags&0x20 | M×4 | 噪声相位 |
| BandCount | flags&0x40 | 4 | 频带数量 |
| BandData | flags&0x40 | K×2 | 频带数据 (short[]) |
| F0 | flags&0x200 | 4 | 基频 (float, 转换为cents) |

### 2.5 帧标志位详解

**低32位 (flags & 0xFFFFFFFF):**

| 位 | 说明 |
|---|------|
| 0x01 | 包含谐波幅度 |
| 0x02 | 包含谐波频率 |
| 0x04 | 包含谐波相位 |
| 0x10 | 包含噪声幅度 |
| 0x20 | 包含噪声相位 |
| 0x40 | 包含频带数据 |
| 0x80 | 包含频谱包络 |
| 0x100 | 包含频谱包络2 |
| 0x200 | 包含F0基频 |
| 0x400 | 幅度范围扩展 (-100~100 → -100~200) |
| 0x800 | 噪声幅度范围扩展 |
| 0x1000 | 包含额外参数1 |
| 0x2000 | 包含谐波详细数据 |
| 0x4000 | 包含共振峰数据 |
| 0x8000 | 未知 |
| 0x10000 | 包含额外数组1 |
| 0x20000 | 包含频谱包络3 |
| 0x40000 | 包含单字节字段1 |
| 0x80000 | 包含4字节字段1 |
| 0x100000 | 包含4字节字段2 |
| 0x8000000 | 包含大型数据块 (0x174 + 0x4C×2) |

**高32位 (flags >> 32):**

| 位 | 说明 |
|---|------|
| 0x01 | 包含数组数据1 |
| 0x02 | 包含数组数据2 + 额外数据 |
| 0x04 | 包含数组数据3 |
| 0x08 | 包含52字节数据块 |
| 0x20 | 包含频谱包络对象 |

**压缩标志:**

| 位 | 说明 |
|---|------|
| 0x40000000 | 谐波数据使用字节压缩 (0-255 映射) |
| 0x20000000 | 噪声数据使用字节压缩 |
| 0x80000000 | F0已转换为cents |

### 2.6 压缩数据转换

**谐波幅度 (字节压缩):**
- 0xFF = 10000.0 (静音标记)
- 其他: `value / 254.0 * range - offset`
  - 普通: range=100, offset=100 → [-100, 0]
  - 扩展(0x400): range=200, offset=100 → [-100, 100]

**谐波相位 (字节压缩):**
- `value / 255.0 * 2π`

**谐波频率 (字节压缩):**
- `value / 255.0 * 2.0 + index` (相对于谐波索引的偏移)

## 3. 关键类和函数地址

| 地址 | 类/函数 | 说明 |
|------|---------|------|
| 0x1005FB40 | CSMSCollection() | SMS容器构造 |
| 0x1005FE50 | CSMSCollection::Load | 加载SMS文件 |
| 0x1005FF20 | CSMSCollection::Read | 读取SMS数据 |
| 0x10060100 | CSMSCollection::Write | 写入SMS数据 |
| 0x1006A6F0 | CSMSGeneric() | 通用轨道构造 |
| 0x1006AC50 | CSMSGenericTrack() | 轨道构造 |
| 0x1006B860 | CSMSGenericTrack::Read | 读取轨道 |
| 0x1006BCE0 | CSMSGenericTrack::Write | 写入轨道 |
| 0x1006BED0 | CSMSRegion() | 区域构造 |
| 0x1006D4E0 | CSMSRegion::Read | 读取区域 |
| 0x1006DC30 | CSMSRegion::Write | 写入区域 |
| 0x100606B0 | CSMSFrame() | 帧构造 |
| 0x10067850 | CSMSFrame::Read | 读取帧 |
| 0x10069310 | CSMSFrame::Write | 写入帧 |
| 0x1000A2A0 | LoadVQMIni | 加载VQM.ini |
| 0x10018B30 | InitVQM | 初始化VQM系统 |

## 4. 相关字符串

```
"C:\VQM\vqm.ini"           - 默认VQM配置路径
"%svqm.ini"                - 歌手目录VQM配置
"Cannot open VQM.ini\n"    - VQM.ini打开失败
"Cannot open WAV:\n"       - WAV文件打开失败
"Cannot open SMS:\n"       - SMS文件打开失败
"Failed to instantiate VQM object" - VQM对象创建失败
"Failed to switch VQM profiles for singer!" - VQM配置切换失败
"VFXGrowl"                 - Growl效果名称
```

## 5. RTTI 类名

```
.?AVCSMSCollection@@
.?AVCSMSFrame@@
.?AVCSMSFrameCollection@@
.?AVCSMSGeneric@@
.?AVCSMSGenericTrack@@
.?AVCSMSRegion@@
.?AVCDBVVQM@@
.?AVCDBVVQMPhU@@
.?AVCDBVVQMPhUPart@@
.?AVCVQModification@@
.?AVCVQMGrowl@@
.?AVCVQMorph@@
```

## 6. Growl SMS 生成流程

### 6.1 类继承关系

```
CVQMorph (基类, vftable @ 0x100866e0)
    └── CVQMGrowl (派生类, vftable @ 0x10085d24)
```

### 6.2 VQM.ini 加载流程 (0x1000A2A0)

1. 打开 VQM.ini 文件
2. 使用 `GetPrivateProfileSectionNamesW` 枚举所有 section
3. 对每个 section:
   - 读取 `begin`, `end`, `pitch`, `file` 字段
   - 构造 WAV 文件路径: `{ini目录}\VQM\{file}`
   - 构造 SMS 文件路径: 将 `.wav` 替换为 `.sms`
   - 加载 WAV 文件
   - 加载对应的 SMS 文件

### 6.3 WAV 文件读取 (0x1000AFC0)

使用 Windows Multimedia API (mmio) 读取 WAV 文件:

```c
struct WavReader {
    HMMIO hmmio;           // +0x00: 文件句柄
    WORD  wFormatTag;      // +0x04: 格式标签 (1=PCM)
    WORD  nChannels;       // +0x06: 声道数
    DWORD nSamplesPerSec;  // +0x08: 采样率
    DWORD nAvgBytesPerSec; // +0x0C: 平均字节率
    WORD  nBlockAlign;     // +0x10: 块对齐
    WORD  wBitsPerSample;  // +0x12: 位深度
    DWORD dataSize;        // +0x18: 数据大小
    DWORD sampleCount;     // +0x1C: 采样点数
    DWORD dataOffset;      // +0x20: 数据偏移
    DWORD currentPos;      // +0x24: 当前位置
};
```

### 6.4 波形数据处理 (0x10009B10)

1. 从 WAV 读取 16-bit PCM 数据
2. 转换为 float (-1.0 ~ 1.0): `sample * 0.000030517578125` (即 1/32768)
3. 应用淡入淡出窗口 (前64个采样点)
4. 生成镜像波形用于循环播放
5. 存储到 VQM 样本数据结构

### 6.5 CVQMorph 数据结构 (0x10002440)

```c
struct CVQMorph {
    void* vftable;           // +0x00: 虚函数表
    // ...
    BYTE  flag1;             // +0x08
    BYTE  flag2;             // +0x09
    int   harmonicCount;     // +0x28 (this+10): 谐波数量
    int   frameRate;         // +0x2C (this+11): 帧率
    int   maxHarmonics;      // +0x30 (this+12): 最大谐波数
    // ...
    float* freqBuffer1;      // +0x4C (this+19): 频率缓冲区1
    float* freqBuffer2;      // +0x54 (this+21): 频率缓冲区2
    float* ampBuffer1;       // +0x5C (this+23): 幅度缓冲区1
    float* ampBuffer2;       // +0x64 (this+25): 幅度缓冲区2
    float* phaseBuffer1;     // +0x6C (this+27): 相位缓冲区1
    float* phaseBuffer2;     // +0x74 (this+29): 相位缓冲区2
    void*  waveformBuffer;   // +0x7C (this+31): 波形缓冲区 (0x80000字节)
    CSMSFrame* currentFrame; // +0x8C (this+35): 当前帧
    // ...
    float* gainTable;        // +0xA8 (this+42): 增益表
};
```

### 6.6 Growl 合成过程 (0x10005620, 0x10003530)

1. 根据目标音高选择最近的 VQM 样本
2. 从 SMS 帧数据中提取谐波参数
3. 应用音高变换和相位调制
4. 生成输出帧数据

关键参数:
- 目标频率 (F0)
- Growl 强度 (0.0 ~ 1.0)
- 相位偏移
- 时间位置

### 6.7 SMS 帧生成

Growl SMS 帧主要包含:
- 谐波频率 (相对于基频的倍数)
- 谐波幅度 (dB)
- 谐波相位 (弧度)
- 噪声频谱包络

## 7. 生成 Growl SMS 的要点

要生成可用的 Growl SMS 文件，需要:

1. **准备 WAV 源文件**: 包含 growl 效果的音频样本
2. **进行频谱分析**: 提取谐波和噪声成分
3. **生成 SMS 结构**:
   - CSMSCollection 容器
   - CSMSGeneric 轨道 (通常单轨)
   - CSMSGenericTrack
   - CSMSRegion (按时间分段)
   - CSMSFrame (每帧频谱数据)

4. **配置 VQM.ini**: 指定样本的音高、时间范围和文件路径

### 7.1 最小 SMS 文件结构

**重要**: CSMSCollection::Load 调用 CChunk::Read，因此文件以 CChunk 头部开始：
[4字节魔数 "SMS2"][4字节总大小]。CSMSCollection::Read 作为 specific reader 在
CChunk 头部之后开始解析。每个子结构（GEN/GTRK/RGN/FRM2）都是 CChunk 格式，
带有 [4字节魔数][4字节大小] 头部。大小字段包含头部本身（8字节+数据）。

```
SMS2 文件 (从位置0开始):
├── CChunk "SMS2" (CSMSCollection, 外层容器)
│   ├── Magic: "SMS2" (4字节)
│   ├── ChunkSize: 整个文件大小 (4字节, 包含头部)
│   ├── MetadataBufferSize: 0 (4字节, 最小文件无需元数据)
│   ├── TrackCount: 1 (4字节)
│   └── CChunk "GEN " (CSMSGeneric)
    ├── Magic: "GEN " (4字节)
    ├── ChunkSize (4字节, 包含头部)
    ├── TrackType: 1 (1字节, 单轨)
    └── CChunk "GTRK" (CSMSGenericTrack)
        ├── Magic: "GTRK" (4字节)
        ├── ChunkSize (4字节)
        ├── TrackIndex: 0 (4字节)
        ├── Flags: 0x0F (4字节)
        ├── SampleRate: 44100 (4字节, flags&1)
        ├── Duration: (8字节 double, flags&2)
        ├── FrameRate: 172 (4字节, flags&4)
        ├── Precision: 11 (1字节, flags&8)
        ├── RegionCount (4字节)
        └── CChunk "RGN " (CSMSRegion)[]
            ├── Magic: "RGN " (4字节)
            ├── ChunkSize (4字节)
            ├── TimeOffset: 0.0 (8字节 double)
            ├── RegionType: 0 (1字节)
            ├── Flags1: 0x00 (1字节)
            ├── Flags2: 0x00 (1字节)
            ├── FrameCount (4字节)
            └── CChunk "FRM2" (CSMSFrame)[]
                ├── Magic: "FRM2" (4字节)
                ├── ChunkSize (4字节)
                ├── FrameIndex (4字节)
                ├── TimePosition (8字节 double, 帧的绝对时间位置)
                ├── Flags: 0x80000207 (8字节, amp|freq|phase|F0|cents)
                ├── HarmonicCount: N (4字节)
                ├── Frequencies[N] (N×4字节 float)
                ├── Amplitudes[N] (N×4字节 float)
                ├── Phases[N] (N×4字节 float)
                └── F0: (4字节 float, cents)
```
