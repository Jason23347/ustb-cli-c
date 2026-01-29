# ustb-cli (C version)

北京科技大学校园网命令行工具，C语言重写版本。

bash版看这里：https://github.com/Jason23347/ustb-cli

## 功能特性

### 账户管理

| 命令      | 功能         | 说明                                 |
| --------- | ------------ | ------------------------------------ |
| `login`   | 登录校园网   | 支持IPv6自动获取、交互式登录         |
| `logout`  | 登出校园网   | -                                    |
| `whoami`  | 显示当前用户 | 支持显示用户名、NID、完整信息        |
| `devices` | 查看在线设备 | 支持watch模式、Markdown输出（需SSL） |

### 流量与费用

| 命令   | 功能         | 说明                                     |
| ------ | ------------ | ---------------------------------------- |
| `info` | 显示账户信息 | IPv4/IPv6地址、已用/剩余流量、流量节省率 |
| `fee`  | 显示费用信息 | 本月消费、账户余额                       |

### 网络测速

| 命令        | 功能     | 说明                              |
| ----------- | -------- | --------------------------------- |
| `speedtest` | 内网测速 | 多线程上传/下载测速、ping延迟测试 |
| `monitor`   | 流量监控 | 实时显示下载速度                  |

### 其他

| 命令         | 功能      | 说明          |
| ------------ | --------- | ------------- |
| `completion` | Shell补全 | 支持zsh、bash |
| `help`       | 显示帮助  | -             |
| `version`    | 显示版本  | -             |

## 技术特性

- **C11标准** - 现代C语言特性
- **彩色输出** - 可选的终端彩色显示
- **多线程测速** - 支持多线程并发测速
- **OpenSSL支持** - HTTPS连接、MD5计算
- **交互式输入** - 基于linenoise的密码输入
- **跨平台编译** - 支持x86_64、ARM64、OpenWrt
- **GB2312解码** - 支持iconv或内置解码器
- **命令行解析** - 基于cargs库，支持长短参数

## 性能提升

相比bash版，运行速度提升约50%。在ImmortalWrt 24.10 rockchip/armv8 (R5C)上测试：

| 重复次数 | C版平均速度 (s) | bash版平均速度 (s) |
| :------: | :-------------: | :----------------: |
|    10    |      0.119      |       0.256        |
|    50    |      0.129      |       0.210        |
|   100    |      0.123      |       0.248        |

<details>
<summary>benchmark脚本</summary>

```bash
seq ${NUM:-10} |
	while read i; do {
		time ./ustb-cli info >/dev/null;
	} 2>&1; done |
	grep real |
	awk -F'm|s' '{sum+=($1*60+$2)} END{print "avg:",sum/NR,"s"}'
```

</details>

## 使用方法

```bash
# 显示帮助
ustb-cli help
ustb-cli login -h

# 登录（从~/.ustb.env读取账号密码）
ustb-cli login

# 交互式登录
ustb-cli login -m

# 查看账户信息
ustb-cli info
ustb-cli fee
ustb-cli whoami -a

# 查看在线设备
ustb-cli devices -f          # 显示格式化MAC地址
ustb-cli devices -w -t 5     # watch模式，5秒刷新

# 内网测速
ustb-cli speedtest                    # 上传+下载测速
ustb-cli speedtest -d -c=200 -j=8     # 200MB下载，8线程
ustb-cli speedtest -p                 # ping延迟测试
ustb-cli speedtest -u -b              # 上传测速，显示Mbps

# 实时监控
ustb-cli monitor
```

### 账号配置

创建 `~/.ustb.env` 文件：

```
USTB_USERNAME=你的学号
USTB_PASSWORD=你的密码
```

## 编译

### 编译选项

| 选项              | 说明                                   | 默认值 |
| ----------------- | -------------------------------------- | ------ |
| `WITH_COLOR`      | 彩色输出                               | ON     |
| `WITH_ACCOUNT`    | 账户功能 (login/logout/whoami/devices) | ON     |
| `WITH_BALANCE`    | 余额功能 (info/fee)                    | ON     |
| `WITH_SPEEDTEST`  | 测速功能 (speedtest/monitor)           | ON     |
| `WITH_COMPLETION` | Shell补全                              | OFF    |
| `USE_INTERACTIVE` | 交互式输入 (linenoise)                 | OFF    |
| `USE_THREADS`     | 多线程测速                             | ON     |
| `GB2312_DECODER`  | GB2312解码方式：iconv/builtin/disabled | iconv  |

### 本机编译

```shell
mkdir -p build && cd build
cmake .. \
	-DWITH_COLOR=on \
	-DWITH_BALANCE=on \
	-DWITH_ACCOUNT=on \
	-DWITH_SPEEDTEST=on \
	-DWITH_COMPLETION=on \
	-DUSE_INTERACTIVE=on \
	-DGB2312_DECODER="iconv"
make
```

### x86_64交叉编译到ARM64

```shell
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../aarch64-toolchain.cmake \
	-DWITH_COLOR=on \
	-DWITH_BALANCE=on \
	-DWITH_ACCOUNT=on \
	-DWITH_SPEEDTEST=on \
	-DWITH_COMPLETION=on \
	-DUSE_INTERACTIVE=on \
	-DGB2312_DECODER="iconv"
make
```

### 编译到OpenWrt (MUSL)

目前只准备了面向arm64架构软路由的工具链文件。有需要可以自行修改。

<details>
<summary>准备OpenWrt SDK</summary>

下载安装官方SDK压缩包，例如：

```bash
wget https://downloads.immortalwrt.org/releases/24.10.2/targets/rockchip/armv8/immortalwrt-sdk-24.10.2-rockchip-armv8_gcc-13.3.0_musl.Linux-x86_64.tar.zst
```

解压、安装`libopenssl`、编译：

```bash
tar --zstd -xf immortalwrt-sdk-24.10.2-rockchip-armv8_gcc-13.3.0_musl.Linux-x86_64.tar.zst
cd immortalwrt-sdk-24.10.2-rockchip-armv8_gcc-13.3.0_musl.Linux-x86_64
mkdir host # 因为提示touch指令出错
./scripts/feeds update base
./scripts/feeds install libopenssl
make package/feeds/base/openssl/compile V=s
```

</details>

在`ustb-cli-c`下编译，其中`<OpenWrt SDK path>`就是刚才编译的SDK的路径：

```shell
mkdir -p build && cd build
export STAGING_DIR=<OpenWrt SDK path>/staging_dir
cmake .. \
	-DCMAKE_TOOLCHAIN_FILE=../aarch64-openwrt-toolchain.cmake \
	-DWITH_COLOR=on \
	-DWITH_BALANCE=on \
	-DWITH_ACCOUNT=on \
	-DWITH_SPEEDTEST=on \
	-DWITH_COMPLETION=off \
	-DGB2312_DECODER="disabled"
make
```

### 其他平台

需自行编写toolchain file。

## Shell补全

### Zsh

将以下代码添加到 `~/.zshrc`：

```zsh
eval "$(ustb-cli completion zsh)"
```

### Bash

将以下代码添加到 `~/.bashrc`：

```bash
eval "$(ustb-cli completion bash)"
```

## 许可证

MIT License
