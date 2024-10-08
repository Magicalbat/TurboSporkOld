#include "costs.h"
#include "err.h"

#include <stdio.h>
#include <math.h>

typedef ts_f32 (_cost_func)(const ts_tensor*, const ts_tensor*);
typedef void (_cost_grad)(ts_tensor*, const ts_tensor*);

typedef struct {
    _cost_func* func;
    _cost_grad* grad;
} _cost;

static ts_f32 null_func(const ts_tensor* in, const ts_tensor* desired_out);
static void null_grad(ts_tensor* in_out, const ts_tensor* desired_out);
static ts_f32 mean_squared_func(const ts_tensor* in, const ts_tensor* desired_out);
static void mean_squared_grad(ts_tensor* in_out, const ts_tensor* desired_out);
static ts_f32 cce_func(const ts_tensor* in, const ts_tensor* desired_out);
static void cce_grad(ts_tensor* in_out, const ts_tensor* desired_out);

static _cost _costs[TS_COST_COUNT] = {
    [TS_COST_NULL] = { null_func, null_grad },
    [TS_COST_MEAN_SQUARED_ERROR] = { mean_squared_func, mean_squared_grad },
    [TS_COST_CATEGORICAL_CROSS_ENTROPY] = { cce_func, cce_grad },
};

ts_f32 ts_cost_func(ts_cost_type type, const ts_tensor* in, const ts_tensor* desired_out) {
    // Checks
    if (type >= TS_COST_COUNT) {
        TS_ERR(TS_ERR_INVALID_INPUT, "Invalid cost function type");
        return 0.0f;
    }
    if (!ts_tensor_shape_eq(in->shape, desired_out->shape)) {
        TS_ERR(TS_ERR_INVALID_INPUT, "Input and desired output must align in cost function");
        return 0.0f;
    }
    if (in == NULL || desired_out == NULL) {
        TS_ERR(TS_ERR_INVALID_INPUT, "Cannot compute cost: in and/or desired_out is NULL");
        return 0.0f;
    }

    // Actual cost function
    return _costs[type].func(in, desired_out);
}
void ts_cost_grad(ts_cost_type type, ts_tensor* in_out, const ts_tensor* desired_out) {
    // Checks
    if (type >= TS_COST_COUNT) {
        TS_ERR(TS_ERR_INVALID_INPUT, "Invalid cost function type");
        return;
    }
    if (!ts_tensor_shape_eq(in_out->shape, desired_out->shape)) {
        TS_ERR(TS_ERR_INVALID_INPUT, "Input and desired output must align in cost function");
        return;
    }
    if (in_out == NULL || desired_out == NULL) {
        TS_ERR(TS_ERR_INVALID_INPUT, "Cannot compute cost: in and/or desired_out is NULL");
        return;
    }

    // Actual gradient
    _costs[type].grad(in_out, desired_out);
}

static ts_f32 null_func(const ts_tensor* in, const ts_tensor* desired_out) {
    TS_UNUSED(in);
    TS_UNUSED(desired_out);

    return 0.0f;
}
static void null_grad(ts_tensor* in_out, const ts_tensor* desired_out) {
    TS_UNUSED(in_out);
    TS_UNUSED(desired_out);
}

#if TS_TENSOR_BACKEND == TS_TENSOR_BACKEND_CPU

static ts_f32 mean_squared_func(const ts_tensor* in, const ts_tensor* desired_out) {
    ts_f32 sum = 0.0f;

    ts_f32* in_data = (ts_f32*)in->data;
    ts_f32* d_out_data = (ts_f32*)desired_out->data;

    ts_u64 size = (ts_u64)in->shape.width * in->shape.height * in->shape.depth;
    for (ts_u64 i = 0; i < size; i++) {
        sum += 0.5f * (in_data[i] - d_out_data[i]) * (in_data[i] - d_out_data[i]);
    }

    return sum / (ts_f32)size;
}
static void mean_squared_grad(ts_tensor* in_out, const ts_tensor* desired_out) {
    ts_u64 size = (ts_u64)in_out->shape.width * in_out->shape.height * in_out->shape.depth;

    ts_f32* in_out_data = (ts_f32*)in_out->data;
    ts_f32* d_out_data = (ts_f32*)desired_out->data;

    for (ts_u64 i = 0; i < size; i++) {
        in_out_data[i] = (in_out_data[i] - d_out_data[i]);
    }
}

static ts_f32 cce_func(const ts_tensor* in, const ts_tensor* desired_out) {
    ts_f32 sum = 0.0f;

    ts_f32* in_data = (ts_f32*)in->data;
    ts_f32* d_out_data = (ts_f32*)desired_out->data;

    ts_u64 size = (ts_u64)in->shape.width * in->shape.height * in->shape.depth;
    for (ts_u64 i = 0; i < size; i++) {
        sum += d_out_data[i] * logf(in_data[i]);
    }

    return -sum;
}
static void cce_grad(ts_tensor* in_out, const ts_tensor* desired_out) {
    ts_u64 size = (ts_u64)in_out->shape.width * in_out->shape.height * in_out->shape.depth;

    ts_f32* in_out_data = (ts_f32*)in_out->data;
    ts_f32* d_out_data = (ts_f32*)desired_out->data;

    for (ts_u64 i = 0; i < size; i++) {
        in_out_data[i] = -d_out_data[i] / (in_out_data[i] + 1e-8f);
    }
}

#endif // TS_TENSOR_BACKEND == TS_TENSOR_BACKEND_CPU

