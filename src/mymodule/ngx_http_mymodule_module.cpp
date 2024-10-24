extern "C"{
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_string.h>
#include <ngx_http_request.h>
#include <ngx_hash.h>
#include <ngx_http_config.h>
static ngx_int_t ngx_http_mymodulecpp_handler(ngx_http_request_t *r);
static char *
ngx_http_mymodulecpp(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
}
//只是包含了c++的库
#include <iostream>
#include <string>
using namespace std;
#include "ngx_http_mymodulecpp_module.h"

static ngx_command_t ngx_http_mymodulecpp_commands[] = {
        {
                ngx_string("mymodulecpp"),
                NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_NOARGS,
                //set回调函数，
                //char               *(*set)(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
                //当某个配置快中出现mymodulecpp时，就会回调此函数
                ngx_http_mymodulecpp,
                NGX_HTTP_LOC_CONF_OFFSET,
                0,
                NULL
        },
        //空的ngx_command_t用于表示数组结束
        //#define ngx_null_command  { ngx_null_string, 0, NULL, 0, 0, NULL }
        ngx_null_command

};
static ngx_http_module_t ngx_http_mymodulecpp_module_ctx = {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
};

ngx_module_t ngx_http_mymodulecpp_module = {
        NGX_MODULE_V1,
        //ctx,对于HTTP模块来说，ctx必须是ngx_http_module_t接口
        &ngx_http_mymodulecpp_module_ctx,
        //commands,
        ngx_http_mymodulecpp_commands,
        //定义http模块时，必须设置成NGX_HTTP_MODULE
        NGX_HTTP_MODULE,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NGX_MODULE_V1_PADDING
};

//配置项对应的回调函数
static char *
ngx_http_mymodulecpp(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf;

    clcf = (ngx_http_core_loc_conf_t *)ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

    //在NGX_HTTP_CONTENT_PHASE阶段会调用此回调函数
    clcf->handler = ngx_http_mymodulecpp_handler;

    return NGX_CONF_OK;
}
//实际完成处理的回调函数
/*
 * r 是nginx已经处理完了的http请求头
 */
static ngx_int_t ngx_http_mymodulecpp_handler(ngx_http_request_t *r)
{

    if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD))) {
//非法请求方式 状态码 405
        return NGX_HTTP_NOT_ALLOWED;
    }
//丢弃客户端发送来的HTTP包体内容
    ngx_int_t rc = ngx_http_discard_request_body(r);
    if (rc != NGX_OK) {
        return rc;
    }
    //测试c++语法是否支持
    string* a = new string("cpp");
    ngx_str_t type = ngx_string("text/plain");
    ngx_str_t response = ngx_string(a->c_str());
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = a->length();
    r->headers_out.content_type = type;
//自定义响应头
    ngx_table_elt_t* p = (ngx_table_elt_t*)ngx_list_push(&r->headers_out.headers);
    p->hash = 1;

    p->key.len = sizeof("codelover")-1;
    p->key.data = (u_char*)"codelover";
    p->value.len = sizeof("codelover")-1;
    p->value.data = (u_char*)"codelover";

//发送响应头
    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    ngx_buf_t *b;
//r->pool内存池
    b = ngx_create_temp_buf(r->pool, response.len);
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ngx_memcpy(b->pos, response.data, response.len);
//必须设置好last指针，如果last和pos相等，是不会发送的
    b->last = b->pos + response.len;
//声明这是最后一块缓冲区
    b->last_buf = 1;

    ngx_chain_t out;
    out.buf = b;
    out.next = NULL;

    return ngx_http_output_filter(r, &out);
}