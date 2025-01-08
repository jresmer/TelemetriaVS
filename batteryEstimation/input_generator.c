/*
This scripts generates labeled data that will be used for testing the regression models before the actual data is collected
*/
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

void write_data (float** x, float* y, float* w, int size, int d, int dataset_size, int write_w) 
{

    FILE* file;
    file = fopen("labeled_data.txt", "wb");
    if (file == NULL) 
    {
        fprintf(stderr, "\nError opening file\n");
        exit(1);
    }
    int stored_values = 0;
    int flag;
    if (write_w) {
        // write w1 value to file labeled_data.txt
        flag = fwrite(w, size * sizeof(float), 1, file);
        if (!flag) 
        {
            printf("Error storing weights\n");
            exit(1);
        }
    }
    
    // writing training example x1 to file labeled_data.txt
    for (int i = 0; i < dataset_size; i++) 
    {
        flag = fwrite(x[i], sizeof(float), d, file);
        if (!flag) 
        {
            printf("Error storing input data at %dth training example\n", stored_values + 1);
            exit(1);
        }
        // wring target value yi to file labeled_data.txt
        flag = fwrite(&y[i], sizeof(float), 1, file);
        if (!flag) 
        {
            printf("Error storing input data at %dth training example\n", stored_values + 1);
            exit(1);
        }
        stored_values++;
    }
    
    fclose(file);
}

void generate_quadratic_data () 
{
    float w[2] = {(float) (rand() % 100 - 50) / 50.0f , (float) (rand() % 100 - 50) / 50.0f};
    float* x[1000];
    float y[1000];

    for (int i = 0; i < 1000; i++) 
    {
        x[i] = (float *) malloc(sizeof(float));
        x[i][0] = (i - 500) / 200;
        y[i] = w[0] + w[1] * pow(x[i][0], 2);
    }

    write_data(x, y, w, 2, 1, 1000, 0);
    
    for (int i = 0; i < 1000; i++) 
    {
        free(x[i]);
    }
}

void generate_exponential_data () 
{
    float w[3] = {(float) (rand() % 100 - 50) / 50.0f , (float) (rand() % 100 - 50) / 50.0f, (float) (rand() % 100 - 50) / 50.0f};
    float* x[1000];
    float y[1000];

    for (int i = 0; i < 1000; i++) 
    {
        x[i] = (float *) malloc(sizeof(float));
        x[i][0] = ((float) i - 500.0f) / 200.0f;
        y[i] = w[0] + w[1] * exp(w[2] * x[i][0]);
    }

    write_data(x, y, w, 3, 1, 1000, 1);
    
    for (int i = 0; i < 1000; i++) 
    {
        free(x[i]);
    }
}

void read_data (float* w, float* y, float** x, int d, int dataset_size) 
{
    // read data
    FILE* file;
    file = fopen("labeled_data.txt", "rb");
    if (file == NULL) 
    {
        fprintf(stderr, "\nError opening file\n");
        exit(1);
    }
    int flag = fread(w, sizeof(float), 3, file);
    for (int i = 0; i < dataset_size; i++) 
    {
        x[i] = (float *) malloc(d * sizeof(float));
        flag = fread(x[i], sizeof(float), d, file);
        flag = fread(&y[i], sizeof(float), 1, file);
    }
    
    fclose(file);
}

int main (int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: input_generator [input type]");
        exit(1);
    }
    int algo = atoi(argv[1]);
    switch (algo)
    {
    case '1':
        // generating quadratic data
        generate_quadratic_data();
        break;

    default:
        // generating exponential data
        generate_exponential_data();
        break;
    }
    
    return 1;
}