   # 什么是 **Auto_Disable** ？

**Auto_Disable** 是一个自动检测前台配置 **App** 并冻结指定 **App** 的模块，支持多配置文件。



   ## 编写自定义配置文件：

配置目录：模块根目录下 **CONFIG** 目录（此目录不强制绑定Server，但模块service.sh默认指定这个，如果您是自定义启动Server请忽略）

新建文本文件，不限后缀

第一行开头填写`@<检测App包名>`

之后一行一个待冻结软件包名


**模块会自动检查前台软件，当监测到 **检测App** 在前台运行时自动冻结该列表内其它软件，
当前台离开被检测软件后自动解冻**



模块核心 **AutoDisableServer** 支持以 **Shell** 权限运行，将 **AutoDisableServer** 
复制进 **`/data/local/tmp`** 使用 **`/data/local/tmp/AutoDisableServer <配置文件所在路径>`** 
命令即可正常启动 **Server** 。暂未考虑做方便实现



核心部分全 **C** 编写，不会占用系统资源
