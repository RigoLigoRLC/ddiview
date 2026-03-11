# DDI/DDB Chunk 结构文档

## 基础结构

### CChunk (基类)
所有 chunk 的基类结构：

| 偏移 | 大小 | 字段名 | 说明 |
|------|------|--------|------|
| +0x00 | 8 | LeadingQword | 可选，仅当 HasLeadingQword=true 时存在 |
| +0x08 | 4 | Signature | 4字节签名，如 "ARR ", "SND " 等 |
| +0x0C | 4 | Size | chunk 总大小（包含头部） |

### CChunkArray (数组容器)
签名: `ARR `

继承自 CChunk，用于存储子 chunk 数组。

| 偏移 | 大小 | 字段名 | 说明 |
|------|------|--------|------|
| 基类 | - | CChunk | 基类字段 |
| +0x130 (304) | 4 | ArrayFlags | 数组标志位 |
| +0x140 (320) | 4 | UseEmptyChunk | 是否使用空 chunk 占位 |
| +0x150 (336) | 4 | Count | 子元素数量 |

---

## 数据库结构

### CDBSinger (歌手数据库)
签名: `DBSe`

| 偏移 | 大小 | 字段名 | 说明 |
|------|------|--------|------|
| 基类 | - | CChunkArray | 基类字段 |
| +0x2F8 (760) | 4 | Version | 数据库版本号 (1=V2, 2=V3/V4) |
| 内嵌 | - | PhoneticDictionary | 音素字典（无 LeadingQword） |
| +0x104 (260) | 260 | HashStore | 哈希校验数据（非 DevDb 模式） |

### CDBVoice (声音数据库)
签名: `DBV `

纯容器，无额外字段，直接继承 CChunkArray。

### CDBTimbre (音色数据库)
签名: `TDB `

纯容器，无额外字段。

### CDBTimbreModel (音色模型)
签名: `TMM `

| 偏移 | 大小 | 字段名 | 说明 |
|------|------|--------|------|
| 基类 | - | CChunkArray | 基类字段 |
| +0x160 (352) | 4 | ModelIndex | 模型索引 |

---

## Stationary (稳态音素)

### CDBVStationary
签名: `STA `

纯容器，无额外字段。

### CDBVStationaryPhU (稳态音素单元)
签名: `STAu`

| 偏移 | 大小 | 字段名 | 说明 |
|------|------|--------|------|
| 基类 | - | CChunkArray | 基类字段 |
| +0x160 (352) | 4 | Index | 音素索引 |
| +0x168 (360) | 4 | PitchRangeHigh | 音高范围上限 |
| +0x164 (356) | 4 | PitchRangeLow | 音高范围下限 |

### CDBVStationaryPhUPart (稳态音素部分)
签名: `STAp`

| 偏移 | 大小 | 字段名 | 说明 |
|------|------|--------|------|
| 基类 | - | CChunkArray | 基类字段 |
| +0x178 (376) | 8 | TimeInfo | 时间信息 |
| +0x1A0 (416) | 2 | Flags | 标志位 |
| +0x160 (352) | 4 | mPitch | 相对音高 (MIDI note) |
| +0x164 (356) | 4 | AveragePitch | 平均音高 |
| +0x168 (360) | 4 | PitchDeviation | 音高偏差 |
| +0x16C (364) | 4 | Dynamic | 动态/力度 |
| +0x170 (368) | 4 | Tempo | 速度 |
| +0x174 (372) | 4 | LoopInfo | 循环信息 |
| 子数组后 | - | - | 以下为 PostRead 数据 |
| +0x1B0 (432) | 4 | FrameCount | 帧数量 |
| +0x1A8 (424) | 8*N | FrameRefs | 帧引用数组 |
| +0x1C8 (456) | 4 | SampleRate | 采样率 |
| +0x1CC (460) | 2 | ChannelCount | 声道数 |
| +0x1D0 (464) | 4 | SampleCount | 采样点数 |
| +0x1C0 (448) | 8 | SampleOffset | 音频数据偏移 |
| +0x1F0 (496) | 4 | EpRTrackIndex | EpR轨道索引 (-1=无) |
| +0x1F4 (500) | 4 | ResTrackIndex | 残差轨道索引 (-1=无) |
| +0x1F8 (504) | 4 | OptionalIndex3 | 可选索引3 (-1=无) |
| +0x1FC (508) | 4 | OptionalIndex4 | 可选索引4 (-1=无) |

---

## Articulation (连接音素)

### CDBVArticulation
签名: `ART `

| 偏移 | 大小 | 字段名 | 说明 |
|------|------|--------|------|
| 基类 | - | CChunkArray | 基类字段 |
| +0x160 (352) | 4 | Index | 连接类型索引 |

### CDBVArticulationPhU (连接音素单元)
签名: `ARTu`

| 偏移 | 大小 | 字段名 | 说明 |
|------|------|--------|------|
| 基类 | - | CChunkArray | 基类字段 |
| +0x160 (352) | 4 | Index | 音素索引 |
| +0x164 (356) | 4 | TargetIndex1 | 目标索引1 |
| +0x168 (360) | 4 | TargetIndex2 | 目标索引2 |
| +0x16C (364) | 4 | TargetIndex3 | 目标索引3 |
| +0x170 (368) | 4 | TargetIndex4 | 目标索引4 |

### CDBVArticulationPhUPart (连接音素部分)
签名: `ARTp`

结构与 CDBVStationaryPhUPart 类似，额外包含：
- Section 数据（半音素分割信息）
- HalfPhoneSegmentation 结构

---

## VQMorph (声音变形)

### CDBVVQM / CDBVVQMorph
签名: `VQM `

纯容器。

### CDBVVQMPhU (VQM 音素单元)
签名: `VQMu`

| 偏移 | 大小 | 字段名 | 说明 |
|------|------|--------|------|
| 基类 | - | CChunkArray | 基类字段 |
| +0x160 (352) | 4 | Index | 音素索引 |

### CDBVVQMPhUPart (VQM 音素部分)
签名: `VQMp`

结构与 CDBVStationaryPhUPart 基本相同。

---

## SMS (Spectral Modeling Synthesis) 结构

### CSMSGenericTrack
签名: `GTRK`

| 偏移 | 大小 | 字段名 | 说明 |
|------|------|--------|------|
| +0x1A0 (416) | 4 | TrackType | 轨道类型 |
| +0x170 (368) | 4 | Flags | 标志位 |
| +0x174 (372) | 4 | SampleRate | 采样率 |
| +0x180 (384) | 8 | Duration | 持续时间 |
| +0x188 (392) | 4 | FrameRate | 帧率 |
| +0x18C (396) | 1 | Precision | 精度 |
| 后续 | 4 | RegionCount | 区域数量 |

### CSMSRegion
签名: `RGN `

| 偏移 | 大小 | 字段名 | 说明 |
|------|------|--------|------|
| +0x28 (40) | 8 | TimeOffset | 时间偏移 |
| +0x184 (388) | 1 | RegionType | 区域类型 |
| +0x185 (389) | 1 | Flags1 | 标志位1 |
| +0x188 (392) | 4 | Flags2 | 标志位2（条件读取） |

Flags1 位定义：
- 0x01: 包含扩展参数
- 0x02: 包含 unk18
- 0x04: 包含 ScoringNoteIndex
- 0x08: 包含 SegmentationData
- 0x10: 包含 PitchContour
- 0x20: 包含 TimbreData
- 0x40: 包含 ExtraData

### CSMSFrame
签名: `FRM2`

复杂的帧数据结构，包含：

| 偏移 | 大小 | 字段名 | 说明 |
|------|------|--------|------|
| +0x140 (320) | 4 | FrameType | 帧类型 |
| +0x148 (328) | 8 | FrameFlags | 帧标志位（64位） |
| +0x156 (342) | 4 | HarmonicCount | 谐波数量 |

FrameFlags 位定义：
- 0x01: 包含 Frequencies 数组
- 0x02: 包含 Magnitudes 数组
- 0x04: 包含 Phases 数组
- 0x10: 包含 NoiseEnvelope
- 0x20: 包含 NoiseMagnitudes
- 0x40: 包含 Residual
- 0x200: 包含 F0 (基频)
- 0x2000: 包含 FrameInfo
- 0x40000000: 使用压缩格式

---

## 音频数据

### CSoundChunk
签名: `SND `

| 偏移 | 大小 | 字段名 | 说明 |
|------|------|--------|------|
| 基类 | - | CChunk | 基类字段 |
| +0x00 | 4 | SampleRate | 采样率 (通常 44100) |
| +0x04 | 2 | ChannelCount | 声道数 (通常 1) |
| +0x06 | 4 | SampleCount | 采样点数 |
| +0x0A | N*2 | Samples | 16位 PCM 采样数据 |

---

## 其他结构

### CEmptyChunk
签名: `EMPT`

空占位 chunk，仅包含名称。

### CSkipChunk
用于跳过未知 chunk 类型。

### CPhoneticDictionary
音素字典，包含：
- 音素定义列表
- HMM 模型数据
- Codebook 数据
- PhoneticUnitGroup 数据

---

## 版本差异

### DDI vs DDB
- DDI: 完整数据库，包含所有音频数据
- DDB: 开发数据库，部分字段不同

### Version 标志
- Version 1: Vocaloid 2 格式
- Version 2: Vocaloid 3/4/5 格式

### HasLeadingQword
- true: 每个 chunk 前有 8 字节前导数据
- false: 无前导数据（如 PhoneticDictionary）

---

## 数据流程

1. 读取 chunk 头部 (签名 + 大小)
2. 根据签名创建对应类型
3. 调用 ReadSelf 读取类型特定数据
4. 如果是数组类型，递归读取子元素
5. 调用 PostRead 读取后续数据（如帧引用）
6. 读取 chunk 名称（字符串）
