extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <sqlite3.h>
#include <ngx_thread_pool.h>
}
#include <iostream>
#include <cstring>
#include <map>

// 保存数据库的连接句柄
sqlite3 *db;
ngx_str_t dbPath;  // SQLite 数据库文件路径
std::map<std::string, bool> uriMap; // 存储

// 配置存储结构体
//typedef struct {
//    ngx_str_t dbPath; // 添加一个字段用于存储数据库路径
//} ngx_http_click_tracker_loc_conf_t;

static ngx_int_t ngx_http_click_tracker_init(ngx_conf_t *cf);

static void ngx_http_click_tracker_exit(ngx_cycle_t *cycle);

static char *ngx_http_click_tracker(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_int_t ngx_http_click_tracker_request_handler(ngx_http_request_t *r);


static ngx_command_t ngx_http_click_tracker_commands[] = {
        {
                ngx_string("click_tracker"),  // 配置指令
                NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF |
                NGX_CONF_TAKE12,  // 指令的作用范围
                ngx_http_click_tracker,  // 配置处理函数
                NGX_HTTP_LOC_CONF_OFFSET,
                0,
                NULL
        },
        ngx_null_command
};

static ngx_http_module_t ngx_http_click_tracker_module_ctx = {
        NULL,  // preconfiguration
        ngx_http_click_tracker_init,  // postconfiguration
        NULL,  // create main configuration
        NULL,  // init main configuration
        NULL,  // create server configuration
        NULL,  // merge server configuration
        NULL,  // create location configuration
        NULL,  // merge location configuration
};

ngx_module_t ngx_http_click_tracker_module = {
        NGX_MODULE_V1,
        &ngx_http_click_tracker_module_ctx,  // module context
        ngx_http_click_tracker_commands,  // module directives
        NGX_HTTP_MODULE,  // module type
        NULL,  // init master
        NULL,  // init module
        NULL,  // init process
        NULL,  // init thread
        NULL,  // exit thread
        ngx_http_click_tracker_exit,  // exit process
        NULL,  // exit master
        NGX_MODULE_V1_PADDING
};

// 模块初始化
static ngx_int_t ngx_http_click_tracker_init(ngx_conf_t *cf) {
    ngx_log_t *log = cf->log;

//    auto cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    // 初始化数据库连接
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, log, 0, "open sqlite3: %s", (char *)dbPath.data);
    if (sqlite3_open((char *)dbPath.data, &db) != SQLITE_OK) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "init sqlite3 failed %s", (char *)dbPath.data);
        return NGX_ERROR;
    }

    // 创建数据库表
    const char *sql = "CREATE TABLE IF NOT EXISTS page_clicks ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "uri TEXT,"
                      "ip TEXT,"
                      "count INTEGER DEFAULT 1,"
                      "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
                      "CONSTRAINT unique_uri_ip UNIQUE (uri, ip));";

    char *err_msg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        ngx_log_error(NGX_LOG_ERR, log, 0, err_msg);
        sqlite3_free(err_msg);
        return NGX_ERROR;
    }

    return NGX_OK;
}

// 模块退出函数
static void ngx_http_click_tracker_exit(ngx_cycle_t *cycle) {
    uriMap.clear();
    if (db != NULL) {
        sqlite3_close(db);
    }
    if (dbPath.data != NULL) {
        free(dbPath.data);      // 释放 data 指针指向的内存
        dbPath.data = NULL;      // 避免悬空指针
    }
}

// 配置处理函数
static char *ngx_http_click_tracker(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_str_t *value = static_cast<ngx_str_t *>(cf->args->elts);
    dbPath = value[1]; // 读取指令中的参数值

    if (cf->args->nelts == 3) {
        ngx_str_t url_paths = value[2];
        // 使用 strtok 分割字符串
        char *token = strtok((char *)url_paths.data, ",");
        // 轮询输出每个子字符串
        while (token != nullptr) {
            uriMap[token] = true;
            token = strtok(nullptr, ","); // 获取下一个分割的子字符串
        }
    }

    ngx_http_core_loc_conf_t *clcf;
    clcf = (ngx_http_core_loc_conf_t *) ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_click_tracker_request_handler;  // 将请求处理器绑定到配置指令


    return NGX_CONF_OK;
}

// 处理请求的 handler 函数
static ngx_int_t ngx_http_click_tracker_request_handler(ngx_http_request_t *r) {
    if (!(r->method & NGX_HTTP_GET)) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    // 执行 SQLite 插入操作的回调函数
    const char *sql = "INSERT INTO page_clicks (uri, ip, count) VALUES (?,?, 1) "
                      "ON CONFLICT(uri, ip) DO UPDATE SET count = count + 1;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "Failed to prepare SQL statement: %s", sqlite3_errmsg(db));
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    char *uri = (char *)malloc(r->uri.len + 1);  // +1 用于 Null 结尾
    // 使用 memcpy 复制 uri 数据并添加 Null 结尾
    memcpy(uri, r->uri.data, r->uri.len);
    uri[r->uri.len] = '\0';  // 手动添加 Null 结尾

//    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
//                  "URIURIURI: %s",  uri);

    if (uriMap.find(uri) != uriMap.end()) {
        // 绑定 IP 地址
        sqlite3_bind_text(stmt, 1, uri, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, (const char *) r->connection->addr_text.data, -1, SQLITE_STATIC);

        // 执行插入
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "Failed to execute SQL statement: %s", sqlite3_errmsg(db));
        }
    }

    // 清理
    sqlite3_finalize(stmt);
    free(uri);

    // 返回静态页面
    ngx_int_t rc = ngx_http_discard_request_body(r);
    if (rc != NGX_OK) {
        return rc;
    }

    return NGX_DECLINED;
}



