input:
    shape = (28, 28, 1);

pooling:
    type = average;
    pool_size = (4, 4, 1);

flatten:

dense:
    size = 64;

activation:
    type = leaky_relu;

dense:
    size = 10;

activation:
    type = softmax;

