# 高分卫星影像波段组合与拉伸预览器：详细设计及集成手册

> **文档边界与真实性声明**
>
> 本文中的宿主项目名称、业务背景、图像场景均为说明软件设计所用的虚构示例。程序生成的内置农田示意图为算法绘制的合成图，不来源于任何卫星任务或真实地理区域。本工具不读取真实遥感产品元数据、不执行地理配准，也不代表已经交付或部署给任何单位。

## 1. 工具定位

### 1.1 假设的宿主项目

本文假设存在一个虚构的 Windows 桌面软件：“天衡高分影像判读工作台”。工作台用于本地遥感影像数据准备与人工判读，已有的主窗口负责项目管理、文件列表和结果记录。

本工具作为“影像快视”页签，解决一个范围受限的问题：在导入一个普通彩色图像文件后，快速检查画面、调整 RGB 三个显示通道的映射、改善视觉对比度，并输出一个用于展示或人工沟通的 PNG 快视图。

| 项目项 | 示例设定（虚构） |
|---|---|
| 宿主产品 | 天衡高分影像判读工作台 |
| 工具入口 | 影像查看区中的“波段/拉伸”页签 |
| 输入范围 | 一幅本地 PNG 或 JPEG |
| 操作 | RGB 通道重映射、每通道百分位裁剪与线性拉伸 |
| 输出 | 拉伸后 8 位 RGB PNG 显示图 |
| 设计目标 | 本地离线、单窗口、无云服务、可嵌入 Qt Widgets 宿主 |

它类似于工程软件里一个职责单一的小工具窗口：不是编辑整份文档或管理完整遥感项目，而是打开一幅图、调几个显示参数、查看像素和直方图、另存预览结果。

### 1.2 为何特意选 PNG/JPEG

本版本采用 Qt 自带的 `QImageReader`/`QImageWriter`，只依赖 Qt Widgets。这样可以在不增加 GDAL、PROJ、OpenCV 或额外 GIS SDK 的情况下，先完成 UI、显示映射、拉伸和集成验证。

PNG/JPEG 是常见**普通图像容器**。虽然它们也可能由卫星影像生成，但通常不保留完整的多光谱测量值、地理坐标变换、投影、NoData、波段比例因子或产品质量掩膜。本工具只能对读出的 RGB 图像通道做显示处理，不能把 JPEG 中的红/绿/蓝误称为某颗卫星的特定传感器波段。

### 1.3 包含与不包含

**实现内容**

1. 打开 PNG、JPEG；显示输入文件名、图像宽高和来源。
2. 使用程序生成的合成演示影像离线试用。
3. 为显示 R/G/B 分别选择输入 R/G/B 通道。
4. 对每个输出显示通道分别计算直方图和百分位拉伸查找表。
5. 显示三条通道直方图；显示缩放图像和鼠标指向的像素 RGB 值。
6. 导出当前拉伸后的预览图为 PNG；不覆盖或修改原文件。
7. 使用 `--self-test` 对核心处理路径执行无测试框架依赖的轻量自检。

**不包含**

- GeoTIFF、JP2、NITF、SAFE 或其他带地理/传感器元数据的遥感产品解析。
- 物理波段选择，例如近红外、红边、短波红外、全色或热红外。
- 波段配准、正射校正、融合、镶嵌、云掩膜、分类或辐射定标。
- 坐标系、投影、地理坐标查询或地图叠加。
- 直方图均衡、Gamma、复杂色彩空间、曲线控制、多图联动或批处理。
- 金字塔、分块/流式读写和超大图像的低内存渲染。
- 对导出图添加地理元数据、版权、水印或审计信息。

单幅图像读取限制为 4000 万像素，用来避免意外载入超大图片导致桌面进程内存暴涨。此上限是内存保护，不是生产级高分辨率影像处理性能承诺。

## 2. 项目结构与代码职责

```text
satellite-band-preview/
├── CMakeLists.txt
├── README.md
├── docs/
│   └── design-and-integration.md
└── src/
    ├── main.cpp
    ├── image_processor.h
    ├── image_processor.cpp
    ├── image_canvas.h
    ├── image_canvas.cpp
    ├── histogram_widget.h
    ├── histogram_widget.cpp
    ├── demo_image.h
    ├── demo_image.cpp
    ├── preview_window.h
    └── preview_window.cpp
```

| 文件 | 职责 |
|---|---|
| `main.cpp` | 创建 `QApplication` 和窗口；解释可选输入路径；提供 `--self-test`。 |
| `image_processor.*` | 无 UI 的 RGB 图像处理：通道重映射、直方图、分位点和 LUT 线性拉伸。 |
| `image_canvas.*` | 用 `QGraphicsView` 显示图像，支持适配、滚轮缩放、拖动平移和坐标悬停信号。 |
| `histogram_widget.*` | 用 `QPainter` 绘制 R/G/B 直方图。 |
| `demo_image.*` | 离线生成一幅确定尺寸的合成示意图；不是实际卫星数据。 |
| `preview_window.*` | 主窗口、输入校验、控制面板、状态信息和 PNG 导出。 |
| `CMakeLists.txt` | Qt 6 Widgets、C++17、AUTOMOC 和单个可执行文件配置。 |

## 3. 总体架构

```text
PNG/JPEG 文件 ──QImageReader──► QImage (RGB32)
                                    │
                                    ▼
                        ImageProcessor::process
                         ├── 源通道直方图
                         ├── 低/高百分位估计
                         ├── 每通道 256 项查找表
                         └── 处理后的 QImage
                              │            │
                              ▼            ▼
                    ImageCanvas      HistogramWidget
                      缩放显示          显示直方图
                      像素读取
                              │
                              ▼
                    QImageWriter / PNG 导出
```

`ImageProcessor` 不依赖任何控件，因此可单独运行、测试或复用。主窗口负责输入输出和 UI 状态，不把像素循环放在自定义绘制代码里。`ImageCanvas` 只负责观看与坐标转换；`HistogramWidget` 只负责统计可视化。

## 4. 数据模型与显示通道语义

### 4.1 参数和结果类型

核心接口定义如下：

```cpp
struct PreviewSettings
{
    std::array<int, 3> sourceChannelForOutput{{0, 1, 2}};
    int lowClipPercent = 2;
    int highClipPercent = 2;
};

struct PreviewResult
{
    QImage image;
    std::array<std::array<quint64, 256>, 3> histograms{};
    std::array<int, 3> lowValues{};
    std::array<int, 3> highValues{};
};
```

`sourceChannelForOutput[0]` 决定显示图红色通道取自输入哪个颜色分量；索引 0/1/2 分别代表输入 R/G/B。对绿色、蓝色显示通道同理。重复选择同一输入通道是允许的，因而可以构造灰度式单通道显示。

每个显示通道分别计算它实际使用的输入分量直方图。举例：显示组合 R←B、G←G、B←R 时，结果里 R 直方图统计的是输入蓝通道，而不是输入红通道。这样直方图与当前显示设置一致。

### 4.2 不将文件通道命名为卫星波段

界面选项用“输入红通道”“输入绿通道”“输入蓝通道”，因为解码后的 PNG/JPEG 只有普通 RGB 显示分量。只有读取保留传感器波段定义的遥感产品，并把每个栅格 band 与产品元数据明确映射后，UI 才可以显示例如 B2、B3、B4 或 NIR 这样的真实波段名。

## 5. 处理算法

### 5.1 输入规范化

`QImageReader` 读取文件并启用方向自动纠正。输入图读入成功后转换到 `QImage::Format_RGB32`，以获得明确的 8 位红、绿、蓝通道访问语义。源图保留在 `m_sourceImage`，预览另存于 `m_previewImage`，处理过程不会原地覆写源文件或输入缓存。

### 5.2 计算直方图

对每个源像素 `(x, y)`：

1. 取出源像素 R、G、B 三个 8 位整数。
2. 根据当前三个显示通道的映射，将被选中的输入分量分别计入对应的 256 桶直方图。
3. 使用 64 位计数，避免大图像直方图在像素数增长时溢出 32 位计数器。

像素总数计算使用 `quint64`；处理尺寸约束在窗口载入阶段检查。

### 5.3 百分位裁剪

默认把每通道最低 2% 和最高 2% 当作显示离群区间。参数范围各为 0–20%。对直方图累计求出低分位点 `L` 和高分位点 `H`。裁剪主要改变显示映射；它不会从数据文件删除像素。

### 5.4 线性拉伸

每一输入亮度值 `v` 经该显示通道的查找表转换：

```text
v' = clamp(round((v - L) × 255 / (H - L)), 0, 255)
```

小于或等于低分位的值映射为 0，高于或等于高分位的值映射为 255。若 `H <= L`（例如所有像素完全相同），查找表输出中灰值 128，避免对零跨度做除法并避免把恒定图像误显示为纯黑。

所有 256 个输入值的映射先算进 LUT；第二遍逐像素查表构造预览图。这样算法保持易读，同时避免对每个像素重复计算浮点范围变换。

### 5.5 复杂度和资源

处理时间约为 `O(width × height)`，空间包括原始解码图、结果图和固定大小的直方图/LUT。以 40 MP 图像估算，两个 32 位像素缓冲本身就约占 320 MB；Qt 解码、窗口缓冲和其他应用都会增加内存占用。因此本工具适合单幅快视，超大影像应改用金字塔和分块/窗口读取，而不是简单地提高像素上限。

## 6. 用户界面交互

1. 启动时自动显示程序合成的演示影像，窗口明确标注“非真实卫星影像”。
2. 用户可打开 PNG/JPEG；读取失败会弹出错误并保留之前的预览图。
3. 左侧中央为图像画布；右侧停靠面板提供 R/G/B 输入分量、上下尾端裁剪百分比、直方图和影像信息。
4. 改变任意通道映射或百分比即重新计算当前预览。
5. 鼠标滚轮缩放、拖动平移；工具栏可放大、缩小或适合窗口。
6. 鼠标位于图像有效范围内时，状态栏显示原始像素坐标 `(x,y)` 和**处理后预览图**的 RGB 值。
7. 导出对话框只提供 PNG；导出写入独立文件，不改变原始影像。

## 7. 构建、运行和自检

### 7.1 Windows 工具链

本项目实际验证环境为 Qt 6.8.3（MSVC 2022 x64）、Visual Studio 2022 Build Tools，编译器 MSVC 19.44.35228.0、Windows SDK 10.0.26100.0，以及 CMake 4.4.4。Qt 安装前缀为 `D:\Qt`。使用 MSVC 时应选择与编译器 ABI 匹配的 Qt kit；不要将 MinGW Qt 与 MSVC 工具链混用。Visual Studio 生成器假定已安装“使用 C++ 的桌面开发”工作负载。

### 7.2 配置和编译

从项目目录执行以下 PowerShell 命令。示例的 Qt 路径应替换为本机已安装且与当前编译器匹配的 Qt 前缀：

```powershell
cmake -S . -B build `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="D:\Qt"
cmake --build build --config Release
.\build\SatelliteBandPreview.exe
```

Visual Studio 生成器会选择本机已安装的 VS 实例；VS 多配置生成器生成的可执行文件通常位于 `build\Release\SatelliteBandPreview.exe`。Windows 配置会在构建后调用 `windeployqt`，在应用目录部署 Qt 核心 DLL、平台/图像插件和 MSVC redistributable 安装程序。部署输出可能提示可选 Qt 翻译目录不存在；此工具未启用额外翻译，必要的核心 DLL/插件部署不依赖翻译目录。

Qt CMake 包未找到时，确认 `CMAKE_PREFIX_PATH` 指向含 `lib\cmake\Qt6` 的 Qt kit 根目录，而不是只指向 Qt 根目录。

### 7.3 自检

```powershell
ctest --test-dir build -C Release --output-on-failure
```

自检覆盖：

- 输入尺寸原样保留。
- 全范围拉伸将低/高端映射至黑/白端点。
- R/G/B 通道置换下每个直方图计数与预期输入分量相符。
- PNG 写入/重新读取保持预览图像素一致。
- 恒定灰度输入映射为中性灰。
- 合成演示图创建成功，尺寸为 1280×900。

这不是完整的 Qt Test 套件，也不覆盖鼠标交互、文件对话框、全部图像格式或百万像素性能；构建和实际 UI 仍需单独验证。

本次开发环境中，CMake 配置和 Release 编译成功，CTest 自检通过；已从无 Qt PATH 覆盖的应用目录启动 GUI，确认默认合成影像窗口正常创建并可正常关闭。命令行构建日志可能提示可选 Qt 翻译目录缺失；自检、PNG 编解码和 GUI 运行均已成功。

### 7.4 命令行打开影像

```powershell
.\build\SatelliteBandPreview.exe "D:\imagery\scene-preview.jpg"
```

命令行参数只支持一个输入文件。GUI 文件选择器支持 PNG、JPG 和 JPEG。

## 8. 在宿主 Qt 项目中集成

### 8.1 作为独立小程序运行

把目标应用源码和本目录 `src` 编入同一可执行目标，复用 `PreviewWindow`，但**不要**把示例项目的 `main.cpp` 加入宿主应用。宿主必须只保留一个 `QApplication` 创建入口。

宿主的 CMake 需要 Qt Widgets、C++17 和 AUTOMOC：

```cmake
find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

target_sources(ImageWorkstation PRIVATE
    path/to/satellite-band-preview/src/image_processor.h
    path/to/satellite-band-preview/src/image_processor.cpp
    path/to/satellite-band-preview/src/image_canvas.h
    path/to/satellite-band-preview/src/image_canvas.cpp
    path/to/satellite-band-preview/src/histogram_widget.h
    path/to/satellite-band-preview/src/histogram_widget.cpp
    path/to/satellite-band-preview/src/demo_image.h
    path/to/satellite-band-preview/src/demo_image.cpp
    path/to/satellite-band-preview/src/preview_window.h
    path/to/satellite-band-preview/src/preview_window.cpp
)
target_include_directories(ImageWorkstation PRIVATE
    path/to/satellite-band-preview/src
)
target_link_libraries(ImageWorkstation PRIVATE Qt6::Widgets)
```

实际宿主不需要 `demo_image.*` 时可从宿主 target 移除对应源码，并自行提供一个已验证的 `QImage` 输入入口。

### 8.2 以停靠页签嵌入现有主窗口

`PreviewWindow` 继承自 `QMainWindow`。若宿主需要把查看器嵌进 `QTabWidget`，更轻量的做法是把 `ImageCanvas`、`HistogramWidget` 和侧边控件组合为一个宿主拥有的 `QWidget` 页面，或在宿主中以独立受控子窗口打开 `PreviewWindow`。不要把 `QMainWindow` 随意塞入普通布局；其 dock/menu/central-widget 语义通常意味着它应独立作为窗口。

独立窗口调用示意：

```cpp
auto* preview = new PreviewWindow(this);
preview->setAttribute(Qt::WA_DeleteOnClose);
preview->show();
```

宿主可在构造后直接加载文件：

```cpp
auto* preview = new PreviewWindow(this);
preview->setAttribute(Qt::WA_DeleteOnClose);
if (!preview->loadImageFile(imagePath)) {
    preview->deleteLater();
    return;
}
preview->show();
```

如果宿主需要真正的页签，应将参数面板、图像画布和直方图提取成普通 `QWidget` 根组件，再让宿主决定窗口/页签生命周期。当前示例为了保持独立工具小巧，采用完整主窗口的自包含形态。

### 8.3 改为读取宿主已有影像缓存

`ImageProcessor::process(const QImage&, const PreviewSettings&)` 是脱离文件对话框的处理入口。如果宿主已经读取并完成必要的格式解码，可以直接把 RGB `QImage` 交给此函数，再将 `PreviewResult::image` 交给画布或宿主图层服务。

不要直接把 georeferenced 栅格误转成 8 位 RGB 后丢弃地理信息。若后续需要在 GIS 地图视图里显示结果，宿主必须另外保存/传播仿射变换、坐标参考系统、NoData 和显示像元范围元数据。

## 9. 错误处理和界面行为

| 情况 | 行为 |
|---|---|
| 用户取消打开/保存 | 返回，不更改当前文件或预览。 |
| 文件解码失败 | 显示读取错误；原已打开影像保持不变。 |
| 空 `QImage` | 显示错误，不生成成功形态的假预览。 |
| 超过 4000 万像素 | 警告并拒绝载入；当前图像不受影响。 |
| 处理结果为空 | 显示处理失败对话框。 |
| PNG 写入失败 | 显示保存错误，不报告已导出。 |
| 悬停位置超出图像 | 清空像素读数。 |
| 图像通道直方图为空 | 画布/直方图显示空数据状态。 |

当前版本使用本地文件名和错误文本，不调用网络服务，不自动写入遥感数据，也不静默把处理结果当成源图替代品。

## 10. 验收和验证清单

### 10.1 编译环境检查

- [ ] Qt 版本为 6.2 或更高，且包含 Widgets。
- [ ] 编译器与 Qt Kit ABI 一致（如 VS 2022 + `msvc2022_64`）。
- [ ] CMake 成功发现 Qt6 和 AUTOMOC。
- [ ] Debug/Release 目标编译通过。

### 10.2 功能检查

- [ ] 首次运行显示明确标注为合成演示数据的示意图。
- [ ] 打开 PNG/JPEG 后宽高与文件信息一致。
- [ ] 将 R 输入通道映射到显示 B 时，图像颜色有对应变化。
- [ ] 改变低/高百分位后预览和直方图同步更新。
- [ ] 单色输入不会触发除零，结果可见且为稳定中灰。
- [ ] 滚轮缩放、拖动、适配窗口、像素悬停读数工作。
- [ ] 导出的 PNG 可重新打开；输入文件仍保持原样。
- [ ] 非图像、损坏文件、取消操作和超过像素上限的图像有明确行为。
- [ ] `--self-test` 返回退出码 0 并输出自检通过信息。

### 10.3 还应增加的测试

- 更多奇数尺寸、单像素及恒定灰度图的边界测试。
- 百分位累计边界、所有裁剪范围和重复通道映射测试。
- 带 EXIF 方向 JPEG 的方向处理测试。
- 超大/损坏文件保护、路径含非 ASCII 字符和 PNG 写入失败测试。
- 代表性 CPU/内存测试、处理期间 UI 响应性与取消行为测试。
- 和最终产品内嵌的图层服务/地图视图一起测试坐标与元数据保留策略。

## 11. 已知限制和后续扩展

**当前限制**

1. 只能打开常见 PNG/JPEG；实际可读取格式还受目标 Qt 构建的图像插件影响。
2. 只支持解码后的 8 位 RGB 通道，不支持高位深 TIFF/JP2 等科学栅格。
3. 通道重映射不代表真实多光谱波段组合。
4. 没有空间参考、传感器标定、NoData 和地理像素坐标。
5. 百分位拉伸处理整个解码图；大图可能占用数百 MB 内存并阻塞 UI。
6. 处理是同步的；没有进度、后台线程、取消或缓存。
7. 直方图描述的是当前映射关联的输入颜色通道，显示为相同亮度轴上的三条曲线。
8. 输出 PNG 只保存显示结果，不保留地理参考元数据或处理参数侧车文件。

**可行扩展顺序**

1. 若需要真实卫星产品，先引入 GDAL 等成熟栅格库，支持 GeoTIFF/JP2 和读取坐标/NoData/波段元数据；同时增加 license、部署和包体积评估。
2. 将通道源改为从产品 band 列表选取，并明确显示真实 band 名称、波长与缩放因子。
3. 用窗口读取、图像金字塔和 tile cache 支持更大数据集。
4. 把分位数估计改为采样/分块统计，并把耗时操作搬到后台任务。
5. 通过可追溯 sidecar 元数据记录映射、拉伸参数、输入来源和软件版本。
6. 为预览结果与原栅格保留一致的地理变换和坐标系，经过验证后再接入地图。

每一步都代表超出当前“单幅 RGB 快视小工具”范围的功能扩展，不应把 PNG/JPEG 小样例包装成已具备真实卫星多光谱处理能力。
