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
	-DWITH_BALANCE=on -DWITH_ACCOUNT=on -DWITH_SPEEDTEST=on
```

### x86_64交叉编译到ARM64

```shell
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../aarch64-toolchain.cmake \
	-DWITH_COLOR=on \
	-DWITH_BALANCE=on -DWITH_ACCOUNT=on -DWITH_SPEEDTEST=on
```

### 编译到OpenWrt (MUSL)

```shell
mkdir -p build && cd build
export STAGING_DIR=<OpenWrt SDK path>/staging_dir
cmake .. -DCMAKE_TOOLCHAIN_FILE=../aarch64-openwrt-toolchain.cmake \
	-DWITH_COLOR=on \
	-DWITH_BALANCE=on -DWITH_ACCOUNT=on -DWITH_SPEEDTEST=on
```

例如我的`OPENWRT_ROOT`为`$HOME/immortalwrt-sdk-24.10.2-rockchip-armv8_gcc-13.3.0_musl.Linux-x86_64`

### 其他平台

你自己写toolchain file吧...
