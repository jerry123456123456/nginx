#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

//定义一个结构体 ngx_http_prefix_filter_conf_t，用于存储模块的配置信息。这里包含一个 ngx_flag_t 类型的 enable 标志，用于启用或禁用模块的功能。
typedef struct {
	ngx_flag_t enable;
} ngx_http_prefix_filter_conf_t;

//定义一个上下文结构体 ngx_http_prefix_filter_ctx_t，用于存储与请求相关的状态信息。这里的 add_prefix 标志用于指示是否需要在响应体前添加前缀。
typedef struct {
	ngx_int_t add_prefix;
} ngx_http_prefix_filter_ctx_t;

static ngx_int_t ngx_http_prefix_filter_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_prefix_filter_header_filter(ngx_http_request_t *r);
static ngx_int_t ngx_http_prefix_filter_body_filter(ngx_http_request_t *r, ngx_chain_t *in);

//filter_prefix 是一个静态字符串，定义了要添加到响应体前的 HTML 前缀。
static ngx_str_t filter_prefix = ngx_string("<h2>Author : King</h2><p><a href=\"http://www.0voice.com\">0voice</a></p>");

/*
ngx_http_prefix_filter_create_conf 用于创建模块的配置块。
使用 ngx_pcalloc 分配内存并初始化为零。
enable 被设置为 NGX_CONF_UNSET，表示默认未设置
*/

static void *ngx_http_prefix_filter_create_conf(ngx_conf_t *cf) {

	ngx_http_prefix_filter_conf_t *conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_prefix_filter_conf_t));
	if (conf == NULL) {
		return NULL;
	}

	conf->enable = NGX_CONF_UNSET;

	return conf;
}

/*
ngx_http_prefix_filter_merge_conf 用于合并父配置和子配置。这里合并 enable 标志的值，
优先使用子配置的值，如果子配置未设置，则使用父配置的值。默认值为 0。
*/

static char *ngx_http_prefix_filter_merge_conf(ngx_conf_t *cf, void *parent, void *child) {
	ngx_http_prefix_filter_conf_t *prev = (ngx_http_prefix_filter_conf_t*)parent;
	ngx_http_prefix_filter_conf_t *conf = (ngx_http_prefix_filter_conf_t*)child;

	ngx_conf_merge_value(conf->enable, prev->enable, 0);

	return NGX_CONF_OK;
}

/*
ngx_http_prefix_filter_commands 定义了一个名为 add_prefix 的指令。
该指令可以放在 http、server、location 和 limit 块中。
ngx_conf_set_flag_slot 是处理这个指令的回调函数，用于设置标志位 enable。
*/

static ngx_command_t ngx_http_prefix_filter_commands[] = {
	{
		ngx_string("add_prefix"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_prefix_filter_conf_t, enable),
		NULL
	},
	ngx_null_command
};


/*
ngx_http_prefix_filter_module_ctx 定义了模块的上下文，
包括创建配置块和合并配置的函数。
ngx_http_prefix_filter_init 用于初始化模块。
*/
static ngx_http_module_t ngx_http_prefix_filter_module_ctx = {
	NULL,
	ngx_http_prefix_filter_init,
	NULL,
	NULL,
	NULL,
	NULL,
	ngx_http_prefix_filter_create_conf,
	ngx_http_prefix_filter_merge_conf
};

//ngx_http_prefix_filter_module 是模块的主要定义，包括模块的版本、上下文、命令和类型等。
ngx_module_t ngx_http_prefix_filter_module = {
	NGX_MODULE_V1,
	&ngx_http_prefix_filter_module_ctx,
	ngx_http_prefix_filter_commands,
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

static ngx_http_output_header_filter_pt ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt ngx_http_next_body_filter;

/*
ngx_http_prefix_filter_init 是模块初始化函数。它将自定义的头部和主体过滤器插入到 Nginx 的过滤器链中。
ngx_http_prefix_filter_header_filter 和 ngx_http_prefix_filter_body_filter 将被设置为新的过滤器函数。
*/
//总的来说，这个函数就是截胡
static ngx_int_t ngx_http_prefix_filter_init(ngx_conf_t *cf) {

	ngx_http_next_header_filter = ngx_http_top_header_filter;
	ngx_http_top_header_filter = ngx_http_prefix_filter_header_filter;

	ngx_http_next_body_filter = ngx_http_top_body_filter;
	ngx_http_top_body_filter = ngx_http_prefix_filter_body_filter;

	return NGX_OK;
}

/*
ngx_http_prefix_filter_header_filter 函数用于处理 HTTP 响应头。
如果响应状态码不是 200 OK，直接传递给下一个头部过滤器。检查模块上下文 ctx，
如果存在则直接传递。如果配置 enable 为 0，则直接传递。
否则，根据 Content-Type 判断是否是 text/html，
如果是，将前缀添加到响应头的 content_length 中，
并将 add_prefix 设置为 1。
*/

static ngx_int_t ngx_http_prefix_filter_header_filter(ngx_http_request_t *r) {

	ngx_http_prefix_filter_ctx_t *ctx;
	ngx_http_prefix_filter_conf_t *conf;

	if (r->headers_out.status != NGX_HTTP_OK) {
		return ngx_http_next_header_filter(r);
	}

	ctx = ngx_http_get_module_ctx(r, ngx_http_prefix_filter_module);
	if (ctx) {
		return ngx_http_next_header_filter(r);
	}

	conf = ngx_http_get_module_loc_conf(r, ngx_http_prefix_filter_module);
	if (conf == NULL) {
		return ngx_http_next_header_filter(r);
	}
	if (conf->enable == 0) {
		return ngx_http_next_header_filter(r);
	}


	ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_prefix_filter_ctx_t));
	if (ctx == NULL) {
		return NGX_ERROR;
	}
	ctx->add_prefix = 0;

	ngx_http_set_ctx(r, ctx, ngx_http_prefix_filter_module);

	if (r->headers_out.content_type.len >= sizeof("text/html") - 1
		&& ngx_strncasecmp(r->headers_out.content_type.data, (u_char*)"text/html", sizeof("text/html")-1) == 0) {

		ctx->add_prefix = 1;
		if (r->headers_out.content_length_n > 0) {
			r->headers_out.content_length_n += filter_prefix.len;
		}

		
	}

	return ngx_http_prefix_filter_header_filter(r);
}

/*
ngx_http_prefix_filter_body_filter 函数用于处理 HTTP 响应体。
首先检查上下文 ctx 和 add_prefix 标志。
如果没有上下文或标志不是 1，直接传递给下一个主体过滤器。
如果 add_prefix 为 1，则设置 add_prefix 为 2，创建一个临时缓冲区 b，将前缀写入缓冲区，并将缓冲区添加到过滤器链 cl 中。
最后，将 cl 链传递给下一个主体过滤器。
*/

static ngx_int_t ngx_http_prefix_filter_body_filter(ngx_http_request_t *r, ngx_chain_t *in) {
	
	ngx_http_prefix_filter_ctx_t *ctx = ngx_http_get_module_ctx(r, ngx_http_prefix_filter_module);
	if (ctx == NULL || ctx->add_prefix != 1) {
		return ngx_http_next_body_filter(r, in);
	}
	
	ctx->add_prefix = 2;

	ngx_buf_t *b = ngx_create_temp_buf(r->pool, filter_prefix.len);
	b->start = b->pos = filter_prefix.data;
	b->last = b->pos + filter_prefix.len;

	ngx_chain_t *cl = ngx_alloc_chain_link(r->pool);
	cl->buf = b;
	cl->next = in;

	return ngx_http_next_body_filter(r, cl);
}








