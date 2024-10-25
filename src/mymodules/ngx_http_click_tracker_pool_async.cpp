extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <sqlite3.h>
#include <ngx_thread_pool.h>
}
/*
 线程池写的有 bug，还要学一下怎么用
 */
typedef struct {
    ngx_str_t client_ip;
    ngx_str_t uri_copy;
    ngx_http_request_t *request;
} click_tracker_ctx_t;

// SQLite 数据库文件路径
#define DATABASE_PATH "/Users/yuanzhaoyi/Develop/nginx/cmake-build-debug/nginx-file/data/click_tracker.db"
// 保存数据库的连接句柄
sqlite3 *db;

static ngx_int_t ngx_http_click_tracker_init(ngx_conf_t *cf);

static void ngx_http_click_tracker_exit(ngx_cycle_t *cycle);

static char *ngx_http_click_tracker(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_int_t ngx_http_click_tracker_request_handler(ngx_http_request_t *r);

static void ngx_http_click_tracker_thread_handler(void *data, ngx_log_t *log);

static void ngx_http_click_tracker_event_handler(ngx_event_t *ev);

static void ngx_http_click_tracker_thread_completion(ngx_event_t *ev);


static ngx_command_t ngx_http_click_tracker_commands[] = {
        {
                ngx_string("click_tracker"),  // 配置指令
                NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF |
                NGX_CONF_NOARGS,  // 指令的作用范围
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

    auto cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    // 初始化数据库连接
    if (sqlite3_open(DATABASE_PATH, &db) != SQLITE_OK) {
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
    if (db != NULL) {
        sqlite3_close(db);
    }
}

// 配置处理函数
static char *ngx_http_click_tracker(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
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

    ngx_str_t client_ip = r->connection->addr_text;

    // 创建上下文数据
    auto *ctx = (click_tracker_ctx_t *) ngx_pcalloc(r->pool, sizeof(click_tracker_ctx_t));
    if (ctx == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    ctx->client_ip = client_ip;
    ctx->uri_copy = r->uri;
    ctx->request = r;

    // 创建线程任务
    ngx_thread_task_t *task = ngx_thread_task_alloc(r->pool, sizeof(click_tracker_ctx_t));
    if (task == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    task->ctx = ctx;
    task->handler = ngx_http_click_tracker_thread_handler;  // 异步任务的处理函数
    task->event.handler = ngx_http_click_tracker_thread_completion;  // 异步任务完成后的回调
    task->event.data = ctx;
    task->event.log = r->connection->log;

    ngx_str_t pool_name = ngx_string("click_tracker_pool");
    ngx_thread_pool_t *tp = ngx_thread_pool_get((ngx_cycle_t *) ngx_cycle, &pool_name);

    if (tp == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    // 向线程池提交任务
    if (ngx_thread_task_post(tp, task) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    // 返回静态页面
    ngx_int_t rc = ngx_http_discard_request_body(r);
    if (rc != NGX_OK) {
        return rc;
    }

    return NGX_DECLINED;
}

// 异步任务处理函数
static void ngx_http_click_tracker_thread_handler(void *data, ngx_log_t *log) {
    auto *ctx = (click_tracker_ctx_t *) data;

    // 创建事件
    auto *ev = (ngx_event_t *) ngx_pcalloc(ctx->request->pool, sizeof(ngx_event_t));
    if (ev == NULL) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "Failed to allocate event");
        return;
    }

    ev->handler = ngx_http_click_tracker_event_handler;
    ev->data = ctx;

    // 调度事件，异步执行
    ngx_post_event(ev, &ngx_posted_events);
    }

// 执行 SQLite 插入操作的回调函数
static void ngx_http_click_tracker_event_handler(ngx_event_t *ev) {
    auto *ctx = (click_tracker_ctx_t *) ev->data;
    char *err_msg = NULL;

    // 准备 SQL 插入语句
    const char *sql = "INSERT INTO page_clicks (uri, ip, count) VALUES (?,?, 1) "
                      "ON CONFLICT(uri, ip) DO UPDATE SET count = count + 1;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        ngx_log_error(NGX_LOG_ERR, ctx->request->connection->log, 0,
                      "Failed to prepare SQL statement: %s", sqlite3_errmsg(db));
        return;
    }

    // 绑定 IP 地址
    sqlite3_bind_text(stmt, 1, (const char *) ctx->uri_copy.data, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, (const char *) ctx->client_ip.data, -1, SQLITE_STATIC);


    // 执行插入
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        ngx_log_error(NGX_LOG_ERR, ctx->request->connection->log, 0,
                      "Failed to execute SQL statement: %s", sqlite3_errmsg(db));
    }

    // 清理
    sqlite3_finalize(stmt);
}

// 异步任务完成后的回调
static void ngx_http_click_tracker_thread_completion(ngx_event_t *ev) {
    auto *ctx = (click_tracker_ctx_t *) ev->data;
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, ev->log, 0, "Asynchronous click recording completed for IP: %V", &ctx->client_ip);
}




