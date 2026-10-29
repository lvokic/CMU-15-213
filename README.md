# CMU 15-213 学习实验

本仓库包含 Data、Bomb、Attack、Cache、Shell、Malloc、Proxy 和 SFS Lab 的练习起始代码及题目。每个实验的 PDF 位于对应目录。SFS 是单独提供的扩展练习。

课程资源、讲次与实验的对应关系、环境配置和运行命令见 [学习指南](学习指南.md)。

部分题目的工具脚本已适配 Python 3，Malloc Lab 补充了测试 trace。两项 32 位实验会自动使用项目内的本地工具链；也可通过环境变量 `CSAPP_GCC32` 指定编译器包装器。没有本地工具链时使用系统 GCC。
