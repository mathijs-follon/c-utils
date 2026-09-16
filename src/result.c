#include "result.h"

const char *utils_status_str(UtilsStatus s) {
    switch (s) {
    case UTILS_OK:
        return "ok";
    case UTILS_ERR:
        return "error";
    case UTILS_ERR_NOMEM:
        return "out of memory";
    case UTILS_ERR_INVAL:
        return "invalid argument";
    case UTILS_ERR_RANGE:
        return "out of range";
    case UTILS_ERR_NOT_FOUND:
        return "not found";
    }
    return "unknown";
}
