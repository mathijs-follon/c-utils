#include "iterator.h"
#include "alloc.h"

#include <stdlib.h>
#include <string.h>

static const void *elem_const(const void *data, usize stride, usize index) {
    return (const u8 *)data + index * stride;
}

static Iterator iter_make(const void *data, usize stride, usize len, isize step) {
    if (!stride || (!data && len != 0))
        return ITER_NULL;

    Iterator it = {
        .data = data,
        .stride = stride,
        .len = len,
        .step = step >= 0 ? 1 : -1,
        .index = 0,
    };

    if (len == 0)
        return it;

    if (it.step < 0)
        it.index = len - 1;

    return it;
}

Iterator iter_from_ptr(const void *data, usize stride, usize len) {
    if (stride == 0 || (len && !data))
        return ITER_NULL;
    return iter_make(data, stride, len, 1);
}

Iterator iter_from_vec(const Vec *vec) {
    if (!vec || !vec_valid(vec))
        return ITER_NULL;
    return iter_make(vec->data, vec->stride, vec->length, 1);
}

Iterator iter_from_stack(const Stack *stack) {
    if (!stack || !stack_valid(stack))
        return ITER_NULL;
    return iter_from_vec(&stack->data);
}

Iterator iter_from_queue(const Queue *queue) {
    if (!queue || !queue_valid(queue))
        return ITER_NULL;

    usize len = queue_length(queue);
    if (len == 0)
        return iter_make(NULL, queue->data.stride ? queue->data.stride : 1, 0, 1);

    const void *base = (const u8 *)queue->data.data + queue->head * queue->data.stride;
    return iter_make(base, queue->data.stride, len, 1);
}

Iterator iter_range(const void *data, usize stride, usize start, usize end) {
    if (stride == 0 || end < start || (end > start && !data))
        return ITER_NULL;
    const void *base = end > start ? elem_const(data, stride, start) : data;
    return iter_make(base, stride, end - start, 1);
}

Iterator iter_range_vec(const Vec *vec, usize start, usize end) {
    if (!vec || !vec_valid(vec) || end < start || end > vec->length)
        return ITER_NULL;
    return iter_range(vec->data, vec->stride, start, end);
}

bool iter_valid(const Iterator *it) { return it && it->stride != 0 && it->step != 0; }

bool iter_done(const Iterator *it) {
    if (!iter_valid(it) || it->len == 0)
        return true;
    return it->index >= it->len;
}

usize iter_index(const Iterator *it) {
    if (!iter_valid(it) || iter_done(it))
        return 0;
    return it->index;
}

usize iter_remaining(const Iterator *it) {
    if (!iter_valid(it) || it->len == 0 || iter_done(it))
        return 0;

    if (it->step > 0)
        return it->len - it->index;
    return it->index + 1;
}

void *iter_get(Iterator *it) {
    if (!it || iter_done(it) || !it->data)
        return NULL;
    return (void *)elem_const(it->data, it->stride, it->index);
}

const void *iter_get_const(const Iterator *it) {
    if (!it || iter_done(it) || !it->data)
        return NULL;
    return elem_const(it->data, it->stride, it->index);
}

bool iter_next(Iterator *it) {
    if (!iter_valid(it) || iter_done(it))
        return false;

    if (it->step > 0) {
        if (it->index >= it->len - 1) {
            it->index = it->len;
            return false;
        }
        it->index++;
        return true;
    }

    if (it->index == 0) {
        it->index = it->len;
        return false;
    }
    it->index--;
    return true;
}

bool iter_prev(Iterator *it) {
    if (!iter_valid(it) || it->len == 0)
        return false;

    if (it->step > 0) {
        if (iter_done(it)) {
            it->index = it->len - 1;
            return true;
        }
        if (it->index == 0)
            return false;
        it->index--;
        return true;
    }

    if (iter_done(it)) {
        it->index = 0;
        return true;
    }
    if (it->index >= it->len - 1)
        return false;
    it->index++;
    return true;
}

bool iter_advance(Iterator *it, isize n) {
    if (!iter_valid(it))
        return false;
    if (n == 0)
        return !iter_done(it);

    if (n > 0) {
        for (isize i = 0; i < n; i++) {
            if (iter_done(it))
                return false;
            if (!iter_next(it) && i + 1 < n)
                return false;
        }
        return !iter_done(it);
    }

    for (isize i = 0; i < -n; i++) {
        if (!iter_prev(it))
            return false;
    }
    return true;
}

bool iter_seek(Iterator *it, usize index) {
    if (!iter_valid(it) || index >= it->len)
        return false;
    it->index = index;
    return true;
}

void iter_reverse(Iterator *it) {
    if (!iter_valid(it))
        return;
    it->step = -it->step;
    iter_reset(it);
}

void iter_reset(Iterator *it) {
    if (!iter_valid(it))
        return;
    it->index = (it->step > 0 || it->len == 0) ? 0 : it->len - 1;
}

/* --- Consumer ----------------------------------------------------------- */

static usize consumer_len(const Consumer *c) {
    if (!c || !c->owner)
        return 0;

    switch (c->kind) {
    case CONSUMER_KIND_VEC:
        return vec_length((const Vec *)c->owner);
    case CONSUMER_KIND_STACK:
        return stack_length((const Stack *)c->owner);
    case CONSUMER_KIND_QUEUE:
        return queue_length((const Queue *)c->owner);
    default:
        return 0;
    }
}

Consumer consumer_from_vec(Vec *vec) {
    if (!vec || !vec_valid(vec))
        return CONSUMER_NULL;
    return (Consumer){vec, vec->stride, CONSUMER_KIND_VEC, 1};
}

Consumer consumer_from_stack(Stack *stack) {
    if (!stack || !stack_valid(stack))
        return CONSUMER_NULL;
    return (Consumer){stack, stack_stride(stack), CONSUMER_KIND_STACK, 1};
}

Consumer consumer_from_queue(Queue *queue) {
    if (!queue || !queue_valid(queue))
        return CONSUMER_NULL;
    return (Consumer){queue, queue_stride(queue), CONSUMER_KIND_QUEUE, 1};
}

bool consumer_valid(const Consumer *c) {
    return c && c->owner && c->stride && c->kind != CONSUMER_KIND_NONE;
}

bool consumer_done(const Consumer *c) { return !consumer_valid(c) || consumer_len(c) == 0; }

usize consumer_remaining(const Consumer *c) { return consumer_len(c); }

void *consumer_peek_ptr(Consumer *c) {
    if (consumer_done(c))
        return NULL;

    switch (c->kind) {
    case CONSUMER_KIND_VEC: {
        Vec *v = (Vec *)c->owner;
        return c->step > 0 ? vec_first(v) : vec_last(v);
    }
    case CONSUMER_KIND_STACK:
        return stack_top((Stack *)c->owner);
    case CONSUMER_KIND_QUEUE:
        return queue_front((Queue *)c->owner);
    default:
        return NULL;
    }
}

bool consumer_peek(const Consumer *c, void *out) {
    if (!c)
        return false;
    void *src = consumer_peek_ptr((Consumer *)c);
    if (!src)
        return false;
    if (out)
        memcpy(out, src, c->stride);
    return true;
}

bool consumer_next(Consumer *c, void *out) {
    if (consumer_done(c))
        return false;

    switch (c->kind) {
    case CONSUMER_KIND_VEC: {
        Vec *v = (Vec *)c->owner;
        if (c->step > 0)
            return vec_remove_at(v, 0, out);
        return vec_pop(v, out);
    }
    case CONSUMER_KIND_STACK:
        return stack_pop((Stack *)c->owner, out);
    case CONSUMER_KIND_QUEUE:
        return queue_pop((Queue *)c->owner, out);
    default:
        return false;
    }
}

void consumer_reverse(Consumer *c) {
    if (!consumer_valid(c) || c->kind != CONSUMER_KIND_VEC)
        return;
    c->step = -c->step;
}

/* --- Producer ----------------------------------------------------------- */

Producer producer_to_vec(Vec *vec, ProducerSource source, void *ctx) {
    if (!vec || !vec_valid(vec) || !source)
        return PRODUCER_NULL;
    return (Producer){vec, vec->stride, PRODUCER_KIND_VEC, source, ctx, 0};
}

Producer producer_to_stack(Stack *stack, ProducerSource source, void *ctx) {
    if (!stack || !stack_valid(stack) || !source)
        return PRODUCER_NULL;
    return (Producer){stack, stack_stride(stack), PRODUCER_KIND_STACK, source, ctx, 0};
}

Producer producer_to_queue(Queue *queue, ProducerSource source, void *ctx) {
    if (!queue || !queue_valid(queue) || !source)
        return PRODUCER_NULL;
    return (Producer){queue, queue_stride(queue), PRODUCER_KIND_QUEUE, source, ctx, 0};
}

bool producer_valid(const Producer *p) {
    return p && p->owner && p->stride && p->source && p->kind != PRODUCER_KIND_NONE;
}

static bool producer_push(Producer *p, const void *element) {
    switch (p->kind) {
    case PRODUCER_KIND_VEC:
        return vec_push((Vec *)p->owner, element);
    case PRODUCER_KIND_STACK:
        return stack_push((Stack *)p->owner, element);
    case PRODUCER_KIND_QUEUE:
        return queue_push((Queue *)p->owner, element);
    default:
        return false;
    }
}

bool producer_next(Producer *p) {
    if (!producer_valid(p))
        return false;

    void *buf = utils_malloc(p->stride);
    if (!buf)
        return false;

    bool ok = p->source(p->ctx, buf) && producer_push(p, buf);
    if (ok)
        p->produced++;

    utils_free(buf);
    return ok;
}

usize producer_run(Producer *p) {
    if (!producer_valid(p))
        return 0;

    usize start = p->produced;
    while (producer_next(p)) {
    }
    return p->produced - start;
}

usize producer_run_until(Producer *p, ProducerUntil until, void *until_ctx) {
    if (!producer_valid(p))
        return 0;

    usize start = p->produced;
    void *buf = utils_malloc(p->stride);
    if (!buf)
        return 0;

    for (;;) {
        if (until && until(until_ctx, p->produced, NULL))
            break;

        if (!p->source(p->ctx, buf))
            break;

        if (!producer_push(p, buf))
            break;

        p->produced++;

        if (until && until(until_ctx, p->produced, buf))
            break;
    }

    utils_free(buf);
    return p->produced - start;
}
