# TinyC

这是一份编译原理课程设计，由小组成员合作完成。

使用的第三方库：

* [WinUIEdit](https://github.com/BreeceW/WinUIEdit) (UWP port of Scintilla)
* Win32Xaml (XAML Islands enhancement library)

## 构建

需要 Visual Studio 2022 以及 C++ 和 UWP 工作负荷。

如果执行 package restore 时无法复原 Win32Xaml 包，则需要在 Visual Studio 中手动添加 PrivPkgs 作为本地源。