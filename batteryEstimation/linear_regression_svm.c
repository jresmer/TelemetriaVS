// SVM regression model for battery estimation based on read voltage (solved through the dual formulation)
/*
max(-1/2.∑((αi - αi*).(αj - αj*).k(xi, xj)) - ε.∑(αi + αi*) + ∑(yi(αi + αi*)))
subjecto to: ∑((αi - αi*)) = 0 and αi, αi* ∈ [0, C]
*/
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

// computes the doct product between a and b, where a and b are vectors of size "size" and stores the result onto the variable "result"
// will be used as the kernel function k(xn, xm)
float dotProduct (float* a, float* b, unsigned int d) {
    float result = 0;
    for (int i = 0; i < d; i++)
        result += a[i] * b[i];
    
    return result;
}

// estimates the output yi of input xi through the model
/* 
h(xi) = ∑(αn - αn*).k(xi, xn) + b, where n ∈ S
to estimate b take one suport vector m
b = ym - ∑(αn - αn*).k(xm, xn)
*/
// TODO - refactor order of the parameters
float predict (float** x, float* a, float* a_, float* xi, int d, int size, float b) {
    // predicting yi through h(xi) = ∑(αn - αn*).k(xi, xn) + b, where n ∈ S
    float yi = b;
    for (int k = 0; k < size; k++) {
        yi += (a[k] - a_[k])*dotProduct(xi, x[k], d);
    }

    return yi;
}

float update_b (float** x, float* y, float* a, float* a_, int d, int size) {
    // calculating b
    float yi = 0;
    for (int i = 0; i < size; i++) {
        if (!a[i] && !a_[i]) {
            continue;
        }
        float yi = 0;
        for (int k = 0; k < size; k++) {
            yi += (a[k] - a_[k])*dotProduct(x[i], x[k], d);
        }
        return yi - y[i];;
    }  
}

// checks if lagrange multiplier a follows the kkt conditions
// TODO - refactor order of the parameters
bool kkt (float** x, float* y, float* a, float* a_, float ai, float a_i, int i, float c, float epsilon, float error, int d, int n_sv) {
    float lambda_i = ai - a_i;
    // Get prediction h(x) using provided predict function
    float pred = predict(x, a, a_, x[i], d, n_sv, d);
    // Calculate prediction error
    float pred_error = fabs(y[i] - pred);

    // λi = 0 ⇐⇒ |yi − fi| < ε
    if (lambda_i < error && lambda_i > -error) {
        if (pred_error >= epsilon + error)
            return false;
    }
    // |λi| = C ⇐⇒ |yi − fi| > ε
    else if (lambda_i < c + error && lambda_i > c - error) {
        if (pred_error <= epsilon - error)
            return false;
    }
    // −C < λi != 0 < C ⇐⇒ |yi − fi| = ε
    else if (-c - error < lambda_i && lambda_i < c + error) {
        if (pred_error > epsilon + error || pred_error < epsilon - error)
            return false;
    }

    return true;
}

// signum function implementation for float values
int sign (float x) {
    
    if (-4E-4 < x && x < 4E-4) return 0;
    else if (0.0f < x) return 1;
    else return -1;
}

// step function implementation
int step (float x) {
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
void update_lagrange_multipliers (int i, int j, float c, int d, float epsilon, int dataset_size, float* a, float* a_, float** x, float* y, int n_sv, float b) {
    // the update rule is more numerically stable if λu > λv, therefore in case it is not we just switch them around
    float lambda_i = (a[i] - a_[i]);
    float lambda_j = (a[j] - a_[j]);
    if (lambda_j > lambda_i) {
        int aux = i;
        i = j;
        j = aux;
        float aux_ = lambda_i;
        lambda_i = lambda_j;
        lambda_j = aux_;
    }
    // s∗ = λu* + λv*
    float z = lambda_i + lambda_j;
    // η = kvv + kuu − 2kuv ;
    float eta = dotProduct(x[i], x[i], d) + dotProduct(x[j], x[j], d) - 2 * dotProduct(x[i], x[j], d);
    // delta = 2ε/η;
    float delta = 2 * epsilon / eta;

    // λi = αi - αi*
    // calculate new λj based off the value of the old one
    float lj_new = (a[j] - a_[j]) + 1/eta * (y[j] - y[i] + predict(x, a, a_, x[j], d, n_sv, b) - predict(x, a, a_, x[i], d, n_sv, b));
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
L(a) = 1/2.∑((αi - αi*).(αj - αj*).k(xi, xj)) + ε.∑(αi + αi*) - ∑(yi(αi + αi*))
subjecto to: ∑((αi - αi*)) = 0 and αi, αi* ∈ [0, C]
*/
float lagrangean (float* a, float* a_, float** x, float* y, float epsilon, int dataset_size, int d) {
    float term1 = 0;
    float term2 = 0;
    float term3 = 0;

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
    return 0.5f * term1 + epsilon * term2 - term3;
}

// threshold update function
float update_threshold (float yi, float yj, float fi, float fj, float li, float lj, float li_new, float lj_new, float* xi, float* xj, float b, int d) {
    float candidate_i, candidate_j;
    float diff_i = li - li_new;
    float diff_j = lj - lj_new;

    // bu = yu − fu* + (λu* − λu)kuu + (λv* − λv)kuv + b*
    candidate_i = (yi - fi) + diff_i * dotProduct(xi, xi, d) + diff_j * dotProduct(xi, xj, d) + b;
    // bv = yv − fv* + (λu* − λu)kuv + (λv* − λv)kvv + b*
    candidate_j = (yj - fj) + diff_i * dotProduct(xi, xj, d) + diff_j * dotProduct(xj, xj, d) + b;

    if (candidate_i != candidate_j) 
        return (candidate_i + candidate_j) / 2;

    return candidate_i;
}

float train (int d, int dataset_size, int max_iterations, float epsilon, float error, int c, float* a, float* a_, float** x, float* y) {
    // for as many iterations as max_iterations optimizes the dual form of the langrangian
    /*
    min{(1/2).∑(αi - αi*).(αj - αj*).k(xi, xj) + ε.∑(αi + αi*) - ∑yi.(αi + αi*)}
    subject to:
    ∑(αi - αi*) = 0 and αi, αi* ∈ [0, C]

    the algorithm to the optimization is as follows:
        pick  i and j according to heuristics
        optimizes for langrange multipliers of j contrained to the boundaries
        recaulculates langrange multipliers of i respecting the chnages in those o j and the constraints
        repeats
    */
    float sv_a[dataset_size];
    float sv_a_[dataset_size];
    size_t sv_size = 0;

    float threshold = pow(epsilon, 0.5);
    printf("new_threshold %f\n", threshold);
    float old_lagrangean, new_lagrangean;
    old_lagrangean = lagrangean(a, a_, x, y, epsilon, dataset_size, d);
    int lagrange_multipliers[dataset_size];
    int list_size = 0;
    float ai, a_i;
    float b = 0;
    int improved;
    int improved_last_iteration = 1;
    for (int o = 0; o < max_iterations; o++) {
        printf("iteration %d\n", o + 1);
        improved = 0;
        // resets the list
        list_size = 0;
        // selecting ai
        // alternates between the two heuristics for ai selection
        /*
        an alternative would be to only look for training examples that do not follow kkt conditions and are non bounded unless no progress was made last iteration
        following the pseudo code present in :
        Machine Learning, 46, 271–290, 2002
        c 2002 Kluwer Academic Publishers. Manufactured in The Netherlands.
        While further progress can be made:
        1. If this is the first iteration, or if the previous iteration made no progress, then
        let the working set be all data points.
        2. Otherwise, let the working set consist only of data points with non-bounded
        Lagrange multipliers.
        3. For all data points in the working set, try to optimize the corresponding
        Lagrange multiplier. To find the second Lagrange multiplier:
        3.1 Try the best one (found from looping over the non-bounded multipliers)
        according to Platt’s heuristic, or
        3.2 Try all among the working set, or
        3.3 Try to find one among the entire set of Lagrange multipliers.
        4. If no progress was made and the working set was all data points, then done
        */
        if (!improved) {
            for (int ii = 0; ii < dataset_size; ii++) {
                // ai and a_i that do not satisfy the kkt conditions within a certain error 
                // (float** x, float* y, float* a, float* a_, int i, float c, float epsilon, float error, int d, int* s, int n_sv)
                if (!kkt(x, y, sv_a, sv_a_, a[ii], a_[ii], ii, c, epsilon, error, d, sv_size)) {
                    lagrange_multipliers[list_size] = ii;
                    list_size++;
                }
            }
        } else {
            for (int ii = 0; ii < dataset_size; ii++) {
                // let the working set consist only of data points with non-bounded Lagrange multipliers
                float li = a[ii] - a_[ii];
                if ((-c > li || li > c) && !kkt(x, y, sv_a, sv_a_, a[ii], a_[ii], ii, c, epsilon, error, d, sv_size)) {
                    lagrange_multipliers[list_size] = ii;
                    list_size++;
                }
            }
        }
        // if no training example violates the kkt conditions then the global minumum has been reached
        if (!list_size) {
            if (!improved) break;
            printf("no vialations to the kkt conditions found\n");
            improved = 0;
            continue;
        }
        int i;
        /*
        for (int ii = 0; ii < dataset_size; ii++) {
            printf("ai, a*i = (%f, %f);  ", a[ii], a_[ii]);
        }
        printf("\n");
        */
        // For all data points in the working set, try to optimize the corresponding Lagrange multiplier
        printf("working set size: %d\n", list_size);
        for (int ii = 0; ii < list_size; ii++) {
            printf("working example number %d\n", ii + 1);
            int improved_this_iter = 0;
            // recovering
            //int i = lagrange_multipliers[rand() % list_size];
            i = lagrange_multipliers[ii];
            ai = a[i];
            a_i = a_[i];
            // picking aj
            // first heuristic: largest change in (aj - a_j) estimated by |Ei - Ej|
            int j;
            float largest_change, change;
            largest_change = 0;
            for (int ii = 0; ii < dataset_size; ii++) {
                float lambda_j = a[ii] - a_[ii];
                if (-c < lambda_j && lambda_j < c) continue;
                change = abs(predict(x, sv_a, sv_a_, x[i], d, sv_size, b) - y[i] - predict(x, sv_a, sv_a_, x[i], d, sv_size, b));
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
            update_lagrange_multipliers(i, j, c, d, epsilon, dataset_size, new_a, new_a_, x, y, dataset_size, b);
            new_lagrangean = lagrangean(new_a, new_a_, x, y, epsilon, dataset_size, d);
            // if the lagrangean improved attribute the new values to ai and aj and continue to the next iteration
            if (old_lagrangean - new_lagrangean > threshold) {
                printf("example %d of the working set improved with the 1st heuristc\n", i + 1);
                // update lagrangean
                old_lagrangean = new_lagrangean;
                // update bias
                float fi = predict(x, sv_a, sv_a_, x[i], d, sv_size, b);
                float fj = predict(x, sv_a, sv_a_, x[j], d, sv_size, b);
                threshold = update_threshold(y[i], y[j], fi, fj, a[i] - a_[i], a[j] - a_[j], new_a[i] - new_a_[i], new_a[j] - new_a_[j], x[i], x[j], threshold, d);
                b = update_b(x, y, a, a_, d, dataset_size);
                // update the lagrangean multipliers
                a[i] = new_a[i];
                a[j] = new_a[j];
                a_[i] = new_a_[i];
                a_[j] = new_a_[j];
                // updating support vectors arrays
                sv_a[sv_size] = a[i];
                sv_a_[sv_size] = a_[i];
                sv_a[sv_size+1] = a[j];
                sv_a_[sv_size+1] = a_[j];
                sv_size = sv_size + 2;
                improved = 1;
                printf("new_threshold %f\n", threshold);
                printf("new value for lagrange multipliers is ai=%f, a*i=%f, aj=%f, a*j=%f\n", a[i], a_[i], a[j], a_[j]);
                continue;
            }
            new_a[i] = a[i];
            new_a[j] = a[j];
            // second heuristic: pick each 0 < aj < c in turn
            for (int j = 0; j < dataset_size; j++) {
                if (0 < a[j] && a[j] < c && 0 < a_[j] && a_[j] < c) {
                    // calculate updates on lagrange multipliers and verify improvement in the cost
                    update_lagrange_multipliers(i, j, c, d, epsilon, dataset_size, new_a, new_a_, x, y, dataset_size, b);
                    // if the lagrangean improved attribute the new values to ai and aj and continue to the next iteration
                    new_lagrangean = lagrangean(new_a, new_a_, x, y, epsilon, dataset_size, d);
                    if (old_lagrangean - new_lagrangean > threshold) {
                        printf("example %d of the working set improved with the 2nd heuristc\n", i + 1);
                        // update lagrangean
                        old_lagrangean = new_lagrangean;
                        // update bias
                        float fi = predict(x, sv_a, sv_a_, x[i], d, sv_size, b);
                        float fj = predict(x, sv_a, sv_a_, x[j], d, sv_size, b);
                        threshold = update_threshold(y[i], y[j], fi, fj, a[i] - a_[i], a[j] - a_[j], new_a[i] - new_a_[i], new_a[j] - new_a_[j], x[i], x[j], threshold, d);
                        b = update_b(x, y, a, a_, d, dataset_size);
                        // update the lagrangean multipliers
                        a[i] = new_a[i];
                        a[j] = new_a[j];
                        a_[i] = new_a_[i];
                        a_[j] = new_a_[j];
                        improved = 1;
                        improved_this_iter = 1;
                        printf("new_threshold %f\n", threshold);
                        printf("new value for lagrange multipliers is ai=%f, a*i=%f, aj=%f, a*j=%f\n", a[i], a_[i], a[j], a_[j]);
                        break;
                    }
                }
            }
            if (improved_this_iter) continue;
            // third heuristic iterate through the rest of the training set
            for (int j = 0; j < dataset_size; j++) {
                if (!(0 < a[j] && a[j] < c && 0 < a_[j] && a_[j] < c)) {
                    // calculate updates on lagrange multipliers and verify improvement in the cost
                    update_lagrange_multipliers(i, j, c, d, epsilon, dataset_size, new_a, new_a_, x, y, dataset_size, b);
                    // if the lagrangean improved attribute the new values to ai and aj and continue to the next iteration
                    new_lagrangean = lagrangean(new_a, new_a_, x, y, epsilon, dataset_size, d);
                    if (old_lagrangean - new_lagrangean > threshold) {
                        printf("example %d of the working set improved with the 3rd heuristc\n", i + 1);
                        // update lagrangean
                        old_lagrangean = new_lagrangean;
                        // update bias
                        float fi = predict(x, sv_a, sv_a_, x[i], d, sv_size, b);
                        float fj = predict(x, sv_a, sv_a_, x[j], d, sv_size, b);
                        threshold = update_threshold(y[i], y[j], fi, fj, a[i] - a_[i], a[j] - a_[j], new_a[i] - new_a_[i], new_a[j] - new_a_[j], x[i], x[j], threshold, d);
                        b = update_b(x, y, a, a_, d, dataset_size);
                        // update the lagrangean multipliers
                        a[i] = new_a[i];
                        a[j] = new_a[j];
                        a_[i] = new_a_[i];
                        a_[j] = new_a_[j];
                        improved = 1;
                        improved_this_iter = 1;
                        printf("new_threshold %f\n", threshold);
                        printf("new value for lagrange multipliers is ai=%f, a*i=%f, aj=%f, a*j=%f\n", a[i], a_[i], a[j], a_[j]);
                        break;
                    }
                }
            }
        }
        if (!improved) {
            if (!improved_last_iteration) {
                printf("no improvement within the last two iterations\n");
                break;
            }
            printf("did not improve this iteration\n");
            improved_last_iteration = 0;
        } else {
            improved_last_iteration = 1;
        }

        // fourth heuristic replace ai and try again
        // in this case we just don't increment variable n_improviments
    }


    return b;
}

// utility copy function
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

int main (int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: svm [dataset size] [dimensionality of the input] [filaname: char[18]]");
        exit(1);
    } 
    // READ DATA
    // declaring variables
    int dataset_size, d, c, max_iterations, target_n_improviments;
    float epsilon, error, b;
    float** x;
    float* y;

    dataset_size = atoi(argv[1]);
    d = atoi(argv[2]);
    char filename[18];
    copy(&filename[0], &argv[3][0], 18);
    // allocating memory
    y = (float *) malloc(dataset_size * sizeof(float));
    x = (float **) malloc(dataset_size * sizeof(float*));
    for (int i = 0; i < dataset_size; i++) x[i] = (float *) malloc(d * sizeof(float));
    // read data
    FILE* data;
    data = fopen(filename, "rb");
    if (data == NULL) {
        fprintf(stderr, "\nError opening file\n");
        exit(1);
    }
    int flag;
    for (int i = 0; i < dataset_size; i++) {
        flag = fread(&y[i], sizeof(float), 1, data);
        flag = fread(x[i], sizeof(float), d, data);
        printf("input=%f; target output=%f\n", x[i][0], y[i]);
    }
    fclose(data);
    exit(1);
    // initializes langrange multipliers as [0 0 ... 0]
    float* a = (float *) malloc(dataset_size * sizeof(float));
    float* a_ = (float *) malloc(dataset_size * sizeof(float));
    for (int k = 0; k < dataset_size; k++) {
        a[k] = 0;
        a_[k] = 0;
    }
    // hyperparameters
    c = 1.5f; // TODO - update value
    max_iterations = dataset_size; // TODO - update value
    epsilon = 0.0005f; // TODO - update value
    error = 5E-6; // TODO - update value
    // train model
    b = train(d, dataset_size, max_iterations, epsilon, error, c, a, a_, x, y);
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
    struct model
    {
        float a[size_of_s_];
        float a_[size_of_s_];
        float* x[size_of_s_];
        float b;
        int size;
    };
    free(y);
    printf("allocating data struct\n");
    // storing the values into the struct
    struct model m;
    int n = 0;
    for (int i = 0; i < size_of_s_; i++) {
        n = s_[i];
        m.a[i] = a[n];
        m.a_[i] = a_[n];
        for (int k = 0; k < d; k++) {
            m.x[i] = (float *) malloc(sizeof(float));
            m.x[i][k] = x[n][k];
        }
    }
    m.b = b;
    m.size = size_of_s_;
    // TEST
    // predict (float** x, float* a, float* a_, float* xi, int d, int size, float b)
    for (int i = 0; i < dataset_size; i++) {
        printf("input=%f; target output=%f\n", x[i][0], y[i]);
    }
    exit(1);
    float prediction_error = 0;
    float prediction;
    for (int i = 0; i < dataset_size; i++) {
        prediction = predict(m.x, m.a, m.a_, x[i], 1, m.size, m.b);
        printf("predicted value=%f; target value=%f\n", prediction, y[i]);
        prediction_error += fabs(prediction - y[i]);
    }
    printf("|h(x) - yi| = %f\n", prediction_error/dataset_size);
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

    return 0;
}