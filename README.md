# ustb-cli (C version)

bash版看这里：https://github.com/Jason23347/ustb-cli

## 性能提升

相比bash版，本项目主打一个小**个屁勒**，原先满打满算9K现在面向x86_64的二进制要40K，面向OpenWrt rockchip/armv8的二进制要67K……
运行速度方面，在ImmortalWrt 24.10 rockchip/armv8 (R5C)上测试，本程序比bash版快大约50%。100ms的提升估计没啥感觉……

### benchmark (info)
```bash
seq ${NUM:-10} |
	while read i; do {
		time ./ustb-cli info >/dev/null;
	} 2>&1; done |
	grep real |
	awk -F'm|s' '{sum+=($1*60+$2)} END{print "avg:",sum/NR,"s"}'
```

| 重复次数（NUM） | C版平均速度 (s) | bash版平均速度 (s) |
| :-------------: | :-------------: | :----------------: |
|       10        |      0.119      |       0.256        |
|       50        |      0.129      |       0.210        |
|       100       |      0.123      |       0.248        |

## 使用方法

```bash
ustb-cli help # 列出所有指令
ustb-cli login -h # 显示某一指令的帮助信息
ustb-cli speedtest -c=200 --upload -db # 基于cargs支持长短参数解析
```

## 编译

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
```

### 编译到OpenWrt (MUSL)

目前只准备了面向arm64架构软路由的工具链文件。有需要可以自行修改。

#### 准备OpenWrt SDK

下载安装官方SDK压缩包，例如
```bash
wget https://downloads.immortalwrt.org/releases/24.10.2/targets/rockchip/armv8/immortalwrt-sdk-24.10.2-rockchip-armv8_gcc-13.3.0_musl.Linux-x86_64.tar.zst
```

解压、安装`libopenssl`、编译

```bash
tar --zstd -xf immortalwrt-sdk-24.10.2-rockchip-armv8_gcc-13.3.0_musl.Linux-x86_64.tar.zst
cd immortalwrt-sdk-24.10.2-rockchip-armv8_gcc-13.3.0_musl.Linux-x86_64
mkdir host # 因为提示touch指令出错
./scripts/feeds update base
./scripts/feeds install libopenssl
make package/feeds/base/openssl/compile V=s
```

然后在`ustb-cli-c`下编译，
其中`<OpenWrt SDK path>`就是刚才编译的SDK的路径，例如`$HOME/immortalwrt-sdk-24.10.2-rockchip-armv8_gcc-13.3.0_musl.Linux-x86_64`。

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
```

### 其他平台

你自己写toolchain file吧...

## 代码补全

目前只适配了zsh，后续还将适配bash

把下面这段代码放入`~/.zshrc`

```zsh
eval "$(ustb-cli completion zsh)"
```
