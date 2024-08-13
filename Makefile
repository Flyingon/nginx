
default:	build

clean:
	rm -rf Makefile objs

.PHONY:	default clean

build:
	$(MAKE) -f objs/Makefile

install:
	$(MAKE) -f objs/Makefile install

modules:
	$(MAKE) -f objs/Makefile modules

upgrade:
	/Users/yuanzhaoyi/Develop/nginx/cmake-build-debug/nginx-file/sbin/nginx -t

	kill -USR2 `cat /Users/yuanzhaoyi/Develop/nginx/cmake-build-debug/nginx-file/logs/nginx.pid`
	sleep 1
	test -f /Users/yuanzhaoyi/Develop/nginx/cmake-build-debug/nginx-file/logs/nginx.pid.oldbin

	kill -QUIT `cat /Users/yuanzhaoyi/Develop/nginx/cmake-build-debug/nginx-file/logs/nginx.pid.oldbin`

.PHONY:	build install modules upgrade
