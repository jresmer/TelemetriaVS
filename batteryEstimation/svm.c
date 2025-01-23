// SVM regression model for battery estimation based on read voltage (solved through SMO)
/*
min(1/2.∑((αi - αi*).(αj - αj*).k(xi, xj)) + ε.∑(αi + αi*) - ∑(yi(αi + αi*)))
subjecto to: ∑((αi - αi*)) = 0 and αi, αi* ∈ [0, C]
*/
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

// signum function implementation as macro
#define SGN(x) ((x > 1e-4) ? 1 : ((x < -1e-4) ? -1 : 0))
// step function implementation as macro
#define STEP(x) ((x) > (int)(x) ? (int)(x) + 1 : (int)(x))

// computes the doct product between a and b, where a and b are vectors of size "size" and stores the result onto the variable "result"
// will be used as the kernel function k(xn, xm)
float dot (float* a, float* b, unsigned int d) {
    float result = 0;
    for (int i = 0; i < d; i++)
        result += a[i] * b[i];
    
    return result;
}

// estimates the output yi of input xi through the model, computes h(xi)
/* 
h(xi) = ∑(αn - αn*).k(xi, xn) + b, where n ∈ S
to estimate b take one suport vector m
b = ym - ∑(αn - αn*).k(xm, xn)
*/
float predict (float* a, float* a_, float** x, int* support_vectors, int size, int i, int d, float b) {
    // predicting yi through h(xi) = ∑(αn - αn*).k(xi, xn) + b, where n ∈ S
    float yi = b;
    int n;
    for (int k = 0; k < size; k++) {
        n = support_vectors[k];
        yi += (a[n] - a_[n]) * dot(x[i], x[n], d);
    }

    return yi;
}

// updates the value of the bias off of a support vector
/*
b = yi - h(xi), where i is a support vector
*/
float update_bias (float* a, float* a_, float** x, float* y, int support_vector, int d, int size) {
    float hxi = 0;
    for (int i = 0; i < size; i++) {
        hxi += (a[i] - a_[i]) * dot(x[i], x[support_vector], d);
    }
    return y[support_vector] - hxi;
}

// checks if lagrange multiplier a follows the kkt conditions
// Ei = |yi - h(xi)|
// will not check for unbounded multipliers as that would interfere with the multiplier selection heuristics
bool kkt (float λi, float Ei, float epsilon, float c, float tolerance) {

    float abs_λi = fabs(λi);
    // λi = 0 ⇐⇒ |yi − fi| < ε
    if (-tolerance < λi && λi < tolerance) {
        if (Ei >= epsilon + tolerance)
            return false;
    }
    // |λi| = C ⇐⇒ |yi − fi| > ε
    else if (c - tolerance < abs_λi && abs_λi < c + tolerance) {
        if (Ei <= epsilon - tolerance)
            return false;
    }
    // −C < λi != 0 < C ⇐⇒ |yi − fi| = ε
    else if (-c - tolerance < λi && λi < c + tolerance) {
        if (Ei > epsilon + tolerance || Ei < epsilon - tolerance)
            return false;
    }

    return true;
}

// updates lagrange multipliers for i and j
/*
update rule based off of:
Machine Learning, 46, 271–290, 2002
c 2002 Kluwer Academic Publishers. Manufactured in The Netherlands.
pseudo code for the analytical step as described on the paper:
    1. s* = λ*u + λ*v;
    2. η = kvv + kuu − 2kuv;
    3. delta = 2ε/η;
    4. λv = λ*v + 1/η.(yv − yu + f*u − f*v);
    5. λu = s* − λv ;
    6. if (λu · λv < 0) {
    7.  if (|λv | ≥ delta ∧ |λu | ≥ delta)
    8.      λv = λv − sgn(λv) · delta;
    9.  else
    10.     λv = step(|λv |−|λu|) · s∗;
    11. }
    12. L = max(s∗ − C, −C);
    13. H = min(C,s∗ + C);
    14. λv = min(max(λv, L), H);
    15. λu = s∗ − λv ;
obsevation: "*" is used to indicate and older/previous value
*/
void update_multipliers (float* a, float* a_, int u, int v, float* xu, float* xv, float yu, float yv, float fu, float fv, float c, float epsilon, int d) {
    // UTILITY FUNCTIONS FOR THE UPDATE RULE
    // max function implemented for two float values
    float max(float a, float b) {
        if (a >= b) return a;
        else return b;
    }
    // min function implemented for two float values
    float min(float a, float b) {
        if (a >= b) return b;
        else return a;
    }
    // s* = λ*u + λ*v;
    float s = a[u] - a_[u] - a[v] + a_[v];
    // η = kvv + kuu − 2kuv;
    float eta = dot(xv, xv, d) + dot(xu, xu, d) - 2 * dot(xu, xv, d);
    // delta = 2ε/η;
    float delta = 2 * epsilon / eta;
    // λv = λ*v + 1/η.(yv − yu + f*u − f*v);
    float λv = (a[v] - a_[v]) + 1 / eta * (yv - yu + fu - fv);
    // λu = s* − λv;
    float λu = s - λv;
    if (λv * λu < 0) {
        if (fabs(λv) >= delta && fabs(λu) >= delta)
            λv = λv - SGN(λv) * delta;
        else
            λv = STEP(fabs(λv) - fabs(λu)) * s;
    }
    // L = max(s∗ − C, −C);
    float L = max(s - c, -c);
    // H = min(C,s∗ + C);
    float H = min(c, s + c);
    // λv = min(max(λv, L), H);
    λv = min(max(λv, L), H);
    // λu = s∗ − λv ;
    λu = s - λv;
    // updating the separate multipliers on the array
    if (SGN(λv) == 1) {
        a[v] = λv;
        a_[v] = 0;
    } else {
        a[v] = 0;
        a_[v] = fabs(λv);
    }
    if (SGN(λu) == 1) {
        a[u] = λu;
        a_[u] = 0;
    } else {
        a[u] = 0;
        a_[u] = λu;
    }

}

// computes lagrangean
/*
L(a, a*) = 1/2.∑((αi - αi*).(αj - αj*).k(xi, xj)) + ε.∑(αi + αi*) - ∑(yi(αi + αi*))
subjecto to: ∑((αi - αi*)) = 0 and αi, αi* ∈ [0, C]
*/
float compute_lagrangean (float* a, float* a_, float** x, float* y, float epsilon, int dataset_size, int d) {
    float term1 = 0;
    float term2 = 0;
    float term3 = 0;

    for (int i = 0; i < dataset_size; i++) {
        // (αi + αi*)
        term2 += (a[i] + a_[i]);
        // yi(αi + αi*)
        term3 += y[i] * (a[i] + a_[i]);
        for (int j = 0; j < dataset_size; j++) {
            // (αi - αi*).(αj - αj*).k(xi, xj)
            term1 += (a[i] - a_[i]) * (a[j] - a_[j]) * dot(x[i], x[j], d);
        }
    }

    // 1/2.∑((αi - αi*).(αj - αj*).k(xi, xj)) + ε.∑(αi + αi*) - ∑(yi(αi + αi*))
    return 0.5f * term1 + epsilon * term2 - term3;
}

// updates threshold
/*
follows the threshold update rule described in:
Machine Learning, 46, 271–290, 2002
c 2002 Kluwer Academic Publishers. Manufactured in The Netherlands.
bu = yu − f*u + (λ*u − λu)kuu + (λ*v − λv)kuv + b* (17)
bv = yv − f*v + (λ*u − λu)kuv + (λ*v − λv)kvv + b* (18)
obsevation: "*" is used to indicate and older/previous value
*/
float update_threshold (float yu, float yv, float fu_, float fv_, float λu_, float λv_, float λu, float λv, float kuu, float kuv, float kvv, float threshold) {
    float bu = yu - fu_ + (λu_ - λu) * kuu + (λv_ - λv) * kuv + threshold;
    float bv = yv - fv_ + (λu_ - λu) * kuv + (λv_ - λv) * kvv + threshold;

    if (bu != bv) return (bu + bv) * 0.5f;
    return bu;
}

// SMO implementation
// returns the final bias
/*
based on what is proposed in:
Machine Learning, 46, 271–290, 2002
c 2002 Kluwer Academic Publishers. Manufactured in The Netherlands.
*/
float train (float* a, float* a_, float** x, float* y, float epsilon, float tolerance, float c, int d, int dataset_size) {
    /*
    min{(1/2).∑(αi - αi*).(αj - αj*).k(xi, xj) + ε.∑(αi + αi*) - ∑yi.(αi + αi*)}
    subject to:
    ∑(αi - αi*) = 0 and αi, αi* ∈ [0, C]

    the algorithm to the optimization is as follows:
        let the working set (I) be all data points for the first iteration or if the previous one made no progress
        otherwise let the working set (I) consist of data points with non-bounded lagrange multipliers
        for each u ∈ I do
            pick v according to heuristics
            optimizes for langrange multipliers of v contrained to the boundaries
            recaulculates langrange multipliers of u respecting the chnages in those o v and the constraints
        repeats until there is no improvement made in the last two iterations or the maximum iteration number is reached
    */
    // update attempt step
    bool step ( float* a,
                float* a_,
                float** x,
                float* y,
                int* support_vectors,
                int u,
                int v,
                int d,
                int heuristic,
                float c,
                float* b,
                float* threshold,
                float* lagrangean,
                size_t* sv_size,
                bool* improved_last_iter ) {
        // store old values
        float au = a[u];
        float au_ = a_[u];
        float av = a[v];
        float av_ = a_[v];
        // compute analytical step
        float fu = predict(a, a_, x, support_vectors, (*sv_size), u, d, (*b));
        float fv = predict(a, a_, x, support_vectors, (*sv_size), v, d, (*b));
        update_multipliers(a, a_, u, v, x[u], x[v], y[u], y[v], fu, fv, c, epsilon, d);
        // compute updated lagrangean
        float new_lagrangean = compute_lagrangean(a, a_, x, y, epsilon, dataset_size, d);
        if ((*lagrangean) - new_lagrangean > (*threshold)) {
            // validate multiplier update and add data points to the support vector list
            (*lagrangean) = new_lagrangean;
            support_vectors[(*sv_size)] = u;
            support_vectors[(*sv_size)+1] = v;
            (*sv_size) += 2;
            // cumpute kernels
            float kuu = dot(x[u], x[u], d);
            float kuv = dot(x[u], x[v], d);
            float kvv = dot(x[v], x[v], d);
            // update threshold
            (*threshold) = update_threshold(y[u], y[v], fu, fv, au - au_, av - av_, a[u] - a_[u], a[v] - a_[v], kuu, kuv, kvv, (*threshold));
            // update bias
            (*b) = update_bias(a, a_, x, y, support_vectors[0], d, dataset_size);
            (*improved_last_iter) = true;
            printf("improvement made with heuristic %d; u=%d, v=%d, L(a,a*)=%f, updated threshold=%f, updated bias=%f\n", heuristic, u, v, new_lagrangean, (*threshold), (*b));
            return true;
        } else {
            // revert update on the multipliers
            a[u] = au;
            a_[u] = au_;
            a[v] = av;
            a_[v] = av_;
            return false;
        }
    }
    // bias
    float b = 0;
    // compute initial lagrangean
    float lagrangean = compute_lagrangean(a, a_, x, y, epsilon, dataset_size, d);
    // support vector array
    // array size control variable
    int support_vectors[dataset_size];
    size_t sv_size = 0;
    // compute initial threshold
    float threshold = sqrt(epsilon);
    // working set array
    // array size control variable
    float working_set[dataset_size];
    size_t working_set_size;
    int in_working_set[dataset_size];
    for (int i = 0; i < dataset_size; i++) in_working_set[i] = 0;
    // improvement control variables
    bool improved_last_iter = false;
    bool improved_this_iter = false;

    for (int iteration = 0; iteration < dataset_size; iteration++) {
        // reset current iterations improvement control variable
        improved_this_iter = false;
        // determine working set
        working_set_size = 0;
        if (!improved_last_iter) {
            for (int i = 0; i < dataset_size; i++) {
                // computing Ei = |yi - h(xi)|
                float Ei = fabs(y[i] - predict(a, a_, x, support_vectors, sv_size, i, d, b));
                // all data points that violate kkt conditions
                if (!kkt(a[i] - a_[i], Ei, epsilon, c, tolerance)) {
                    working_set[working_set_size] = i;
                    in_working_set[i] = 1;
                    working_set_size++;
                }
            }
        } else {
            for (int i = 0; i < dataset_size; i++) {
                // computing Ei = |yi - h(xi)|
                float Ei = fabs(y[i] - predict(a, a_, x, support_vectors, sv_size, i, d, b));
                float λi = a[i] - a_[i];
                // unbounded data points that violate kkt conditions
                if ((-c > λi || λi > c) && !kkt(λi, Ei, epsilon, c, tolerance)) {
                    working_set[working_set_size] = i;
                    in_working_set[i] = 1;
                    working_set_size++;
                }
            }

        }
        printf("working set size = %d\n", (int) working_set_size);
        // iterate through the working set
        for (int i = 0; i < working_set_size; i++) {
            // recover u
            int u = working_set[i];
            // declare v
            int v;
            // first heuristic for v selection
            float largest_change = -10E5;
            float Eu = fabs(y[u] - predict(a, a_, x, support_vectors, sv_size, u, d, b));
            for (int k = 0; k < dataset_size; k++) {
                // Platt's heuristic: best change according to estimation |Eu - Ev|
                float change = fabs(Eu - fabs(y[k] - predict(a, a_, x, support_vectors, sv_size, k, d, b)));
                if (change > largest_change) {
                    v = k;
                    largest_change = change;
                }
            }
            if (step(a, a_, x, y, support_vectors, u, v, d, 1, c, &b, &threshold, &lagrangean, &sv_size, &improved_last_iter)) {
                improved_this_iter = true;
                continue;
            } 
            // second heuristic for v selection
            bool improved = false;
            for (int j = 0; j < working_set_size; j++) {
                v = working_set[j];
                if (step(a, a_, x, y, support_vectors, u, v, d, 2, c, &b, &threshold, &lagrangean, &sv_size, &improved_last_iter)) {
                    improved_this_iter = true;
                    improved = true;
                    break;
                }
            }
            if (improved) continue;
            // third heuristic for v selection
            for (int j = 0; j < dataset_size; j++) {
                if (in_working_set[j]) continue;
                if (step(a, a_, x, y, support_vectors, u, j, d, 3, c, &b, &threshold, &lagrangean, &sv_size, &improved_last_iter)) {
                    improved_this_iter = true;
                    break;
                }
            }
        }
        if (!improved_this_iter) {
            if (!improved_last_iter) {
                printf("did not improve within the last two iterations\n");
                break;
            }
            improved_last_iter = false;
            printf("did not improve this iteration\n");
        }
    }
    
    return b;

}

int main (int argc, char *argv[]) {
    // UTILITY COPY FUNCTION
    // Duff's device
    // copies strings 8 bytes a time
    void copy(char* to, char* from, size_t count) {
        size_t n = (count + 7) / 8;

        switch (count % 8) {
            case 0: do {
                *to++ = *from++;
                case 7: *to++ = *from++;
                case 6: *to++ = *from++;
                case 5: *to++ = *from++;
                case 4: *to++ = *from++;
                case 3: *to++ = *from++;
                case 2: *to++ = *from++;
                case 1: *to++ = *from++;
            } while (--n > 0);
        }
    }
    /*
    TODO add in between code
    */
    if (argc != 4) {
        printf("Usage: svm [dataset size] [dimensionality of the input] [filaname: char[18]]");
        exit(1);
    }
    // READ DATA
    // declaring variables
    int dataset_size, d, c, max_iterations, target_n_improviments;
    float epsilon, tolerance, b;
    float** x;
    float* y;

    dataset_size = atoi(argv[1]);
    d = atoi(argv[2]);
    char filename[18];
    copy(&filename[0], &argv[3][0], 18);
    // allocating memory
    y = (float *) malloc(dataset_size * sizeof(float));
    x = (float **) malloc(dataset_size * sizeof(float*));
    for (int i = 0; i < dataset_size; i++) {
        x[i] = (float *) malloc(d * sizeof(float));
    }
    // read data
    FILE* data;
    data = fopen(filename, "rb");
    if (data == NULL) {
        fprintf(stderr, "\nError opening file\n");
        exit(1);
    }
    int flag;
    for (int i = 0; i < dataset_size; i++) {
        flag = fread(x[i], sizeof(float), d, data);
        flag = fread(&y[i], sizeof(float), 1, data);
        printf("input=%f; target output=%f\n", x[i][0], y[i]);
    }
    fclose(data);
    // initializes langrange multipliers as [0 0 ... 0]
    float* a = (float *) malloc(dataset_size * sizeof(float));
    float* a_ = (float *) malloc(dataset_size * sizeof(float));
    for (int k = 0; k < dataset_size; k++) {
        a[k] = 0;
        a_[k] = 0;
    }
    // hyperparameters
    c = 10.0f; // TODO - update value
    epsilon = 0.0005f; // TODO - update value
    tolerance = 5E-6; // TODO - update value
    // train model
    // train (float* a, float* a_, float** x, float* y, float epsilon, float tolerance, float c, int d, int dataset_size)
    b = train(a, a_, x, y, epsilon, tolerance, c, d, dataset_size);
    printf("model trained\n");
    // determine which training examples are support vectors
    int s_[dataset_size];
    int size_of_s_ = 0;
    for (int i = 0; i < dataset_size; i++) {
        // support vectors are points where either αi is a non-zero or αi* is a non-zero
        if (a[i] || a_[i]) {
            s_[size_of_s_] = i;
            size_of_s_++;
        }
    }
    // store trained model:
    struct model {
        float* a;
        float* a_;
        float** x;
        float b;
        int size;
        int d; // Store dimensionality
    };
    // storing the values into the struct
    struct model m;
    printf("allocating data struct\n");
    m.a = malloc(size_of_s_ * sizeof(float));
    m.a_ = malloc(size_of_s_ * sizeof(float));
    m.x = malloc(size_of_s_ * sizeof(float*));
    for (int i = 0; i < size_of_s_; i++) {
        m.x[i] = malloc(d * sizeof(float));
        m.a[i] = a[s_[i]];
        m.a_[i] = a_[s_[i]];
        memcpy(m.x[i], x[s_[i]], d * sizeof(float));
    }
    m.b = b;
    m.size = size_of_s_;
    // TEST
    float prediction_error = 0;
    float prediction;
    int support_vectors[m.size];
    for (int i = 0; i < m.size; i++) support_vectors[i] = i;
    // (float* a, float* a_, float** x, int* support_vectors, int size, int i, int d, float b)
    for (int i = 5; i < dataset_size; i++) {
        prediction = predict(m.a, m.a_, x, support_vectors, m.size, i, d, m.b);
        printf("predicted value=%f; target value=%f\n", prediction, y[i]);
        prediction_error += fabs(prediction - y[i]);
    }
    printf("|h(x) - yi| = %f\n", prediction_error);
    // opening file
    // .txt extension in case the model is to be accessed through windows
    FILE* file;
    file = fopen("model.txt", "wb");
    if (file == NULL) {
        fprintf(stderr, "\nError opening file\n");
        exit(1);
    }
    // store struct into file
    flag = fwrite(&m, sizeof(struct model), 1, file);
    if (flag) 
        printf("Model successfully stored\n");
    else
        printf("Error storing the model\n");
    // closing file
    fclose(file);
    
    // frees up arrays a, a_
    free(a);
    free(a_);
    for (int i = 0; i < dataset_size; i++) free(x[i]);
    free(x);
    free(y);
    free(m.a);
    free(m.a_);
    for (int i = 0; i < m.size; i++) free(m.x[i]);
    free(m.x);

    return 0;
}