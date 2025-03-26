#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/tls1.h>

// 定义 TLS 指纹配置结构
typedef struct {
    ngx_str_t tls_fingerprint;
    ngx_flag_t enable_chrome_fingerprint;
    ngx_array_t *custom_ciphers;
} ngx_http_tls_proxy_conf_t;

// Chrome 的 TLS 指纹配置 (参考 chromium/src/net/socket/ssl_client_socket_impl.cc)
static const uint16_t kDefaultSignatureAlgorithms[] = {
        // BoringSSL 中的签名算法常量
        SSL_SIGN_ECDSA_SECP256R1_SHA256,
        SSL_SIGN_RSA_PSS_RSAE_SHA256,
        SSL_SIGN_RSA_PKCS1_SHA256,
        SSL_SIGN_ECDSA_SECP384R1_SHA384,
        SSL_SIGN_RSA_PSS_RSAE_SHA384,
        SSL_SIGN_RSA_PKCS1_SHA384,
        SSL_SIGN_RSA_PSS_RSAE_SHA512,
        SSL_SIGN_RSA_PKCS1_SHA512,
};

static const uint16_t kDefaultCiphers[] = {
        // Chrome 默认的密码套件
        TLS1_CK_AES_128_GCM_SHA256,
        TLS1_CK_AES_256_GCM_SHA384,
        TLS1_CK_CHACHA20_POLY1305_SHA256,
        TLS1_CK_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
        TLS1_CK_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
        // 添加更多 Chrome 使用的密码套件...
};

// 模块命令定义
static ngx_command_t ngx_http_tls_proxy_commands[] = {
        {ngx_string("tls_proxy_fingerprint"),
         NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
         ngx_conf_set_str_slot,
         NGX_HTTP_LOC_CONF_OFFSET,
         offsetof(ngx_http_tls_proxy_conf_t, tls_fingerprint),
         NULL},

        {ngx_string("tls_proxy_chrome_fingerprint"),
         NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_FLAG,
         ngx_conf_set_flag_slot,
         NGX_HTTP_LOC_CONF_OFFSET,
         offsetof(ngx_http_tls_proxy_conf_t, enable_chrome_fingerprint),
         NULL},

        ngx_null_command
};

// 创建配置
static void *ngx_http_tls_proxy_create_conf(ngx_conf_t *cf) {
    ngx_http_tls_proxy_conf_t *conf;

    conf = (ngx_http_tls_proxy_conf_t *) ngx_pcalloc(cf->pool, sizeof(ngx_http_tls_proxy_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    // 默认值
    conf->enable_chrome_fingerprint = NGX_CONF_UNSET;
    conf->tls_fingerprint.data = NULL;  // Initialize string properly
    conf->tls_fingerprint.len = 0;

    return conf;
}

// 合并配置
static char *ngx_http_tls_proxy_merge_conf(ngx_conf_t *cf, void *parent, void *child) {
    ngx_http_tls_proxy_conf_t *prev = (ngx_http_tls_proxy_conf_t *) parent;
    ngx_http_tls_proxy_conf_t *conf = (ngx_http_tls_proxy_conf_t *) child;

    ngx_conf_merge_str_value(conf->tls_fingerprint, prev->tls_fingerprint, "");
    ngx_conf_merge_value(conf->enable_chrome_fingerprint, prev->enable_chrome_fingerprint, 0);

    return NGX_CONF_OK;
}


// 模块上下文
static ngx_http_module_t ngx_http_tls_proxy_module_ctx = {
        NULL,                                  /* preconfiguration */
        NULL,                                  /* postconfiguration */
        NULL,                                  /* create main configuration */
        NULL,                                  /* init main configuration */
        NULL,                                  /* create server configuration */
        NULL,                                  /* merge server configuration */
        (void *(*)(ngx_conf_t *)) ngx_http_tls_proxy_create_conf,        /* create location configuration */
        (char *(*)(ngx_conf_t *, void *,
                   void *)) ngx_http_tls_proxy_merge_conf          /* merge location configuration */
};

// 模块定义
ngx_module_t ngx_tls_proxy_module = {
        NGX_MODULE_V1,
        &ngx_http_tls_proxy_module_ctx,        /* module context */
        ngx_http_tls_proxy_commands,           /* module directives */
        NGX_HTTP_MODULE,                       /* module type */
        NULL,                                  /* init master */
        NULL,                                  /* init module */
        NULL,                                  /* init process */
        NULL,                                  /* init thread */
        NULL,                                  /* exit thread */
        NULL,                                  /* exit process */
        NULL,                                  /* exit master */
        NGX_MODULE_V1_PADDING
};


// 设置 Chrome 指纹
static ngx_int_t ngx_http_tls_proxy_set_chrome_fingerprint(SSL_CTX *ctx) {
    // 1. 设置密码套件 (BoringSSL 方式)
    if (!SSL_CTX_set_cipher_list(ctx,
                                 "TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384")) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "Failed to set cipher list");
        return NGX_ERROR;
    }

    // 2. 设置 ALPN 协议
    static const unsigned char alpn_protos[] = {
            2, 'h', '2',  // h2
            8, 'h', 't', 't', 'p', '/', '1', '.', '1'  // http/1.1
    };
    if (SSL_CTX_set_alpn_protos(ctx, alpn_protos, sizeof(alpn_protos)) != 0) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "Failed to set ALPN protocols");
        return NGX_ERROR;
    }

    // 3. 设置 Session Ticket
    SSL_CTX_set_options(ctx, SSL_OP_NO_TICKET);  // Chrome 默认禁用 Session Ticket

    // 4. 设置扩展顺序 (需要 OpenSSL 1.1.1+)
    // Chrome 的扩展顺序通常是：
    // server_name(0), extended_master_secret(23), renegotiation_info(65281),
    // supported_groups(10), ec_point_formats(11), session_ticket(35),
    // application_layer_protocol_negotiation(16), status_request(5),
    // signature_algorithms(13), signed_certificate_timestamp(18),
    // key_share(51), supported_versions(43), psk_key_exchange_modes(45)
    // 注意：OpenSSL 默认的扩展顺序可能不同，需要自定义实现

    // 3. 设置曲线
    if (!SSL_CTX_set1_curves_list(ctx, "X25519:P-256:P-384:P-521")) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "Failed to set curves");
        return NGX_ERROR;
    }

    // 4. 其他 Chrome 特定的 TLS 设置
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
    SSL_CTX_clear_options(ctx, SSL_OP_LEGACY_SERVER_CONNECT);

    // 5. 设置扩展顺序等其他指纹特征
    // 注意: 这部分可能需要更底层的 OpenSSL/BoringSSL 修改

    return NGX_OK;
}

// 初始化 upstream SSL 连接
static ngx_int_t ngx_http_tls_proxy_setup_upstream(ngx_http_request_t *r) {
    ngx_http_tls_proxy_conf_t *tpc;
    ngx_http_upstream_t *u;

    if (r->upstream == NULL) {
        return NGX_DECLINED;
    }

    u = r->upstream;

    // 检查 upstream 是否使用 SSL
    if (u->ssl == NULL) {
        ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0,
                      "upstream SSL not enabled");
        return NGX_DECLINED;
    }

    // 获取 SSL_CTX
    SSL_CTX *ssl_ctx = u->ssl->ctx;
    if (ssl_ctx == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "upstream SSL_CTX is NULL");
        return NGX_ERROR;
    }


    tpc = (ngx_http_tls_proxy_conf_t *)ngx_http_get_module_loc_conf(r, ngx_tls_proxy_module);

    // 应用 Chrome 指纹
    if (tpc->enable_chrome_fingerprint) {
        if (ngx_http_tls_proxy_set_chrome_fingerprint(ctx) != NGX_OK) {
            return NGX_ERROR;
        }
    }

    // 应用自定义指纹
    if (tpc->tls_fingerprint.len > 0) {
        // 这里实现自定义指纹逻辑
        // 可能需要解析指纹字符串并应用到 SSL_CTX
    }

    // 强制使用 TLS 1.3 (BoringSSL 方式)
    SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);

    return NGX_OK;
}


// 处理函数
static ngx_int_t ngx_http_tls_proxy_handler(ngx_http_request_t *r) {
    if (r->upstream && r->upstream->ssl) {
        return ngx_http_tls_proxy_setup_upstream(r);
    }

    return NGX_DECLINED;
}

// 注册处理程序
static ngx_int_t ngx_http_tls_proxy_init(ngx_conf_t *cf) {
    ngx_http_handler_pt *h;
    ngx_http_core_main_conf_t *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    // 使用 NGX_HTTP_CONTENT_PHASE 代替 NGX_HTTP_SSL_PHASE
    h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_tls_proxy_handler;

    return NGX_OK;
}
