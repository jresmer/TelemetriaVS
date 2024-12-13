#include <stdlib.h>
#include <stdbool.h>
// Hard Margin SVM model for battery estimation based on read voltage (solved through the dual formulation)

// computes the doct product between a and b, where a and b are vectors of size "size" and stores the result onto the variable "result"
// will be used as the kernel function k(xn, xm)
void dotProduct (float* a, float* b, unsigned int size, float* result) {
    (* result) = 0;
    for (int i = 0; i < size; i++)
        (* result) += a[i] * b[i];
}

// computes the resulting vector of the nonlinear transformation ϕ(x)
// will be used to enable the model to pick from a hypothesis set where the relation between input and output is nonlinear
/* 
two good possible hypothesis sets are:
h(x) = w1 + w2.x1 + w3.x1^2
h(x) = w1 + w2.e^x1
h(x) = w1 + w2.x1 (no nonlinear transformation)
*/
void linearTransformation(float* x, float* w, float* result) {
    (* result) = w[0] + w[1]*x[0] + w[2]*x[0]*x[0];
}

// checks if lagrange multiplier a follows the kkt conditions
bool kkt(float* w, float a, float x, float y, float c, float error) {

    float yhxn;
    linearTransformation(&x, w, &yhxn);
    yhxn = yhxn * y;
    bool followsKKT = false;
    float diff = yhxn - 1;
    // a(n) = 0 y(n)h(x(n)) ≥ 1 | Not support vectors
    if (!a)
        followsKKT = diff >= error;
    // a(n) = C y(n)h(x(n)) ≤ 1 | Support vectors on or violating the margin
    else if (a == c) 
        followsKKT = yhxn <= error;
    // 0 < a(n) < C y(n) h(x(n)) = 1 | Support vectors on the margin
    else
        followsKKT = yhxn <= error && yhxn >= 0;

    return followsKKT;
}

// implementation of a linked list
// to list of lagrange multipliers that don't follow kkt conditions
// creates list with first element e of type int
int* createList() {}

// adds element e of type int to the linked list of first element "first"
void addToList(int* first, float e) {}

// removes element e of index i of to the linked list of first element "first"
void removeFromList(int* first, int i) {}

// returns element e of index i of the linked list of first element "first"
int at(int* first, int i) {}

// returns the size of the liked list refered to by the first element "first"
int getSize(int* first) {}

// deletes the liked list refered to by the first element "first"
void delete(int* first) {}

// optimizes the lagrange multipliers in order to train the model
void train(float* w, float* a, int size, float* x, float* y, float c, float error, int max_iterations) {
    float ai, aj;

    for (int i = 0; i < max_iterations; i++) {
        // selecting ai
        if (i % 2 == 0) {
            int* o0 = createList();
            for (int ii = 0; ii < size; ii++) {
                if (!kkt(w, a[i], x[i], y[i], c, error))
                    addToList(o0, i);
            }
            int index = rand() % getSize(o0);
            ai = at(o0, index);
        } else {
            int* o0 = createList();
            for (int ii = 0; ii < size; ii++) {
                if (!kkt(w, a[i], x[i], y[i], c, error) && 0 < a[i] && a[i] < c)
                    addToList(o0, i);
            }
            int index = rand() % getSize(o0);
            ai = at(o0, index);
        }
    }
}

void main() {}
