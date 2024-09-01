
#include <ngx_http.h>
#include <ngx_core.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>

#define ENABLE_RBTREE   1

/*
ngx_pv_table_t 结构体定义了一个包含 count 和 addr 的结构体。
count: 记录来自特定 IP 地址的请求次数。
addr: 存储 IP 地址。
*/

typedef struct{
    int count;
    struct in_addr addr;
}ngx_pv_table_t;

ngx_pv_table_t pv_table[256];

#if ENABLE_RBTREE
    static ngx_rbtree_t ngx_pv_rbtree;
    static ngx_rbtree_node_t ngx_pv_sentinel;

//插入新结点
    void ngx_rbtree_insert_count_value(ngx_rbtree_node_t *temp, ngx_rbtree_node_t *node,ngx_rbtree_node_t *sentinel)
{   
    ngx_rbtree_node_t  **p;

    for ( ;; ) {

        /*
         * Timer values
         * 1) are spread in small range, usually several minutes,
         * 2) and overflow each 49 days, if milliseconds are stored in 32 bits.
         * The comparison takes into account that overflow.
         */

        /*  node->key < temp->key */
#if 0
        p = ((ngx_rbtree_key_int_t) (node->key - temp->key) < 0)
            ? &temp->left : &temp->right;
#else
		if (node->key < temp->key) {
			p = &temp->left;
		} else if (node->key > temp->key) {
			p = &temp->right;
		} else { // node->key == temp->key
			return ;
		}


#endif
        if (*p == sentinel) {
            break;
        }

        temp = *p;
    }

    *p = node;
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    ngx_rbt_red(node);
}

//
ngx_rbtree_node_t *ngx_rbtree_count_search(ngx_rbtree_t *rbtree, ngx_rbtree_key_t key) {

	ngx_rbtree_node_t *temp = rbtree->root;

	ngx_rbtree_node_t  **p;

    for ( ;; ) {


		if (key < temp->key) {
			p = &temp->left;
		} else if (key > temp->key) {
			p = &temp->right;
		} else { // node->key == temp->key
			return temp;
		}

        if (*p == &ngx_pv_sentinel) {
            return NULL;
        }

        temp = *p;
    }
	

}


void ngx_rbtree_count_traversal(ngx_rbtree_t *T, ngx_rbtree_node_t *node, char *html) {	
	
	if (node != &ngx_pv_sentinel) {		
		ngx_rbtree_count_traversal(T, node->left, html);		
		//printf("key:%d, color:%d\n", node->key, node->color);	

		char str[INET_ADDRSTRLEN] = {0};
		char buffer[128] = {0};
		snprintf(buffer, 128, "req from : %s, count : %d <br/>",
				inet_ntop(AF_INET, &node->key, str, sizeof(str)), node->data);

		strcat(html, buffer);
		
		ngx_rbtree_count_traversal(T, node->right, html);	
	}
}



#endif


char  *ngx_http_handler_count_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
ngx_int_t ngx_http_handler_count_handler(ngx_http_request_t *r);
ngx_int_t   ngx_http_handler_count_init(ngx_conf_t *cf);

/*
ngx_http_handler_count_cmds 定义了一个 Nginx 配置指令 count。
ngx_string("count"): 配置指令的名称。
NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS: 指令作用于位置配置，并且不需要额外参数。
ngx_http_handler_count_set: 当指令被解析时调用的处理函数。
NGX_HTTP_LOC_CONF_OFFSET: 配置项的偏移量。
0: 没有特别的标志。
NULL: 额外的配置项。
*/
static ngx_command_t ngx_http_handler_count_cmds[] = {
    {
        ngx_string("count"),
        NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
        ngx_http_handler_count_set,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    ngx_null_command
};

/*

ngx_http_handler_count_ctx 定义了模块的上下文。
这些字段用于定义模块的初始化、配置和处理函数。
ngx_http_handler_count_init 被注释掉了，如果需要模块初始化，请取消注释。*/
// share mem
static ngx_http_module_t ngx_http_handler_count_ctx = {
	NULL,
	//ngx_http_handler_count_init, //
    NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
}; 

/*
ngx_http_handler_count_module 定义了模块的主要结构。
NGX_MODULE_V1: 模块的版本。
&ngx_http_handler_count_ctx: 模块上下文。
ngx_http_handler_count_cmds: 模块的配置指令。
NGX_HTTP_MODULE: 模块类型（HTTP 模块）。
NULL: 一些其他的字段（可选）设置为空。
*/

ngx_module_t ngx_http_handler_count_module = {
    NGX_MODULE_V1,
    &ngx_http_handler_count_ctx,
    ngx_http_handler_count_cmds,
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


//当在conf文件中遇到count这个命令时，会调用这个回调函数 
char  *ngx_http_handler_count_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
#if ENABLE_RBTREE
    ngx_rbtree_init(&ngx_pv_rbtree,&ngx_pv_sentinel,ngx_rbtree_insert_count_value);
#endif    
    ngx_http_core_loc_conf_t *corecf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
	
	corecf->handler = ngx_http_handler_count_handler;

	return NGX_CONF_OK;

}

//组织网页
static int ngx_http_encode_page(char *html) {

	sprintf(html, "<h1>zvoice.jerry</h1>");
	strcat(html, "<h2>");
	
#if ENABLE_RBTREE

	ngx_rbtree_count_traversal(&ngx_pv_rbtree, ngx_pv_rbtree.root, html);

#else
	int i = 0;
	for (i = 0;i < 256;i ++) {
		if (pv_table[i].count != 0) {

			char str[INET_ADDRSTRLEN] = {0};
			char buffer[128] = {0};
			
			snprintf(buffer, 128, "req from : %s, count : %d <br/>",
				inet_ntop(AF_INET, &pv_table[i].addr, str, sizeof(str)), pv_table[i].count);

			strcat(html, buffer);
		}
	}
#endif
	strcat(html, "<h2/>");

	return 0;
}


/*
ngx_http_handler_count_handler 函数处理 HTTP 请求。
u_char html[1024] = {0};: 定义一个用于存储 HTML 内容的缓冲区。
int len = sizeof(html);: 获取缓冲区的大小。
struct sockaddr_in *client_addr = (struct sockaddr_in *)r->connection->sockaddr;: 获取客户端地址信息。
计算 idx 作为 IP 地址的索引，将计数增加 1，并更新 pv_table 中的 IP 地址。
使用 ngx_http_encode_page 函数将数据编码成 HTML。
设置 HTTP 响应状态为 200，并设置内容类型为 text/html。
通过 ngx_http_send_header 发送响应头。
创建一个 ngx_buf_t 缓冲区，用于存储 HTML 内容，并通过 ngx_http_output_filter 发送响应体
*/

//每次发起请求，就会走到这里
ngx_int_t ngx_http_handler_count_handler(ngx_http_request_t *r) {
    //可以获取到对端的信息
    u_char html[1024] = {0};
	int len = sizeof(html);
    struct sockaddr_in *client_addr = (struct sockaddr_in *)r->connection->sockaddr; // ip

#if ENABLE_RBTREE
    //查找，找到就把count+=
    ngx_rbtree_key_t key = (ngx_rbtree_key_t)client_addr->sin_addr.s_addr;
	ngx_rbtree_node_t *node = ngx_rbtree_count_search(&ngx_pv_rbtree, key);
	if (!node) {

		node = ngx_pcalloc(r->pool, sizeof(ngx_rbtree_node_t));
		node->key = key;
		node->data = 1;
		
		ngx_rbtree_insert(&ngx_pv_rbtree, node);
	} else {
		
		node->data ++;

	}


#else
    //拿到ip后，取最后三位，即256个作为索引，存储对应的ip的count值
    int idx = client_addr->sin_addr.s_addr >> 24;
	pv_table[idx].count ++;
	memcpy(&pv_table[idx].addr, &client_addr->sin_addr, sizeof(client_addr->sin_addr));
#endif

    ngx_http_encode_page((char *)html);

    //返回状态默认200
    r->headers_out.status = 200;
    ngx_str_set(&r->headers_out.content_type, "text/html");

    //先把http头发出去
    ngx_http_send_header(r);

    //接着发body部分
    ngx_buf_t *b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    ngx_chain_t out;
    out.buf = b;
    out.next = NULL;
    b->pos = html;
	b->last = html+len;
	b->memory = 1;
	b->last_buf = 1;

	return ngx_http_output_filter(r, &out);
}

//
