#ifndef ITERATOR_H
#define ITERATOR_H

#include "queue.h"
#include "stack.h"
#include "vec.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Type-erased iteration helpers.
 *
 * Iterator  - walk elements without mutating the collection
 * Consumer  - walk while removing (popping) each visited element
 * Producer  - pull values from a source and push into a collection
 *
 *     Vec v = VEC_NEW(int);
 *     VEC_PUSH(int, &v, 1);
 *     VEC_PUSH(int, &v, 2);
 *
 *     for (Iterator it = iter_from_vec(&v); !iter_done(&it); iter_next(&it)) {
 *         int x = ITER_GET(int, &it);
 *     }
 *
 *     Consumer c = consumer_from_queue(&q);
 *     int x;
 *     while (CONSUMER_NEXT(int, &c, &x)) { ... }
 *
 *     Producer p = producer_to_vec(&v, my_source, ctx);
 *     producer_run_until(&p, my_stop, stop_ctx);
 */

typedef struct {
    const void *data;
    usize stride;
    usize index;
    usize len;
    isize step; /* +1 forward, -1 reverse */
} Iterator;

typedef enum {
    CONSUMER_KIND_NONE = 0,
    CONSUMER_KIND_VEC,
    CONSUMER_KIND_STACK,
    CONSUMER_KIND_QUEUE,
} ConsumerKind;

typedef struct {
    void *owner;
    usize stride;
    ConsumerKind kind;
    isize step; /* Vec only: +1 consume from front, -1 from back */
} Consumer;

typedef enum {
    PRODUCER_KIND_NONE = 0,
    PRODUCER_KIND_VEC,
    PRODUCER_KIND_STACK,
    PRODUCER_KIND_QUEUE,
} ProducerKind;

/* Return false when the source is exhausted. Writes one element into `out`. */
typedef bool (*ProducerSource)(void *ctx, void *out);

/*
 * Return true when production should stop.
 * `last` is the element just pushed (NULL on the pre-check before a push).
 */
typedef bool (*ProducerUntil)(void *ctx, usize produced, const void *last);

typedef struct {
    void *owner;
    usize stride;
    ProducerKind kind;
    ProducerSource source;
    void *ctx;
    usize produced;
} Producer;

#define ITER_NULL ((Iterator){NULL, 0, 0, 0, 1})
#define CONSUMER_NULL ((Consumer){NULL, 0, CONSUMER_KIND_NONE, 1})
#define PRODUCER_NULL ((Producer){NULL, 0, PRODUCER_KIND_NONE, NULL, NULL, 0})

/* --- Iterator ----------------------------------------------------------- */

Iterator iter_from_ptr(const void *data, usize stride, usize len);
Iterator iter_from_vec(const Vec *vec);
Iterator iter_from_stack(const Stack *stack);
Iterator iter_from_queue(const Queue *queue);

Iterator iter_range(const void *data, usize stride, usize start, usize end);
Iterator iter_range_vec(const Vec *vec, usize start, usize end);

bool iter_valid(const Iterator *it);
bool iter_done(const Iterator *it);
usize iter_index(const Iterator *it);
usize iter_remaining(const Iterator *it);

void *iter_get(Iterator *it);
const void *iter_get_const(const Iterator *it);

bool iter_next(Iterator *it);
bool iter_prev(Iterator *it);
bool iter_advance(Iterator *it, isize n);
bool iter_seek(Iterator *it, usize index);

void iter_reverse(Iterator *it);
void iter_reset(Iterator *it);

/* --- Consumer ----------------------------------------------------------- */

Consumer consumer_from_vec(Vec *vec);
Consumer consumer_from_stack(Stack *stack);
Consumer consumer_from_queue(Queue *queue);

bool consumer_valid(const Consumer *c);
bool consumer_done(const Consumer *c);
usize consumer_remaining(const Consumer *c);

/* Peek next element without removing it. */
bool consumer_peek(const Consumer *c, void *out);
void *consumer_peek_ptr(Consumer *c);

/* Remove and return the next element into `out` (out may be NULL). */
bool consumer_next(Consumer *c, void *out);

void consumer_reverse(Consumer *c);

/* --- Producer ----------------------------------------------------------- */

Producer producer_to_vec(Vec *vec, ProducerSource source, void *ctx);
Producer producer_to_stack(Stack *stack, ProducerSource source, void *ctx);
Producer producer_to_queue(Queue *queue, ProducerSource source, void *ctx);

bool producer_valid(const Producer *p);

/* Pull one value from source and push it. False if source done or push fails. */
bool producer_next(Producer *p);

/* Produce until source is exhausted. Returns number produced. */
usize producer_run(Producer *p);

/* Produce until source ends or `until` returns true. Returns number produced. */
usize producer_run_until(Producer *p, ProducerUntil until, void *until_ctx);

/* --- Typed helpers ------------------------------------------------------ */
/*
 *   ITER_GET(T, Iterator *it)                         -> T
 *   ITER_GET_CONST(T, const Iterator *it)             -> const T
 *   ITER_PTR(T, Iterator *it)                         -> T *
 *   ITER_PTR_CONST(T, const Iterator *it)             -> const T *
 *
 *   CONSUMER_PEEK(T, const Consumer *c, T *out)       -> bool
 *   CONSUMER_NEXT(T, Consumer *c, T *out)             -> bool
 *
 *   ITER_FOR(Iterator it, init_expr) { body }
 */

#define ITER_GET(T, it) (*(T *)iter_get(it))
#define ITER_GET_CONST(T, it) (*(const T *)iter_get_const(it))
#define ITER_PTR(T, it) ((T *)iter_get(it))
#define ITER_PTR_CONST(T, it) ((const T *)iter_get_const(it))

#define CONSUMER_PEEK(T, c, out) (consumer_peek((c), (out)))
#define CONSUMER_NEXT(T, c, out) (consumer_next((c), (out)))

#define ITER_FOR(it_var, init_expr)                                                                    \
    for (Iterator it_var = (init_expr); !iter_done(&it_var); iter_next(&it_var))

#ifdef __cplusplus
}
#endif

#endif /* ITERATOR_H */
