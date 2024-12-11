// Hard Margin SVM model for battery estimation based on read voltage (solved through the dual formulation)

// computes the doct product between a and b, where a and b are vectors of size "size" and stores the result onto the variable "result"
// will be used as the kernel function k(xn, xm)
void dotProduct (float* a, float* b, size_t size, float* result) {
    (* result) = 0;
    for (size_t i = 0; i < size; i++)
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
void linearTransformation(float* x, float** result) {}

// optimizes the lagrange multipliers in order to train the model
void train(float* a, float* x, float* y) {}

void main() {

}
