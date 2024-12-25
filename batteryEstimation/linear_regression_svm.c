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
// TODO - refactor order of the parameters
bool kkt(float** x, float* y, float* a, float* a_, int i, float c, float epsilon, float error, int d, int* s, int n_sv) {
    // Check bounds for Lagrange multipliers (0 ≤ αi,αi* ≤ C)
    if (a[i] < -error || a[i] > c + error || a_[i] < -error || a_[i] > c + error) {
        return false;
    }

    // Product constraint (αi.αi* = 0)
    // Due to floating point arithmetic, we check if the product is close to zero
    if (a[i] * a_[i] > error) {
        return false;
    }

    // Get prediction h(x) using provided predict function
    float pred = predict(x, y, a, a_, i, d, s, n_sv);
    
    // Calculate prediction error
    float pred_error = y[i] - pred;

    // Check KKT conditions for different cases
    if (a[i] > error && a[i] < c - error) {
        // If 0 < αi < C, then prediction error should be ε
        if (fabs(pred_error - epsilon) > error) {
            return false;
        }
    } else if (a_[i] > error && a_[i] < c - error) {
        // If 0 < αi* < C, then prediction error should be -ε
        if (fabs(pred_error + epsilon) > error) {
            return false;
        }
    } else if (a[i] < error && a_[i] < error) {
        // If α = αi* = 0, then -ε ≤ prediction error ≤ ε
        if (fabs(pred_error) > epsilon + error) {
            return false;
        }
    } else if (a[i] > c - error) {
        // If αi = C, then prediction error ≥ ε
        if (pred_error < epsilon - error) {
            return false;
        }
    } else if (a_[i] > c - error) {
        // If αi* = C, then prediction error ≤ -ε
        if (pred_error > -epsilon + error) {
            return false;
        }
    }

    return true;
}

// signum function implementation for float values
int sign(float x) {
    
    if (-4E-4 < x && x < 4E-4) return 0;
    if else (0.0f < x) return 1;
    else return -1;
}

// step function implementation
int step(float x) {
    int a = x;
    if (x > a) {
        return a + 1;
    } else {
        return a;
    }
}

// updates lagrange multipliers for i and j
/*
update rule based off of:
Machine Learning, 46, 271–290, 2002
c 2002 Kluwer Academic Publishers. Manufactured in The Netherlands.
*/
void update_lagrange_multipliers(int i, int j, float c, int d, float epsilon, int dataset_size, float* a, float* a_, float* w, float** x, float* y, float* s, int n_sv) {
    // s∗ = λu* + λv*
    float z = (a[i] - a_[i]) + (a[j] - a_[j]);
    // η = kvv + kuu − 2kuv ;
    float ita = dotProduct(x[i], x[i]) + dotProduct(x[j], x[j]) - 2 * dotProduct(x[i], x[j]);
    // delta = 2ε/η;
    float delta = 2 * epsilon / ita;

    // λi = αi - αi*
    // calculate new λj based off the value of the old one
    float lj_new = (a[j] - a_[j]) + 1/ita * (y[j] - y[i] + predict(x, y, a, a_, j, s, n_sv) - predict(x, y, a, a_, i, s, n_sv));
    float li_new = z - lj_new;

    // if the multipliers differ in sign adjust the multipliers
    if (lj_new * li_new < 0.0f) {
        if (fabs(lj_new) >= delta && fabs(li_new) >= delta) {
            // λv = λv − sgn(λv ) · delta
            lj_new = lj_new - sign(lj_new) * delta;
        } else {
            // λv = step(|λv |−|λu |) · s∗
            lj_new = step(fabs(lj_new) - fabs(li_new)) * z;
        }
    }
    // L = max(s∗ − C, −C)
    float l_bound, h_bound, aux;
    aux = z - c;
    if (aux > -c) {
        l_bound = aux;
    } else {
        l_bound = -c;
    }
    // H = min(s∗ + C, C)
    aux = z + c;
    if (aux > c) {
        h_bound = aux;
    } else {
        h_bound = c;
    }
    // λv = min(max(λv , L), H)
    if (lj_new < l_bound) {
        lj_new = l_bound;
    }
    if (lj_new > h_bound) {
        lj_new = h_bound;
    }
    // λu = s∗ − λv
    li_new = z - lj_new;
    // updating lagrange multipliers
    if (sign(lj_new)) {
        a[j] = lj_new;
        a_[j] = 0;
    } else {
        a_[j] = lj_new;
        a[j] = 0; 
    }
    if (sign(li_new)) {
        a[i] = li_new;
        a_[i] = 0;
    } else {
        a_[i] = li_new;
        a[i] = 0; 
    }
}

// lagrangean function
/*
L(a) = -1/2.∑((αi - αi*).(αj - αj*).k(xi, xj)) - ε.∑(αi + αi*) + ∑(yi(αi + αi*))
subjecto to: ∑((αi - αi*)) = 0 and αi, αi* ∈ [0, C]
*/
float lagrangean(float* a, float* a_, float** x, float* y, float epsilon, int dataset_size, int d) {
    float term1 = 0;
    float term2 = 0;
    float term2 = 0;

    for (int i = 0; i < dataset_size; i++) {
        // (αi + αi*)
        term2 += (a[i] + a_[i]);
        // (yi(αi + αi*)
        term3 += y[i] * (a[i] + a_[i]);
        for (int j = 0; j < dataset_size; j++) {
            // (αi - αi*).(αj - αj*).k(xi, xj)
            term1 += (a[i] - a_[i]) * (a[j] - a_[j]) * dotProduct(x[i], x[j], d);
        }
    }

    // -1/2.∑((αi - αi*).(αj - αj*).k(xi, xj)) - ε.∑(αi + αi*) + ∑(yi(αi + αi*))
    return -0.5f * term1 -epsilon * term2 + term3;
}

// estimates the output yi of input xi through the model
/* 
h(xi) = ∑(αn - αn*).k(xi, xn) + b, where n ∈ S
to estimate b take one suport vector m
b = ym - ∑(αn - αn*).k(xm, xn)
*/
// TODO - refactor order of the parameters
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

void train(int d, int dataset_size, int target_n_improvements, int max_iterations, float epsilon, float error, int c, float* a, float* a_, float* w, float** x, float* y) {
    // for as many iterations as max_iterations optimizes the dual form of the langrangian
    /*
    max{(-1/2).∑(αi - αi*).(αj - αj*).k(xi, xj) - ε.∑(αi + αi*) + ∑yi.(αi + αi*)}
    subject to:
    ∑(αi - αi*) = 0 and αi, αi* ∈ [0, C]

    the algorithm to the optimization is as follows:
        pick  i and j according to heuristics
        optimizes for langrange multipliers of j contrained to the boundaries
        recaulculates langrange multipliers of i respecting the chnages in those o j and the constraints
        repeats
    */
    float threshold = pow(epsilon, 0.5);
    float old_lagrangean;
    old_lagrangean = lagrangean(a, a_, w, x, y);
    int lagrange_multipliers[dataset_size];
    int list_size = 0;
    float ai, a_i;
    // TODO - comment here
    // array 
    int s[dataset_size];
    for (int ii = 0; ii < dataset_size; ii++)
        s[ii] = ii;

    int n_improvements = 0;
    for (int o = 0; o < max_iterations; o++) {
        // resets the list
        list_size = 0;
        // breaks out of loop if the target number of improvements in the lagrangean has been achieved
        if (n_improvements >= target_n_improvements)
            break;
        // selecting ai
        // alternates between the two heuristics for ai selection
        if (o % 2) {
            for (int ii = 0; ii < dataset_size; ii++) {
                // ai and a_i that do not satisfy the kkt conditions within a certain error 
                // (float** x, float* y, float* a, float* a_, int i, float c, float epsilon, float error, int d, int* s, int n_sv)
                if (!kkt(x, y, a, a_, ii, c, epsilon, error, d, s, dataset_size)) {
                    lagrange_multipliers[list_size] = ii;
                    list_size++;
                }
            }
        } else {
            for (int ii = 0; ii < dataset_size; ii++) {
                // ai and a_i that do not satisfy the kkt conditions within a certain error and belong to the interval [0, c]
                if (!kkt(x, y, a, a_, ii, c, epsilon, error, d, s, dataset_size) && 0 < a[ii] && a[ii] < c && 0 < a_[ii] && a_[ii] < c) {
                    lagrange_multipliers[list_size] = ii;
                    list_size++;
                }
            }
        }
        // if no training example violates the kkt conditions then the global minumum has been reached
        if (!list_size) break;
        // recovering
        int i = lagrange_multipliers[rand() % list_size];
        ai = a[i];
        a_i = a_[i];
        // picking aj
        // first heuristic: largest change in (aj - a_j) estimated by |Ei - Ej|
        int j;
        float largest_change, change;
        largest_change = 0;
        for (int ii = 0; ii < dataset_size; ii++) {
            change = abs(predict(x, y, a, a_, i, d, s, dataset_size) - y[i] - predict(x, y, a, a_, i, d, s, dataset_size));
            if (change > largest_change) {
                largest_change = change;
                j = ii;
            }
        }
        // calculate updates on lagrange multipliers and verify improvement in the cost
        float new_a[dataset_size];
        float new_a_[dataset_size];
        for (int ii = 0; ii < dataset_size; ii++) {
            new_a[ii] = a[ii];
            new_a_[ii] = a_[ii];
        }
        update_lagrange_multipliers(i, j, c, d, epsilon, dataset_size, new_a, new_a_, w, x, y, s, n_sv);
        // if the lagrangean improved attribute the new values to ai and aj and continue to the next iteration
        if (new_lagrangeam - old_lagrangean > threshold) {
            old_lagrangean = new_lagrangean;
            a[i] = new_a[i];
            a[j] = new_a[j];
            n_improvements++;
            continue;
        }
        new_a[i] = a[i];
        new_a[j] = a[j];
        // second heuristic: pick each 0 < aj < c in turn
        for (int j = 0; j < dataset_size; j++) {
            if (0 < a[j] && a[j] < c && 0 < a_[j] && a_[j] < c) {
                // calculate updates on lagrange multipliers and verify improvement in the cost
                update_lagrange_multipliers(i, j, c, d, epsilon, dataset_size, new_a, new_a_, w, x, y, s, n_sv);
                // if the lagrangean improved attribute the new values to ai and aj and continue to the next iteration
                float new_lagrangeam = lagrangean(new_a, new_a_, w, x, y);
                if (new_lagrangeam - old_lagrangean > threshold) {
                    old_lagrangean = new_lagrangean;
                    a[i] = new_a[i];
                    a[j] = new_a[j];
                    n_improvements++;
                    continue;
                }
            }
        }
        // third heuristic iterate through the rest of the training set
        for (int j = 0; j < dataset_size; j++) {
            if (!(0 < a[j] && a[j] < c && 0 < a_[j] && a_[j] < c)) {
                // calculate updates on lagrange multipliers and verify improvement in the cost
                update_lagrange_multipliers(i, j, c, d, epsilon, dataset_size, new_a, new_a_, w, x, y, s, n_sv);
                // if the lagrangean improved attribute the new values to ai and aj and continue to the next iteration
                if (new_lagrangeam - old_lagrangean > threshold) {
                    old_lagrangean = new_lagrangean;
                    a[i] = new_a[i];
                    a[j] = new_a[j];
                    n_improvements++;
                    continue;
                }
            }
        }
        // TODO - update threshold
        // fourth heuristic replace ai and try again
        // in this case we just don't increment variable n_improviments
    }
}

int main () {
    // read data
    int dataset_size;
    // TODO
    // train model
    // initializes langrange multipliers as [0 0 ... 0]
    float* a = (float *) malloc(dataset_size * sizeof(float));
    float* a_ = (float *) malloc(dataset_size * sizeof(float));
    // TODO
    // store trained model
    // list support vectors
    // TODO
    // frees up arrays a, a_
    free(a);
    free(a_);
    return 0;
}