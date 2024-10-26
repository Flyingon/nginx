
### ngx_http_click_tracker.cpp
#### 记录点击页面次数到 sqlite3
#### 使用
会记录点击 /tools/images/ 或 /tools/json/ 页面的 ip 和 次数
```
        location / {
            charset   utf-8;
            alias /data/;
            autoindex on;
            click_tracker /data/click_tracker.db /tools/images/,/tools/json/;
        }
```
#### 备注
- 目前是同步记录，即先记录再打开页面