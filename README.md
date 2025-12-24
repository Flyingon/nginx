# Nginx 开发环境

本项目是 Nginx Web 服务器的开发版本，支持使用 CMake 或传统的 configure 脚本进行构建。

## 前置依赖

在编译之前，请确保已安装以下依赖：

- **PCRE2**: 正则表达式库（必需）
  ```bash
  # macOS (Homebrew)
  brew install pcre2
  
  # 或使用系统包管理器安装
  ```

- **zlib**: 压缩库（通常系统自带）

## 构建方式

### 方式一：使用 CMake 构建（推荐）

CMake 构建方式更加灵活，支持跨平台，并且会自动查找依赖库。

```shell
# 运行配置脚本，指定安装前缀路径
# --prefix 参数指定 nginx 的安装根目录，要从这里面读取配置
./auto/configure --prefix=~/Develop/nginx/cmake-build-debug/nginx-file

# 创建构建目录
mkdir -p cmake-build-debug
cd cmake-build-debug

# 运行 CMake 配置
cmake ..

# 编译
make

# 或者使用 CMake 的构建命令
cmake --build .
```

编译完成后，可执行文件位于：`cmake-build-debug/nginx`

### 方式二：使用传统 configure 脚本构建

这是 Nginx 官方的标准构建方式。

```shell
# 运行配置脚本，指定安装前缀路径
# --prefix 参数指定 nginx 的安装根目录
./auto/configure --prefix=${HOME}/Develop/nginx/cmake-build-debug/nginx-file

# 编译（使用 make）
make

# 安装（可选，会将文件复制到 --prefix 指定的目录）
make install
```

## 运行和部署

### 准备运行环境

在运行 nginx 之前，需要准备配置文件和静态资源：

```shell
# 创建目标目录（如果使用 configure 方式）
mkdir -p cmake-build-debug/nginx-file/log

# 复制配置文件到目标目录
cp -rf conf cmake-build-debug/nginx-file/.

# 复制 HTML 文档到目标目录
cp -rf docs/html cmake-build-debug/nginx-file/.
```

### 启动 Nginx

```shell
# 如果使用 CMake 构建
./cmake-build-debug/nginx -p cmake-build-debug/nginx-file

# 如果使用 configure 构建
./cmake-build-debug/nginx-file/sbin/nginx
```

### 停止 Nginx

```shell
# 查找 nginx 进程
ps aux | grep nginx

# 优雅停止（发送 QUIT 信号）
kill -QUIT <主进程PID>

# 或快速停止（发送 TERM 信号）
kill -TERM <主进程PID>
```

## 调试配置

在开发调试时，建议在 `nginx.conf` 的 `main` 上下文（最外层）添加以下配置：

```nginx
# 关闭守护进程模式
# 这样 nginx 会在前台运行，方便查看日志输出
daemon off;

# 关闭主进程模式
# 这样只会启动一个工作进程，便于调试（如使用 GDB）
# 注意：生产环境不要使用此配置
master_process off;
```

### 调试配置说明

- **`daemon off;`**: 
  - 默认情况下，nginx 会以守护进程（后台进程）方式运行
  - 设置为 `off` 后，nginx 会在前台运行，所有日志直接输出到终端
  - 方便开发时查看实时日志和错误信息

- **`master_process off;`**: 
  - 默认情况下，nginx 使用主进程+工作进程的多进程模型
  - 设置为 `off` 后，只运行单个进程，便于使用调试器（如 GDB）进行调试
  - ⚠️ **警告**: 此配置会禁用 nginx 的多进程特性，仅用于开发调试，生产环境必须设置为 `on`

### 使用 GDB 调试

```shell
# 启动 GDB
gdb ./cmake-build-debug/nginx

# 在 GDB 中设置参数
(gdb) set args -p cmake-build-debug/nginx-file

# 设置断点
(gdb) break ngx_http_process_request

# 运行
(gdb) run
```

## 项目结构

```
nginx/
├── src/              # 源代码目录
│   ├── core/        # 核心模块
│   ├── event/       # 事件处理模块
│   ├── http/        # HTTP 模块
│   └── os/          # 操作系统相关代码
├── conf/            # 配置文件目录
├── docs/            # 文档目录
├── auto/            # 自动配置脚本
├── CMakeLists.txt   # CMake 构建配置文件
└── objs/            # 编译输出目录（自动生成）
```

## 常见问题

### 1. 找不到 PCRE2 库

如果 CMake 构建时提示找不到 PCRE2：

```bash
# macOS
brew install pcre2

# 或手动指定路径（在 CMakeLists.txt 中已包含常见路径）
```

### 2. 端口被占用

```bash
# 查看端口占用情况
lsof -i :80

# 修改 nginx.conf 中的 listen 端口
```

### 3. 权限问题

```bash
# 如果使用 1024 以下端口，需要 root 权限
sudo ./nginx -p /path/to/nginx
```

## 参考资源

- [Nginx 官方文档](https://nginx.org/en/docs/)
- [Nginx 开发指南](https://nginx.org/en/docs/dev/development_guide.html)