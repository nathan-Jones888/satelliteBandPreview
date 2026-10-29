# 高分卫星影像波段组合与拉伸预览器

SatelliteBandPreview 是一个以 **Qt 6 Widgets、C++17 和 CMake** 构建的 Windows 桌面影像快视工具。它将一幅本地 PNG/JPEG 解码为 8 位 RGB 图像，支持 RGB 显示通道重映射、分位数对比度拉伸、直方图查看、缩放/平移、预览像素读数和 PNG 快视图导出。

本项目刻意保持小而明确：处理算法、图像画布、直方图和窗口控制分别位于独立模块；可作为其他 Qt Widgets/遥感桌面应用里的视觉快视功能起点。

## 先了解数据边界

> **重要：本项目不是多光谱卫星数据处理软件。** 输入范围为普通 PNG/JPG/JPEG 中的 **8 位 RGB 显示像素**。界面所称“通道组合”，只是将输入红/绿/蓝颜色分量重新映射到屏幕输出 R/G/B；它不会从 JPEG 推断近红外、红边或其他传感器 band，也没有读取真实卫星产品元数据。内置演示图是程序绘制的合成示意影像，**不是真实卫星照片**。当前工具不支持 GeoTIFF、JP2、坐标系、地理变换、辐射定标、NoData、云掩膜或地物分类。

因此，当前结果适用于显示预览和程序学习，不应作为科学遥感产品、定量测量结果、带坐标图层或原始数据的替代物。若真实需求是处理物理传感器波段，需要增加真实栅格读取与 band/CRS/NoData 等元数据模型。

## 项目适合做什么

- 在没有 GDAL 等额外 GIS SDK 的情况下，演示 Qt 桌面图像工具的完整开发与部署链。
- 学习 RGB 通道映射、直方图统计、离散百分位和查找表（LUT）线性拉伸。
- 作为完整 Qt 遥感工作台中一个小型图像**显示快视子窗口**的原型。
- 练习模块分层、输入输出错误处理、CMake/AUTOMOC 和自动测试。

## 功能一览

| 功能 | 当前行为 |
| --- | --- |
| 本地图像 | 打开 PNG、JPG/JPEG；也可通过命令行传入文件路径。 |
| 合成演示影像 | 启动默认展示程序绘制的 1280×900 农田风格示意图，标题标明它并非真实卫星影像。 |
| RGB 映射 | 为输出 R/G/B 独立选择输入 R/G/B 显示分量；允许重复使用某个输入颜色分量。 |
| 百分位拉伸 | 为每个输出显示通道统计映射源的 256 桶直方图，计算低/高百分位边界并应用线性拉伸。界面默认低端和高端各裁剪 2%，可选 0–20%。 |
| 直方图 | 以三条曲线展示当前映射所使用的 RGB 源像素分量分布。 |
| 查看与像素读数 | 适配窗口、工具栏缩放、滚轮缩放、拖动平移；悬停显示当前处理后预览图的像素坐标及 RGB 值。 |
| PNG 导出 | 将当前显示变换结果另存为 PNG，不覆写源文件。该 PNG 不保留空间参考或传感器 band 元数据。 |
| 输入保护 | 单图限制为 40,000,000 像素；报告读图或保存错误，不应把失败伪装成成功。 |
| 算法自检 | `--self-test` / CTest 检查通道映射、直方图、拉伸边界、常量图、PNG 往返和合成演示图生成。 |

## 操作流程

1. 启动程序；首次显示带明确标签的合成演示图。
2. 点击 **打开影像**，选择本地 PNG/JPG/JPEG；也可启动时把路径作为第一个参数传入。
3. 在右侧面板分别选择显示 R、G、B 对应的输入颜色分量。
4. 调整低端和高端裁剪百分比，观察处理图像和直方图变化。
5. 用滚轮缩放、拖动平移或使用 **适合窗口**；鼠标悬停可查看当前预览像素的坐标和 RGB。
6. 点击 **导出 PNG** 保存屏幕显示效果。保留原始数据和元数据；不要用显示快照代替科学产品。

```powershell
.\build\Release\SatelliteBandPreview.exe
.\build\Release\SatelliteBandPreview.exe "D:\imagery\scene-preview.jpg"
```

## 技术架构

```text
PNG/JPEG
   │ QImageReader
   ▼
QImage / Format_RGB32
   │
   ▼
ImageProcessor::process
   ├── 直方图：按显示 RGB 映射统计三组 256 桶
   ├── percentileValue：查找低/高亮度端点
   ├── LUT：每通道预计算 256 个亮度输出
   └── 输出独立的 RGB32 预览 QImage
          ├── ImageCanvas：缩放、平移、像素悬停
          ├── HistogramWidget：直方图绘制
          └── 用户确认后导出 PNG
```

| 组件 | 文件 | 职责 |
| --- | --- | --- |
| 应用入口 | `src/main.cpp` | 创建 `QApplication`/预览窗口、解释命令行路径、运行内置处理器自检。 |
| 核心处理器 | `src/image_processor.h/.cpp` | 纯图像算法入口；生成直方图、计算百分位、构造 LUT 并输出预览图。 |
| 图像画布 | `src/image_canvas.h/.cpp` | 使用 `QGraphicsView` 显示图像、处理缩放平移并发出悬停像素坐标。 |
| 直方图控件 | `src/histogram_widget.h/.cpp` | 用 `QPainter` 绘制三通道直方图。 |
| 合成演示图 | `src/demo_image.h/.cpp` | 绘制教学用途的模拟图像，不读取或伪装真实卫星数据。 |
| 主窗口 | `src/preview_window.h/.cpp` | 管理控件交互、输入读取、预览刷新、错误提示和 PNG 导出。 |
| 构建与测试 | `CMakeLists.txt` | Qt6 Widgets、C++17、AUTOMOC、MSVC 警告、`windeployqt` 与 CTest。 |

### 算法概要

1. `QImageReader` 读取本地图像、执行方向变换，并规范化为 `QImage::Format_RGB32`。
2. 根据映射设置，为每个**输出显示通道**统计所对应输入颜色分量的 256 桶直方图。
3. 用离散桶累计计算低/高百分位亮度边界 `L` 和 `H`；百分位值内部裁剪到 0–49%。
4. 针对每通道预先计算 256 项 LUT，按 `round((v-L)×255/(H-L))` 线性映射并限制到 0–255。
5. 若 `H <= L`，将退化通道映射到中灰 128，避免零跨度除法。
6. 再逐像素查 LUT，生成独立预览图，源 `QImage` 不会被原地修改。

算法时间复杂度为 `O(宽 × 高)`；主要内存消耗来自源图像、预览图和 Qt 显示缓存。处理在当前 GUI 线程同步执行，故 40 MP 只是输入保护上限，**不是实时性能承诺**。

## 技术要求

- Qt **6.2+ Widgets**，并安装与本机编译器 ABI 匹配的 Windows x64 Qt Kit。
- CMake **3.21+**。
- 支持 C++17 的编译器；本工程已用 Visual Studio 2022 / MSVC x64 验证。
- `qwindows.dll` 平台插件及 PNG/JPEG 解码插件须可用；Windows 构建阶段通过 `windeployqt` 部署。

### 本机验证环境

| 组件 | 已验证版本/状态 |
| --- | --- |
| Qt | 6.8.3 Widgets，安装前缀 `D:\Qt` |
| CMake | 4.4.4 |
| 编译器 | Visual Studio 2022 Build Tools，MSVC 19.44.35228.0 |
| Windows SDK | 10.0.26100.0 |
| 构建生成器 | Visual Studio 17 2022 / x64 |

本机原有的 Qt 5.9.5/Anaconda 保持不变，Qt 6 单独安装；如果新的 VS Code 终端找不到 `cmake`，重新打开终端或重启 VS Code 以刷新用户 PATH。

## 配置、编译、测试和启动

在项目根目录 `D:\codeSpace\cProject\satellite-band-preview` 中运行：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="D:\Qt"

cmake --build build --config Release --parallel 4

ctest --test-dir build -C Release --output-on-failure

.\build\Release\SatelliteBandPreview.exe
```

输入本地图像：

```powershell
.\build\Release\SatelliteBandPreview.exe "D:\imagery\scene-preview.jpg"
```

CTest 会运行目标程序的 `--self-test`。自检覆盖 RGB 映射和直方图、零裁剪的黑白端点、恒定灰度输入中灰、PNG 内存写入/读回一致性及合成演示图尺寸；它不是完整 GUI 自动化测试，也没有验证 GeoTIFF 或实际传感器波段。

Windows 构建会在目标目录部署 Qt 核心 DLL、图像/平台插件及适用的 MSVC runtime installer。不要只复制 `.exe`。向另一台机器分发时，请完整提供运行目录或下方便携 ZIP，并遵守 Qt 和各依赖适用的许可证/NOTICE 条款。

## 分享与简历使用

**仅运行演示：**

- [Windows x64 便携运行包](dist/SatelliteBandPreview-Windows-x64-Qt6.8.3-v1.0.zip)
- 完整解压后运行 `SatelliteBandPreview\SatelliteBandPreview.exe`，不要只发送 exe。
- [分享包 SHA-256 校验值](dist/SHA256SUMS.txt)

**学习或构建源码：**

- [源码分享 ZIP](dist/SatelliteBandPreview-source-v1.0.zip)
- 或从本目录直接查看 `src/`、CMakeLists 和设计文档。

这是一个共享的学习示例底稿。求职者可把它作为学习、复现和改造的起点；简历必须依据本人实际提交、修改、测试和可以独立讲解的工作来写。不得把共享代码冒充个人独立原创，也不得声称已支持当前未实现的真实卫星波段或 GeoTIFF/JP2 多波段。

更多面试介绍、常见问题、5 分钟演示脚本和贡献归属指导见 [面试准备 Word 手册](docs/interview-preparation-manual.docx)。

## 文档导航

- [项目技术手册（Word）](docs/satellite-band-preview-project-manual.docx)：技术基础、完整设计、算法、构建、宿主项目集成和验收。
- [面试准备手册（Word）](docs/interview-preparation-manual.docx)：分享方法、简历贡献边界、面试问答和演示准备。
- [详细设计与集成手册（Markdown）](docs/design-and-integration.md)：源代码架构、接口/算法说明和集成示例。

## 许可与数据分享注意

当前工程是学习和功能演示样例。公开或商业发布前需补齐并审查项目自身许可证，并核对 Qt、MSVC runtime 及其他随包第三方组件适用的许可和声明义务。不要在分享包或公共仓库中加入未获分享授权的真实卫星影像、客户文件、坐标清单、账户信息或密钥。
