extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <sqlite3.h>
#include <ngx_thread_pool.h>
}

typedef struct {
    ngx_str_t client_ip;
    ngx_http_request_t *request;
} click_tracker_ctx_t;

static ngx_int_t ngx_http_clicktracker_handler(ngx_http_request_t *r);

static void ngx_http_clicktracker_thread_handler(void *data, ngx_log_t *log);

static void ngx_http_clicktracker_thread_completion(ngx_event_t *ev);

static char *ngx_http_clicktracker(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_command_t ngx_http_clicktracker_commands[] = {
        {
                ngx_string("clicktracker"),  // 配置指令
                NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_NOARGS,  // 指令的作用范围
                ngx_http_clicktracker,  // 配置处理函数
                NGX_HTTP_LOC_CONF_OFFSET,
                0,
                NULL
        },
        ngx_null_command
};

static ngx_http_module_t ngx_http_clicktracker_module_ctx = {
        NULL,  // preconfiguration
        NULL,  // postconfiguration
        NULL,  // create main configuration
        NULL,  // init main configuration
        NULL,  // create server configuration
        NULL,  // merge server configuration
        NULL,  // create location configuration
        NULL,  // merge location configuration
};

ngx_module_t ngx_http_clicktracker_module = {
        NGX_MODULE_V1,
        &ngx_http_clicktracker_module_ctx,  // module context
        ngx_http_clicktracker_commands,  // module directives
        NGX_HTTP_MODULE,  // module type
        NULL,  // init master
        NULL,  // init module
        NULL,  // init process
        NULL,  // init thread
        NULL,  // exit thread
        NULL,  // exit process
        NULL,  // exit master
        NGX_MODULE_V1_PADDING
};

// 异步任务处理函数
static void ngx_http_clicktracker_thread_handler(void *data, ngx_log_t *log) {
    click_tracker_ctx_t *ctx = (click_tracker_ctx_t *) data;
    ngx_str_t client_ip = ctx->client_ip;

    const char *db_path = "/path/to/clicktracker.db";
    sqlite3 *db;
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO clicks (ip, count) VALUES (?, 1) ON CONFLICT(ip) DO UPDATE SET count=count+1";

    // 打开 SQLite3 数据库
    if (sqlite3_open(db_path, &db) != SQLITE_OK) {
        ngx_log_error(NGX_LOG_ERR, log, 0, "Failed to open SQLite database.");
        return;
    }

    // 准备 SQL 语句
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        sqlite3_close(db);
        ngx_log_error(NGX_LOG_ERR, log, 0, "Failed to prepare SQL statement.");
        return;
    }

    // 绑定 IP 地址到 SQL 语句
    sqlite3_bind_text(stmt, 1, (const char *) client_ip.data, client_ip.len, SQLITE_TRANSIENT);

    // 执行 SQL 语句
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        ngx_log_error(NGX_LOG_ERR, log, 0, "Failed to execute SQL statement.");
        return;
    }

    // 清理资源
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    ngx_log_debug(NGX_LOG_DEBUG_HTTP, log, 0, "Click recorded for IP: %V", &client_ip);

    return;
}

// 异步任务完成后的回调
static void ngx_http_clicktracker_thread_completion(ngx_event_t *ev) {
    click_tracker_ctx_t *ctx = (click_tracker_ctx_t *) ev->data;
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, ev->log, 0, "Asynchronous click recording completed for IP: %V", &ctx->client_ip);
}

// 处理请求的 handler 函数
static ngx_int_t ngx_http_clicktracker_handler(ngx_http_request_t *r) {
    if (!(r->method & NGX_HTTP_GET)) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    ngx_str_t client_ip = r->connection->addr_text;

    // 创建上下文数据
    click_tracker_ctx_t *ctx = (click_tracker_ctx_t *) ngx_pcalloc(r->pool, sizeof(click_tracker_ctx_t));
    if (ctx == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    ctx->client_ip = client_ip;
    ctx->request = r;

    // 创建线程任务
    ngx_thread_task_t *task = ngx_thread_task_alloc(r->pool, sizeof(click_tracker_ctx_t));
    if (task == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    task->ctx = ctx;
    task->handler = ngx_http_clicktracker_thread_handler;  // 异步任务的处理函数
    task->event.handler = ngx_http_clicktracker_thread_completion;  // 异步任务完成后的回调
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

    ngx_str_t response = ngx_string("<html><body><h1>Static Page</h1></body></html>");
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = response.len;

    ngx_buf_t *b;
    b = (ngx_buf_t *) ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ngx_chain_t out;
    out.buf = b;
    out.next = NULL;

    b->pos = response.data;
    b->last = response.data + response.len;
    b->memory = 1;
    b->last_buf = 1;

    ngx_str_t content_type = ngx_string("text/html");
    r->headers_out.content_type = content_type;

    rc = ngx_http_send_header(r);
    if (rc != NGX_OK) {
        return rc;
    }

    return ngx_http_output_filter(r, &out);
}

// 配置处理函数
static char *ngx_http_clicktracker(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_core_loc_conf_t *clcf;

    clcf = (ngx_http_core_loc_conf_t *) ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_clicktracker_handler;  // 将请求处理器绑定到配置指令

    return NGX_CONF_OK;
}
