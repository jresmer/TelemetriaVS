// SVM regression model for battery estimation based on read voltage (solved through the dual formulation)
/*
max(-1/2.∑((αi - αi*).(αj - αj*).k(xi, xj)) - ε.∑(αi + αi*) + ∑(yi(αi + αi*)))
subjecto to: ∑((αi - αi*)) = 0 and αi, αi* ∈ [0, C]
*/

// computes the doct product between a and b, where a and b are vectors of size "size" and stores the result onto the variable "result"
// will be used as the kernel function k(xn, xm)

#include <stdlib.h>
#include <stdbool.h>

float dotProduct (float* a, float* b, unsigned int d) {
    float result = 0;
    for (int i = 0; i < d; i++)
        result += a[i] * b[i];
    
    return result;
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

// estimates the output yi of input xi through the model
/* 
h(xi) = ∑(αn - αn*).k(xi, xn) + b, where n ∈ S
to estimate b take one suport vector m
b = ym - ∑(αn - αn*).k(xm, xn)
*/
float predict(float** x, float* y, float* a, float* a_, int i, int d, int* s, int n_sv) {
    // estimating b
    int m = s[0];
    float b = y[m];
    for (int k = 0; k < n_sv; k++) {
        int n = s[k];
        b -= (a[n] - a_[n])*dotProduct(x[m], x[n], d);
    }
    // predicting yi through h(xi) = ∑(αn - αn*).k(xi, xn) + b, where n ∈ S
    float yi = 0;
    for (int k = 0; k < n_sv; k++) {
        int n = s[k];
        yi += (a[n] - a_[n])*dotProduct(x[i], x[n], d);
    }
    yi += b;

    return yi;
}

void train(float** x, float* y, float* a, float* a_, int d, int dataset_size, int max_iterations) {
    // for as many iterations as max_iterations optimizes the dual form of the langrangian
    /*
    max{(-1/2).∑(αi - αi*).(αj - αj*).k(xi, xj) - ε.∑(αi + αi*) + ∑yi.(αi + αi*)}
    subject to:
    ∑(αi - αi*) = 0 and αi, αi* ∈ [0, C]

    the algorithm to the optimization is as follows:
        take an i, j according to heuristics
        optimizes for langrange multipliers of j contrained to the boundaries
        recaulculates langrange multipliers of i respecting the chnages in those o j and the constraints
        repeats
    */
    for (int o = 0; o < max_iterations; o++) {}
}

void main () {
    // read data
    int dataset_size;
    // TODO
    // train model
    // initializes langrange multipliers as [0 0 ... 0]
    float* a = (float *) malloc(dataset_size * sizeof(float));
    float* a_ = (float *) malloc(dataset_size * sizeof(float));
    // TODO
    // store trained model
    // TODO
    // frees up arrays a, a_
    free(a);
    free(a_);
    return 0;
}