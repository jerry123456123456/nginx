#include "ngx_config.h"
#include "ngx_conf_file.h"
#include "nginx.h"
#include "ngx_core.h"
#include "ngx_string.h"
#include "ngx_palloc.h"
#include "ngx_array.h"
#include "ngx_hash.h"

#include<stdio.h>
#include<stdlib.h>

//nginx头文件中又包含下面的两个变量，所以我们必须包含

#define unused(x)  x=x

volatile ngx_cycle_t  *ngx_cycle;


void
ngx_log_error_core(ngx_uint_t level, ngx_log_t *log, ngx_err_t err,
    const char *fmt, ...) {

	unused(level);
	unused(log);
	unused(err);
	unused(fmt);

}

void print_pool(ngx_pool_t *pool) {

	printf("\nlast: %p, end: %p\n", pool->d.last, pool->d.end);

}


int main(){
#if 0
    ngx_str_t name =ngx_string("jerry");

    printf("name --> len:%ld, data: %s\n",name.len,name.data);
#else
    //内存池的使用
    ngx_pool_t *pool = ngx_create_pool(4096,NULL);

    print_pool(pool);

    void *p1 = ngx_palloc(pool,10);

    print_pool(pool);
    int *p2 = ngx_palloc(pool,sizeof(int));

    print_pool(pool);

    ngx_destroy_pool(pool);

#endif
    return 0;
}