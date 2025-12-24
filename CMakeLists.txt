# CMake 最低版本要求
cmake_minimum_required(VERSION 3.20)

# 项目名称和语言设置
project(nginx C)

# 设置 C 标准（nginx 通常使用 C99 或更高版本）
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 查找 PCRE2 库的头文件路径
find_path(PCRE2_INCLUDE_DIR
	NAMES pcre2.h
	PATHS
		/opt/homebrew/include                # Homebrew (Apple Silicon)
		/usr/local/include                   # 标准安装路径
		/usr/include                         # 系统路径
		/opt/homebrew/Cellar/pcre2/*/include # Homebrew 特定版本路径（回退）
	PATH_SUFFIXES pcre2
)

# 查找 PCRE2 库文件
find_library(PCRE2_LIBRARY
	NAMES pcre2-8
	PATHS
		/opt/homebrew/lib                    # Homebrew (Apple Silicon)
		/usr/local/lib                       # 标准安装路径
		/usr/lib                             # 系统路径
		/opt/homebrew/Cellar/pcre2/*/lib     # Homebrew 特定版本路径（回退）
)

# 检查是否找到 PCRE2
if(NOT PCRE2_INCLUDE_DIR OR NOT PCRE2_LIBRARY)
	message(FATAL_ERROR "未找到 PCRE2 库。请安装 PCRE2 开发包。")
endif()

message(STATUS "找到 PCRE2 头文件: ${PCRE2_INCLUDE_DIR}")
message(STATUS "找到 PCRE2 库文件: ${PCRE2_LIBRARY}")

# 设置包含目录（头文件搜索路径）
include_directories(
		.                                    # 项目根目录
		src/core                             # 核心模块
		src/event                            # 事件模块
		src/event/modules                    # 事件处理模块
		src/os/unix                          # Unix 系统相关
		objs                                 # 自动生成的文件目录
		src/http                             # HTTP 模块
		src/http/modules                     # HTTP 子模块
		${PCRE2_INCLUDE_DIR}                 # PCRE2 正则表达式库头文件（自动查找）
)

# 定义所有源文件
set(SOURCE_FILES
	# 自动生成的模块文件
	objs/ngx_modules.c
	
	# 核心模块源文件
	src/core/nginx.c 
	src/core/ngx_log.c 
	src/core/ngx_palloc.c 
	src/core/ngx_array.c 
	src/core/ngx_list.c 
	src/core/ngx_hash.c 
	src/core/ngx_buf.c 
	src/core/ngx_queue.c 
	src/core/ngx_output_chain.c 
	src/core/ngx_string.c 
	src/core/ngx_parse.c 
	src/core/ngx_parse_time.c 
	src/core/ngx_inet.c 
	src/core/ngx_file.c 
	src/core/ngx_crc32.c 
	src/core/ngx_murmurhash.c 
	src/core/ngx_md5.c 
	src/core/ngx_sha1.c 
	src/core/ngx_rbtree.c 
	src/core/ngx_radix_tree.c 
	src/core/ngx_slab.c 
	src/core/ngx_times.c 
	src/core/ngx_shmtx.c 
	src/core/ngx_connection.c 
	src/core/ngx_cycle.c 
	src/core/ngx_spinlock.c 
	src/core/ngx_rwlock.c 
	src/core/ngx_cpuinfo.c 
	src/core/ngx_conf_file.c 
	src/core/ngx_module.c 
	src/core/ngx_resolver.c 
	src/core/ngx_open_file_cache.c 
	src/core/ngx_crypt.c 
	src/core/ngx_proxy_protocol.c 
	src/core/ngx_syslog.c 
	src/core/ngx_regex.c 
	
	# 事件模块源文件
	src/event/ngx_event.c 
	src/event/ngx_event_timer.c 
	src/event/ngx_event_posted.c 
	src/event/ngx_event_accept.c 
	src/event/ngx_event_udp.c 
	src/event/ngx_event_connect.c 
	src/event/ngx_event_pipe.c 
	src/event/modules/ngx_kqueue_module.c  # macOS 的 kqueue 事件模型
	
	# 操作系统相关源文件（Unix）
	src/os/unix/ngx_time.c 
	src/os/unix/ngx_errno.c 
	src/os/unix/ngx_alloc.c 
	src/os/unix/ngx_files.c 
	src/os/unix/ngx_socket.c 
	src/os/unix/ngx_recv.c 
	src/os/unix/ngx_readv_chain.c 
	src/os/unix/ngx_udp_recv.c 
	src/os/unix/ngx_send.c 
	src/os/unix/ngx_writev_chain.c 
	src/os/unix/ngx_udp_send.c 
	src/os/unix/ngx_udp_sendmsg_chain.c 
	src/os/unix/ngx_channel.c 
	src/os/unix/ngx_shmem.c 
	src/os/unix/ngx_process.c 
	src/os/unix/ngx_daemon.c 
	src/os/unix/ngx_setaffinity.c 
	src/os/unix/ngx_setproctitle.c 
	src/os/unix/ngx_posix_init.c 
	src/os/unix/ngx_user.c 
	src/os/unix/ngx_dlopen.c 
	src/os/unix/ngx_process_cycle.c 
	src/os/unix/ngx_darwin_init.c              # macOS 初始化
	src/os/unix/ngx_darwin_sendfile_chain.c    # macOS sendfile 实现
	
	# HTTP 核心模块源文件
	src/http/ngx_http.c 
	src/http/ngx_http_core_module.c 
	src/http/ngx_http_special_response.c 
	src/http/ngx_http_request.c 
	src/http/ngx_http_parse.c 
	src/http/ngx_http_request_body.c 
	src/http/ngx_http_variables.c 
	src/http/ngx_http_script.c 
	src/http/ngx_http_upstream.c 
	src/http/ngx_http_upstream_round_robin.c 
	src/http/ngx_http_file_cache.c 
	src/http/ngx_http_write_filter_module.c 
	src/http/ngx_http_header_filter_module.c 
	src/http/ngx_http_postpone_filter_module.c 
	src/http/ngx_http_copy_filter_module.c 
	
	# HTTP 子模块源文件
	src/http/modules/ngx_http_log_module.c 
	src/http/modules/ngx_http_chunked_filter_module.c 
	src/http/modules/ngx_http_range_filter_module.c 
	src/http/modules/ngx_http_gzip_filter_module.c 
	src/http/modules/ngx_http_ssi_filter_module.c 
	src/http/modules/ngx_http_charset_filter_module.c 
	src/http/modules/ngx_http_userid_filter_module.c 
	src/http/modules/ngx_http_headers_filter_module.c 
	src/http/modules/ngx_http_not_modified_filter_module.c 
	src/http/modules/ngx_http_static_module.c 
	src/http/modules/ngx_http_autoindex_module.c 
	src/http/modules/ngx_http_index_module.c 
	src/http/modules/ngx_http_mirror_module.c 
	src/http/modules/ngx_http_try_files_module.c 
	src/http/modules/ngx_http_auth_basic_module.c 
	src/http/modules/ngx_http_access_module.c 
	src/http/modules/ngx_http_limit_conn_module.c 
	src/http/modules/ngx_http_limit_req_module.c 
	src/http/modules/ngx_http_geo_module.c 
	src/http/modules/ngx_http_map_module.c 
	src/http/modules/ngx_http_split_clients_module.c 
	src/http/modules/ngx_http_referer_module.c 
	src/http/modules/ngx_http_rewrite_module.c 
	src/http/modules/ngx_http_proxy_module.c 
	src/http/modules/ngx_http_fastcgi_module.c 
	src/http/modules/ngx_http_uwsgi_module.c 
	src/http/modules/ngx_http_scgi_module.c 
	src/http/modules/ngx_http_memcached_module.c 
	src/http/modules/ngx_http_empty_gif_module.c 
	src/http/modules/ngx_http_browser_module.c 
	src/http/modules/ngx_http_upstream_hash_module.c 
	src/http/modules/ngx_http_upstream_ip_hash_module.c 
	src/http/modules/ngx_http_upstream_least_conn_module.c 
	src/http/modules/ngx_http_upstream_random_module.c 
	src/http/modules/ngx_http_upstream_keepalive_module.c 
	src/http/modules/ngx_http_upstream_zone_module.c
)

# 创建可执行文件
add_executable(nginx ${SOURCE_FILES})

# 链接外部库
# PCRE2_LIBRARY: PCRE2 正则表达式库（8位字符版本，自动查找）
# z: zlib 压缩库（系统库，CMake 会自动查找）
target_link_libraries(nginx PRIVATE ${PCRE2_LIBRARY} z)


