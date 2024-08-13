
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
	/Users/zy.yuan/Develop/nginx/cmake-build-debug/nginx-file/sbin/nginx -t

	kill -USR2 `cat /Users/zy.yuan/Develop/nginx/cmake-build-debug/nginx-file/logs/nginx.pid`
	sleep 1
	test -f /Users/zy.yuan/Develop/nginx/cmake-build-debug/nginx-file/logs/nginx.pid.oldbin

	kill -QUIT `cat /Users/zy.yuan/Develop/nginx/cmake-build-debug/nginx-file/logs/nginx.pid.oldbin`

.PHONY:	build install modules upgrade
