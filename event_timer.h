
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_EVENT_TIMER_H_INCLUDED_
#define _NGX_EVENT_TIMER_H_INCLUDED_

#include "rbtree.h"
#include "main.h"
#include <sys/time.h>

#define TIMER_INFINITE  (msec_t) -1

#define TIMER_LAZY_DELAY  300

typedef rbtree_key                  msec_t;
typedef rbtree_key_int              msec_int_t;


int event_timer_init(mtcp_lua_ctx_t *);
msec_t event_find_timer(rbtree_t *);
void event_expire_timers(rbtree_t *);


static inline void
event_del_timer(mtcp_lua_ctx_t *ctx, event_t *ev)
{
    //ngx_mutex_lock(ngx_event_timer_mutex);

    rbtree_delete(&ctx->timer, &ev->timer);

    //ngx_mutex_unlock(ngx_event_timer_mutex);

#if (NGX_DEBUG)
    ev->timer.left = NULL;
    ev->timer.right = NULL;
    ev->timer.parent = NULL;
#endif

    ev->timer_set = 0;
}


static inline void
event_add_timer(mtcp_lua_ctx_t *ctx, event_t *ev, msec_t timer)
{
    msec_t      key;
    msec_int_t  diff;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    msec_t current_msec = (msec_t) tv.tv_sec * 1000 + tv.tv_usec / 1000;
    key = current_msec + timer;

    if (ev->timer_set) {

        /*
         * Use a previous timer value if difference between it and a new
         * value is less than NGX_TIMER_LAZY_DELAY milliseconds: this allows
         * to minimize the rbtree operations for fast connections.
         */

        diff = (msec_int_t) (key - ev->timer.key);

        if (abs(diff) < TIMER_LAZY_DELAY) {
            return;
        }

        event_del_timer(ctx, ev);
    }

    ev->timer.key = key;

    //ngx_mutex_lock(ngx_event_timer_mutex);

    rbtree_insert(&ctx->timer, &ev->timer);

    //ngx_mutex_unlock(ngx_event_timer_mutex);

    ev->timer_set = 1;
}


#endif /* _NGX_EVENT_TIMER_H_INCLUDED_ */
