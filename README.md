
```shell
./auto/configure --prefix=/Users/zy.yuan/Develop/nginx/cmake-build-debug/nginx-file

mkdir cmake-build-debug/nginx-file

cp -rf conf cmake-build-debug/nginx-file/.
cp -rf docs/html cmake-build-debug/nginx-file/.
```

```shell
include_directories(
		.
		src/core src/event src/event/modules / src/os/unix objs src/http src/http/modules
		/opt/homebrew/Cellar/pcre2/10.43/include
		/opt/homebrew/Cellar/pcre2/10.44/include
)
```

#### 用 find，有点问题
```shell
find_package(PCRE2 REQUIRED COMPONENTS 8BIT)
include_directories(${PCRE2_INCLUDE_DIRS})
...
target_link_libraries(nginx ${PCRE2_LIBRARIES})
```

```
daemon off;
master_process off;
```

### 增加 mymodules
```shell
./auto/configure --prefix=/Users/yuanzhaoyi/Develop/nginx/cmake-build-debug/nginx-file --add-module=src/mymodules
```

```
./auto/configure --prefix=/Users/yuanzhaoyi/Develop/nginx/cmake-build-debug/nginx-file  --with-threads --add-module=src/mymodules
```