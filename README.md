   # 什么是 **Auto_Disable** ？

**Auto_Disable** 是一个自动检测前台配置 **App** 并冻结指定 **App** 的模块。



配置文件填写：

检测 **App** 列表：**`CheckList.conf`**

一行一个包名，当模块监测到其中某软件处于前台运行状态则冻结以下列表内软件：

冻结 **App** 列表：**`DisableList.conf`**

一行一个包名，当模块监测到 **`CheckList.conf`** 中某软件处于前台运行状态则冻结该列表内软件

配置更改实时生效，无需重启设备/进程



模块会自动检查前台软件，当监测到 **`CheckList.conf`** 内软件在前台运行时自动冻结 **`DisableList.conf`** 内填写的软件，当前台离开 **`CheckList.conf`** 监测的软件后自动解冻 **`DisableList.conf`** 内填写的软件



模块核心 **`AutoDisableServer`** 实际支持以 **Shell** 权限运行，将 **`AutoDisableServer`** 复制进 **`/data/local/tmp`** 使用 **`/data/local/tmp/AutoDisableServer <配置文件所在路径>`** 命令即可正常启动 **Server** 。暂未考虑做方便实现



核心部分全 **C** 编写，不会占用系统资源
