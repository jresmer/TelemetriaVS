// SVM regression model for battery estimation based on read voltage (solved through the dual formulation)
/*
max(-1/2.∑((αi - αi*).(αj - αj*).k(xi, xj)) - ε.∑(αi + αi*) + ∑(yi(αi + αi*)))
subjecto to: ∑((αi - αi*)) = 0 and αi, αi* ∈ [0, C]
*/

// computes the doct product between a and b, where a and b are vectors of size "size" and stores the result onto the variable "result"
// will be used as the kernel function k(xn, xm)

#include <stdlib.h>
#include <stdbool.h>

void dotProduct (float* a, float* b, unsigned int d, float* result) {
    (* result) = 0;
    for (int i = 0; i < d; i++)
        (* result) += a[i] * b[i];
}

// checks if lagrange multiplier a follows the kkt conditions
bool kkt(float* w, float a, float* x, float y, float c, float error) {

    float yhxn;
    linearTransformation(x, w, &yhxn);
    yhxn = yhxn * y;
    bool followsKKT = false;
    float diff = yhxn - 1;
    // a(n) = 0 y(n)h(x(n)) ? 1 | Not support vectors
    if (!a)
        followsKKT = diff >= error;
    // a(n) = C y(n)h(x(n)) ? 1 | Support vectors on or violating the margin
    else if (a == c) 
        followsKKT = yhxn <= error;
    // 0 < a(n) < C y(n) h(x(n)) = 1 | Support vectors on the margin
    else
        followsKKT = yhxn <= error && yhxn >= 0;

    return followsKKT;
}

void main () {
    // read data
    // TODO
    // train model
    // TODO
    // store trained model
    // TODO

    return 0;
}