// Nonlinear Regression model for battery estimation based on read voltage
// constant e
const float e = 2.7182;
/*
TODO - review calculations and review GD for nonlinear regression
as of now the implementation corresponds to an intuition of how the optimization works

hypothesis set:
h(xi) = w1 + w2.e^(xi.w3)
cost function:
E(w) = (1/2n).∑[(h(xi) - yi)^2]
derivative of h:
h' = [1, xi.w3.w2^(xi.w3 - 1), w2.xi.e^(xi.w3)]
derivative of E:
E' = (1/n).∑[2h.h' - 2h'yi + 0]
*/
void costGradient(float* w, float* x, float* y, int n, float* result) {
    
    float hxi, h_;
    for (int i; i < n; i++) {
        hxi = w[0] + w[1]*pow(e, (x[i]*w[2]));
        // relative to w1
        result[0] += 2*hxi - 2*y[i];
        // relative to w2
        h_ = (x[i]*w[2]*pow(w[1], (x[i]*w[2] - 1)));
        result[1] += 2*hxi*h_ - 2*h_*y[i];
        //relative to w3
        h_ = w[1]*x[i]*pow(e, (x[i]*w[2]));
        result[2] += 2*hxi*h_ - 2*h_*y[i];
    }

    for (int i; i < 3; i++)
        result[i] = result[i] / n;
}

void gradientDescent(float* w, float* x, float* y, int n, int step, int max_iterations) {

    for (int i = 0; i < max_iterations; i++) {
        float gradient[3] = {0, 0, 0};
        costGradient(w, x, y, n, gradient);
        float sum = 0;
        for (int i = 0; i < 3; i++)
            sum += gradient[i];
        if (sum == 0)
            break;

        for (int ii = 0; ii < 3; ii++) {
            w[ii] -= step*gradient[ii];
        }
    }
}

void main () {}