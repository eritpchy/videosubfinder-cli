中文教程实时更新：https://docs.qq.com/doc/DRk9HWWlXdkRFa05o?&u=146bab4d9f414c3693447729af1de915

#--------------------关于程序--------------------

程序主要提供两个功能：
1）通过数字图像处理算法，自动检测视频中带有硬字幕的帧，生成只带有时间轴的空白字幕；
2）通过文本挖掘算法，用带有硬字幕的图片生成图形文字。可以用其他软件来进一步识别，如FineReader和Subtitle Edit，生成既有文本又有时间轴的字幕。

为了使程序正常运行，需要下载安装"Microsoft Visual C++ Redistributable runtime libraries 2022"，下载地址：
https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads
最新版本测试基于Windows 10 x64, Ubuntu 20.04.5 LTS, Arch Linux (EndeavourOS Cassini Nova 03-2023)

#--------------------快速入门指南--------------------

#---------生成RGB图片---------
1）点击菜单栏"File->Open Video"（推荐使用OpenCV，提取的时间轴准确性更高）；
2）在"Video Box"中，通过移动垂直和水平分隔线来缩小检测区域，结果更准确、时间轴更完整；
3）确认字幕在检测区域内的水平对齐方式："Center/Left/Right/Any"中心对齐/左对齐/右对齐/任意对齐，在"Settings"标签页里的"Text Alignment"中选择对应属性
4）强烈推荐使用"Use Filter Colors"颜色滤镜提高识别结果准确率，该步骤可以跳过：
* - 拖动进度条，找到带有字幕的图像
* - 鼠标点击"Video Box"任意位置，选中后按"U"，全屏查看图片，鼠标左键点击字幕像素点，即可获取字幕的颜色信息
* - 字幕颜色信息在"Settings"便签页中的右下角，复制Lab颜色信息到左边的"Use Filter Colors"编辑栏中，如：Lab: l:0 a:128 b:128
* - 如果字幕颜色有多种，可以用"Ctrl+Enter"在"Use Filter Colors"编辑栏中添加新的一行颜色信息
5）在"Search"标签页中点击"Run Search"（如果你只需要时间轴，这步之后，直接跳到"OCR"标签页，按 "Create Empty Sub From RGBImages"，生成一个只有时间轴信息的字幕）
#---------二值化图片---------
6）如果不使用"Use Filter Colors"颜色滤镜，则需要注意：
     在继续下一步之前，检查字幕边线的颜色是否比字幕文本的颜色更深（大多数情况下是这样的，如果不是，取消选择在"Settings"标签页中的右边栏中的第一个复选框）
     在大多数情况下，程序能正确地识别与字幕文本相关联的颜色，但某些情况过于复杂，就需要通过其他参数设置来处理；
7）点击"OCR"标签页中的"Create Cleared TXTImages"
8）需要借助其他软件才可获得可编辑的字幕文本

#--------------------已知的问题--------------------

1) 用OpenCV和FFMPEG打开视频，提取出的时间轴会有细微的差别，在000-001毫秒之间
2) 关于字幕中心对齐，如果字幕整体在检测区域的右边，将会被移除

#--------------------参数设置--------------------

1）为了在"Run Search"和"Create Cleared TXTImages"得到更好的结果
#---------生成RGB图片---------
*- 导入视频，参数设置好之后，在"Settings"便签页点击"Test"按钮来进行测试，尽量多的选择视频中背景明暗度差异较大的帧来进行测试，比如背景太亮或者太暗
*- 通过用鼠标移动"Video Box"中的垂直和水平分隔线来缩小检测区域
   在某些复杂的情况下，你可以多次运行程序来识别不同区域的字幕，可以解决多行字幕可能出现分割问题
   比如：识别双语字幕、对话在视频下面而注释在视频上面的情况
*- 确认字幕在检测区域里的水平对齐方式："Center/Left/Right/Any"中心对齐/左对齐/右对齐/任意对齐，在"Settings"标签页里的"Text Alignment"中选择对应属性
   中心对齐：默认对齐方式
   任意对齐：当前不如其他几种对齐方式好用
*- 为了减少在"Run Search"时出现的：丢轴、错轴，在"Clean TXT Images"时出现的：文字未被识别、只识别部分的情况，
   对"Moderate Threshold"参数的范围[0.25, 0.6]进行调整：
   为了找到最佳的参数值，在"Test"时，需要对'After First/Second/Third Filtration'不同的阶段进行测试，
   直到在右边的"Image Box"中出现完整的白色字幕。
   尽可能尝试那些太亮或太暗的画面，这样画面中大部分与字幕无关的像素将会被移除。
   0.25 - 大多数视频通用，特别是1080p的视频，但是会生成大量无用的图片
   0.5-0.6 - 适用于字幕边线明显的，像白字黑边框的字幕，或者画质低于480p的视频
   0.1 - 适用于字幕没有边线或者字幕颜色比较浅的视频，但是会生成大量无用的图片
*- "Use Filter Colors"颜色滤镜可以极大提高结果的准确性：
   丢轴、错轴的情况会极大的改善，图片二值化后的结果也会更好
*- 字幕的分割也受以下两个因素影响：
   vedges_points_line_error = 0.3
   ila_points_line_error = 0.3
   0.3意为两个相近字幕的允许差异值为30%，比如：两条不同的字幕都只有十个字，在允许差异值为30%的情况下，其中一条字幕只要有七个字以上与另一条字幕相同，就视为同一条字幕。
   同理，如果允许差异值为50%，则只需要五个字以上与另一条字幕相同就可视为同一条字幕
   值越高，时间轴分割的越少
   值越低，时间轴分割的越多
   一般情况下，不推荐改动，只有在没有连续的字幕的情况下，才去降低参数值。

#---------二值化图片---------
*- 检查字幕颜色是否更深，如果不是，取消选择"Settings"标签页中右边第一项"Characters Border Is Darker"的复选框

大多数情况下，软件能准确识别与字幕有关的颜色，但某些特殊复杂情况，则需要考虑一下参数设置：
*- 二值化过程中部分字符丢失
   与"Moderate Threshold For Scaled Image"这个参数有关，参数范围[0.1-0.25]
   0.25 - 大多数视频通用，如果有字符丢失，适当调低参数
   0.1-0.15 - 适用于字幕边线不明显的或没有字幕边线的、字幕颜色与背景颜色相近的
   推荐打开"Use ILAImages before clear TXT images from borders"选项，能改善结果
*- 减少二值化过程中多余符号的产生
   "Clear Images Logical"默认打开，但是可能会移除部分正确的字符

*- 其他能帮助二值化图片的选项
    识别的语言不是符号、阿拉伯语、手写字体
    字符是完整的，有着稳定的亮度
    使用"Use Outline Filter Colors"——边线颜色滤镜
    如果使用了颜色滤镜，以及提取的ILAImages质量很好，推荐打开"Use ILAImages for getting TXT symbols areas"选项，可以减少多余符号的生成
    打开"Remove too wide symbols"选项，减少多余字符的产生

2) 颜色滤镜参数详解
*- 常用颜色滤镜参数范围
Lab: l:180-255 a:108-148 b:108-148  (大多数视频通用)
Lab: l:200-255 a:118-138 b:118-138
Lab: l:220-255 a:118-138 b:118-138 (强力颜色滤镜，视频质量很好的情况下使用，字幕亮度不稳定不推荐使用)

参数格式：
Lab: l:l_val a:a_val b:b_lab_val
Lab: l:min_l_val-max_l_val a:min_a_val-max_a_val b:min_b_lab_val-max_b_lab_val
RGB: r:min_r_val-max_r_val g:min_g_val-max_g_val b:min_b_val-max_b_val
RGB: r:r_val g:g_val b:b_val L:l_val
RGB: r:r_val g:g_val b:b_val L:min_l_val-max_l_val

*- 边线的颜色参数范围同上，除非边线很明显，否则"Use Outline Filter Colors"选项不建议使用强力滤镜
*- 在"Video Box"中，按"T"可全屏查看设置颜色滤镜后效果
    可通过左右方向键和空格键来查看不同帧
* - 字体颜色是红色的
* - 边线颜色是绿色的
* - 字体和边线重合部分是黄色的

同样：
按"U"可全屏查看原始视频帧
按"Y"查看原视频中剥离出来的字体的颜色，比如：字体是黄色的，屏幕上应该就只出现黄色的字幕
按"I"查看原视频中剥离出来的边线的颜色
*- 如果使用"Use Outline Filter Colors"或ILAImages图像质量不错，推荐打开"Use ILAImages for getting TXT symbols areas"，可减少无效字符的出现

3) 二值化质量不高的视频
* - 字幕亮度不稳定的情况
     打开"Extend By Grey Color"，人工设定"Allow Min Luminance"，最低亮度的值
     如果使用颜色滤镜，建议把最低亮度的值设成跟颜色滤镜的"min_l_val"相等
     参数范围：[min("允许的最低亮度", 自动检测的最低亮度), 自动检测的最高亮度]

"Video Gamma"和"Video Contrast"对结果也有帮助
某些情况下，设置"Video Gamma" == 0.7、"Allow Min Luminance" == 100、打开"Extend By Grey Color"，会很有帮助
具体情况中，需要自己不断测试才能得到最优的参数值

* - 借助第三方图片增强软件
     使用"Topaz Gigapixel AI": https://topazlabs.com/gigapixel-ai/，对提取的RGB图片进行增强
     建议按默认的两倍放大增强，由于图片的名称就是字幕的时间轴，增强后的图片名称应与原来的一致

4) 二值化没有边线的字幕
这种情况下，某些字符的分离会变得很困难
可以打开"Use ILAImages before clear TXT images from borders"选项，会很有帮助
正确设置合适的字幕颜色滤镜
并且，在背景是动态的情况下，ILA images会很有帮助
以下列举的参数对背景分离也很有帮助：
"Min Sum Color Difference": min_sum_color_diff = 0
"Moderate Threshold": moderate_threshold = 0.1
"Moderate Threshold For Scaled Image": moderate_threshold_for_scaled_image = 0.1
"Use ILAImages for getting TXT symbols areas" - Turn Off: use_ILA_images_for_getting_txt_symbols_areas = 0

5) 一条字幕有多种颜色
用"Ctrl+Enter"在"Use Filter Colors"编辑栏中添加多条颜色参数后，打开"Combine To Single Cluster"选项

6) 提高软件二值化过程中的运行性能
默认的参数为：
"CPU kmeans initial loop iterations" == 20
"CPU kmeans loop iterations" = 30
以上参数适应于各种情况，特别是在字幕没有良好的边线的情况下，当字幕有明显的边线的时候，两个指标的参数都可以设为10。

#--------------------常用术语--------------------
*- ISAImages -  Intersected Subtitles Areas 交叉字幕区域
*- ILAImages - Intersected Luminance Areas 相交亮度区域

#--------------------后记--------------------
翻译：豆瓣@我在成都养熊猫
相较于英文版教程，进行了部分筛减和补充，完整版请查看英文版本
欢迎加入硬字幕提取交流群：1161766056
