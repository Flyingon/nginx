
```shell
./auto/configure --prefix=/Users/yuanzhaoyi/Develop/nginx/cmake-build-debug/nginx-file --with-threads

mkdir -p cmake-build-debug/nginx-file/logs

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

#### 配置
```
daemon off;
master_process off;

    location /guaguatuweb {
            alias /Users/yuanzhaoyi/Develop/github.com/Flyingon/GuaguatuWebsite; # 替换为你的单页应用的根目录
            try_files $uri $uri/ /guaguatuweb/index.html;
    }
```

### 增加 mymodules
#### mac
```
./auto/configure --prefix=/Users/yuanzhaoyi/Develop/nginx/cmake-build-debug/nginx-file --add-module=src/mymodules
```

```
./auto/configure --prefix=/Users/yuanzhaoyi/Develop/nginx/cmake-build-debug/nginx-file  --with-threads --add-module=src/mymodules
```

#### ubuntu
- 下载一些 lib
```
sudo apt install libpcre3 libpcre3-dev
sudo apt-get install sqlite3 libsqlite3-dev
sudo apt install libssl-dev
```
- 下载 brotli
```
apt install -y brotli libbrotli-dev

git clone https://github.com/google/ngx_brotli.git
cd ngx_brotli/
git submodule update --init
```
- 配置，添加 http_ssl_module、ngx_brotli、mymodules
```
./auto/configure --prefix=/root/services/nginx --with-http_ssl_module --add-module=../ngx_brotli --add-module=src/mymodules --with-cc-opt="-I/usr/include" --with-ld-opt="-L/../usr/lib/x86_64-linux-gnu/ -lsqlite3"
```
- 编译和链接
```
make -j$(nproc)  # 编译用 gcc
sed -i 's/^CC =\s*cc/CC =\tg++/' objs/Makefile  # 链接用 g++
```

