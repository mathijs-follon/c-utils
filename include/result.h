#ifndef RESULT_H
#define RESULT_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Generic status codes for fallible library helpers (beyond FileResult).
 *
 *     UtilsStatus s = do_thing();
 *     UTILS_TRY(s);
 */

typedef enum {
    UTILS_OK = 0,
    UTILS_ERR = 1,
    UTILS_ERR_NOMEM,
    UTILS_ERR_INVAL,
    UTILS_ERR_RANGE,
    UTILS_ERR_NOT_FOUND,
} UtilsStatus;

const char *utils_status_str(UtilsStatus s);

#define UTILS_TRY(expr)                                                                            \
    do {                                                                                           \
        UtilsStatus _s = (expr);                                                                   \
        if (_s != UTILS_OK)                                                                        \
            return _s;                                                                             \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* RESULT_H */
