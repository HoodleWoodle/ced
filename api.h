#include "common.h"

namespace ced {
    typedef void *error_t; // TODO: IMPL

    typedef void *seg_t;   // TODO: IMPL
    typedef void *point_t; // TODO: IMPL
    typedef void *iter_t;  // TODO: IMPL

    typedef struct {
        char *buf;
        u64 len;
    } slice_t;

    error_t open(const char *filepath, seg_t *retv);
    error_t close(seg_t);
    error_t save(seg_t);

    error_t get_point(seg_t, u64 offset, point_t *retv);
    error_t get_point(iter_t, u64 offset, point_t *retv);
    error_t subseg(seg_t, point_t begin, point_t end, seg_t *retv);

    error_t iter(seg_t, iter_t *retv);
    slice_t next(iter_t *);

    error_t write(seg_t, slice_t);
}
