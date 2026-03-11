# Vocaloid 引擎架构与合成原理

## 1. 整体架构

```
┌─────────────────────────────────────────────────────────────┐
│                      Vocaloid Engine                        │
├─────────────────────────────────────────────────────────────┤
│  输入层: VSQX/VSQ (乐谱) + DDI/DDB (声库)                    │
├─────────────────────────────────────────────────────────────┤
│  处理层:                                                     │
│    ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│    │ 音素选择器    │→│ 单元拼接器    │→│ 频谱合成器    │     │
│    │ PhU Selector │  │ Concatenator │  │ SMS Synth    │     │
│    └──────────────┘  └──────────────┘  └──────────────┘     │
├─────────────────────────────────────────────────────────────┤
│  输出层: PCM 音频波形                                        │
└─────────────────────────────────────────────────────────────┘
```

## 2. 声库数据结构 (DDI/DDB)

### 2.1 层级结构

```
CDBSinger (DBSe) - 歌手数据库根节点
├── PhonemeDict (PHDC) - 音素字典
│   └── PhonemeGroup[] - 音素组 (按语言/类型分组)
│       └── Phoneme[] - 具体音素定义
│
├── CDBVoice (DBV) - 声音数据
│   ├── CDBVStationary (STA) - 稳态音素库
│   │   └── CDBVStationaryPhU[] (STAu) - 稳态音素单元
│   │       └── CDBVStationaryPhUPart[] (STAp) - 不同音高的采样
│   │
│   ├── CDBVArticulation (ART) - 连接音素库
│   │   └── CDBVArticulationPhU[] (ARTu) - 连接音素单元
│   │       └── CDBVArticulationPhUPart[] (ARTp) - 不同音高的采样
│   │
│   └── CDBVVQMorph (VQM) - 声质变换库
│       └── CDBVVQMorphPhU[] (VQMu)
│           └── CDBVVQMorphPhUPart[] (VQMp)
│
└── CDBTimbre (TDB) - 音色模型库
    └── CDBTimbreModel[] (TMM) - 音色模型
```

### 2.2 音素单元 (PhU) 结构

每个 PhUPart 包含特定音高下的完整合成数据：

```
PhUPart
├── 元数据
│   ├── TimeInfo (8字节) - 时间戳信息
│   ├── Flags (2字节) - 控制标志
│   ├── mPitch - 相对音高 (MIDI note)
│   ├── AveragePitch - 平均基频
│   ├── PitchDeviation - 音高偏差
│   ├── Dynamic - 力度
│   └── Tempo - 速度
│
├── SMS 轨道
│   ├── EpR Track - 激励+共振轨道 (主要频谱)
│   └── Res Track - 残差轨道 (噪声成分)
│
├── 帧数据
│   ├── FrameCount - 帧数量
│   └── FrameRefs[] - 帧引用数组
│
└── 音频数据
    ├── SampleRate - 采样率 (通常 44100)
    ├── ChannelCount - 声道数
    ├── SampleCount - 采样点数
    └── SampleOffset - 原始波形偏移
```

## 3. SMS (Spectral Modeling Synthesis) 频谱建模合成

### 3.1 核心概念

Vocaloid 使用 **EpR (Excitation plus Resonances)** 模型：

```
语音信号 = 激励源 (Excitation) × 共振滤波器 (Resonances) + 残差 (Residual)

┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  Excitation │ ──→ │  Resonance  │ ──→ │   Output    │
│  (基频脉冲)  │     │  (共振峰)    │     │   (语音)    │
└─────────────┘     └─────────────┘     └─────────────┘
                           ↑
                    ┌──────┴──────┐
                    │  Residual   │
                    │  (气息噪声)  │
                    └─────────────┘
```

### 3.2 SMS 轨道结构

```cpp
// SMSGenericTrack 轨道参数
struct SMSTrack {
    uint32_t TrackType;      // 轨道类型 (EpR=0, Res=1, ...)
    uint32_t Flags;          // 标志位
    uint32_t SampleRate;     // 采样率
    double   Duration;       // 持续时间
    uint32_t FrameRate;      // 帧率 (通常 ~172 fps)
    uint8_t  Precision;      // 数据精度

    SMSRegion Regions[];     // 区域数组
};
```

### 3.3 SMS 区域 (Region)

每个区域代表一个语音片段，包含包络和控制参数：

```cpp
struct SMSRegion {
    double   TimeOffset;     // 时间偏移
    uint8_t  RegionType;     // 区域类型

    // 包络参数 (ADSR)
    float    AttackTime;     // 起音时间
    float    DecayTime;      // 衰减时间
    float    SustainLevel;   // 持续电平
    float    ReleaseTime;    // 释音时间

    // 音高控制
    float    PeakLevel;      // 峰值
    float    InitialLevel;   // 初始值
    float    FinalLevel;     // 结束值

    // 颤音参数
    float    VibratoDepth;   // 颤音深度
    double   VibratoRate;    // 颤音速率
    double   VibratoDelay;   // 颤音延迟

    // 稳定区标记 (用于循环/拉伸)
    uint32_t StableBegin;    // 稳定区开始帧
    uint32_t StableEnd;      // 稳定区结束帧

    SMSFrame Frames[];       // 频谱帧数组
};
```

### 3.4 SMS 帧 (Frame)

每帧包含一个时间点的完整频谱信息：

```
Frame 结构 (约 5.8ms/帧 @ 172fps):
┌────────────────────────────────────────┐
│ 谐波成分 (Harmonics)                    │
│   - 基频 F0                             │
│   - 谐波幅度 A1, A2, A3, ...            │
│   - 谐波相位 φ1, φ2, φ3, ...            │
├────────────────────────────────────────┤
│ 共振峰 (Formants)                       │
│   - F1, F2, F3, F4, F5 频率             │
│   - B1, B2, B3, B4, B5 带宽             │
│   - A1, A2, A3, A4, A5 幅度             │
├────────────────────────────────────────┤
│ 残差频谱 (Residual Spectrum)            │
│   - 噪声包络                            │
└────────────────────────────────────────┘
```

## 4. 合成流程

### 4.1 音素选择

```
输入: 歌词 + 音高 + 时值
         ↓
┌─────────────────────────────────────┐
│ 1. 歌词 → 音素序列转换               │
│    "か" → [k, a]                    │
│    "さ" → [s, a]                    │
└─────────────────────────────────────┘
         ↓
┌─────────────────────────────────────┐
│ 2. 查找音素单元                      │
│    - Stationary: 元音稳态 [a], [i]  │
│    - Articulation: 过渡 [k→a], [a→s]│
└─────────────────────────────────────┘
         ↓
┌─────────────────────────────────────┐
│ 3. 音高匹配                          │
│    - 查找最接近目标音高的 PhUPart    │
│    - 使用 PitchRangeHigh/Low 范围   │
└─────────────────────────────────────┘
```

### 4.2 单元拼接

```
时间轴:
    ├──────┼──────────────┼──────┼──────────────┼──────┤
    │ ART  │     STA      │ ART  │     STA      │ ART  │
    │[k→a] │     [a]      │[a→s] │     [a]      │[s→?] │
    ├──────┼──────────────┼──────┼──────────────┼──────┤

拼接策略:
1. 使用 StableBegin/StableEnd 标记确定可拉伸区域
2. 元音稳态部分可循环拉伸以匹配时值
3. 过渡部分保持原始长度
4. 在边界处进行交叉淡化 (crossfade)
```

### 4.3 频谱合成

```
┌─────────────────────────────────────────────────────┐
│                    帧插值                           │
│  Frame[n] ──────────────────────────→ Frame[n+1]   │
│     ↓              插值                    ↓        │
│  ┌──────┐    ┌──────┐    ┌──────┐    ┌──────┐     │
│  │ F0   │    │ F0'  │    │ F0'' │    │ F0   │     │
│  │ Harm │ →  │ Harm'│ →  │Harm''│ →  │ Harm │     │
│  │ Form │    │ Form'│    │Form''│    │ Form │     │
│  └──────┘    └──────┘    └──────┘    └──────┘     │
└─────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────┐
│                  音高变换                           │
│  - 根据目标音高调整 F0                              │
│  - 保持共振峰位置 (防止 "花栗鼠效应")               │
│  - 使用 PSOLA 或频域变换                           │
└─────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────┐
│                  波形重建                           │
│  1. 谐波合成: Σ An·sin(n·ω0·t + φn)               │
│  2. 共振滤波: 应用共振峰滤波器                      │
│  3. 残差叠加: 添加气息噪声成分                      │
│  4. 输出: 16-bit PCM @ 44100Hz                     │
└─────────────────────────────────────────────────────┘
```

### 4.4 表情控制

```cpp
// 从反编译代码中发现的表情参数
struct ExpressionParams {
    // 动态控制
    float DYN;        // 力度 (0-127)
    float BRE;        // 气息 (Breathiness)
    float BRI;        // 明亮度 (Brightness)
    float CLE;        // 清晰度 (Clearness)

    // 音高控制
    float PIT;        // 音高弯曲
    float PBS;        // 音高弯曲灵敏度
    float POR;        // 滑音时间

    // 共振峰控制
    float GEN;        // 性别因子 (Gender Factor)
    float OPE;        // 开口度 (Mouth Opening)
};
```

## 5. 关键算法

### 5.1 音高匹配算法

```cpp
// 伪代码: 选择最佳 PhUPart
PhUPart* SelectBestPart(PhU* phu, float targetPitch) {
    PhUPart* best = nullptr;
    float minDist = FLT_MAX;

    for (auto part : phu->Parts) {
        // 检查音高是否在范围内
        if (targetPitch >= part->PitchRangeLow &&
            targetPitch <= part->PitchRangeHigh) {

            float dist = abs(targetPitch - part->AveragePitch);
            if (dist < minDist) {
                minDist = dist;
                best = part;
            }
        }
    }
    return best;
}
```

### 5.2 时间拉伸算法

```cpp
// 伪代码: 稳态区域拉伸
void StretchStableRegion(Region* region, double targetDuration) {
    double stableDuration = region->StableEnd - region->StableBegin;
    double totalDuration = region->Duration;
    double nonStableDuration = totalDuration - stableDuration;

    // 计算需要的稳态区长度
    double neededStable = targetDuration - nonStableDuration;

    // 循环次数
    int loopCount = ceil(neededStable / stableDuration);

    // 生成拉伸后的帧序列
    for (int i = 0; i < loopCount; i++) {
        // 复制稳态区帧，应用交叉淡化
        CopyFramesWithCrossfade(region->StableBegin, region->StableEnd);
    }
}
```

### 5.3 EpR 轨道索引

```cpp
// 从反编译发现: PhUPart 末尾的四个索引字段
struct PhUPartIndices {
    int32_t EpRTrackIndex;    // EpR 轨道索引 (-1 = 无)
    int32_t ResTrackIndex;    // 残差轨道索引 (-1 = 无)
    int32_t OptionalIndex3;   // 可选索引 (用途待确认)
    int32_t OptionalIndex4;   // 可选索引 (用途待确认)
};

// 获取 EpR 轨道
SMSTrack* GetEpRTrack(PhUPart* part) {
    if (part->EpRTrackIndex == -1) return nullptr;
    return part->Children[part->EpRTrackIndex];
}
```

## 6. 版本差异

| 特性 | V2 | V3/V4 | V5 |
|------|-----|-------|-----|
| 数据库版本号 | 1 | 2 | 2 |
| LeadingQword | 无 | 有 | 有 |
| 哈希校验 | 无 | 有 | 有 |
| VQMorph | 无 | 有 | 有 |
| 采样率 | 44100 | 44100 | 44100 |
| 帧率 | ~172 | ~172 | ~172 |

## 7. 总结

Vocaloid 引擎的核心是 **基于 SMS 的拼接合成**：

1. **预录制**: 录制大量音素样本，提取频谱参数
2. **存储**: 以 EpR 模型存储，分离激励和共振
3. **选择**: 根据音高和上下文选择最佳样本
4. **拼接**: 在频谱域进行平滑拼接
5. **变换**: 应用音高、时值、表情变换
6. **合成**: 重建时域波形输出

这种方法结合了拼接合成的自然度和参数合成的灵活性，是 Vocaloid 音质的关键。

---

*文档基于 ddiview 项目逆向工程成果，仅供学习研究使用。*
