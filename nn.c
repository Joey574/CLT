#include <immintrin.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

void display_help();
void training_summary(int e, int b, float lr, int fr);

void leaky_relu(float* a, float* b, size_t num_elements);
void leaky_relu_derivative(float* a, float* b, size_t num_elements);
void onehot_loss();

void dot_prod(float* a, float* b, float* c, size_t a_r, size_t a_c, size_t b_r, size_t b_c, bool clear);
void train_network(int epochs, int batch_size, float learning_rate, int validation_fr);

void forward_prop();
void backward_prop();

int main(int argc, char* argv[]) {
    int epochs = -1;
    int batch_size = -1;
    float learning_rate = 0.0f;

    int validation_frequency = -1;

    if (argc < 2) {
        display_help();
        return 0;
    }

    // check for help arg
    for(int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            display_help();
            return 0;
        }
    }

    // parse args
    for(int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-e") == 0) {
            if (argc > i + 1) {
                epochs = atoi(argv[i + 1]);
            }
        }

        if (strcmp(argv[i], "-lr") == 0) {
            if (argc > i + 1) {
                learning_rate = atof(argv[i + 1]);
            }
        }

        if (strcmp(argv[i], "-b") == 0) {
            if (argc > i + 1) {
                batch_size = atof(argv[i + 1]);
            }
        }

        if (strcmp(argv[i], "-fr") == 0) {
            if (argc > i + 1) {
                validation_frequency = atof(argv[i + 1]);
            }
        }
    }

    training_summary(epochs, batch_size, learning_rate, validation_frequency);
}

void training_summary(int e, int b, float lr, int fr) {
    printf("Network training with args:\n"
    "\tEpochs:             %d\n"
    "\tBatch size:         %d\n"
    "\tLearning rate:      %f\n"
    "\tValidation fr:      %d\n\n",
    e, b, lr, fr    
    );
}

void display_help() {
    printf("Neural Network (alpha)\n"
    "Usage: nn [-e epochs] [-lr learning_rate]\n"
    "          [-b batch_size] [-fr validation_frequency]\n"
    "\tCommand Summary:\n"
    "\t\t-e\t\tNumber of epochs to train for\n"
    "\t\t-lr\t\tLearning rate for training\n"
    "\t\t-b\t\tBatch size for training\n"
    "\t\t-fr\t\tHow many epochs before validation\n"    
    );
}

void leaky_relu(float* a, float* b, size_t num_elements) {
    size_t i = 0;
    
    for (; i <= num_elements; i += 8) {
        const __m256 a = _mm256_load_ps(&a[i]);
        const __m256 res = _mm256_max_ps(a, _mm256_setzero_ps());
        _mm256_store_ps(&b[i], res);
    }

    for(; i < num_elements; i++) {
        b[i] = a[i] > 0.0f ? a[i] : a[i] * 0.1f; 
    }
}
void leaky_relu_derivative(float* a, float* b, size_t num_elements) {
    for (int i = 0; i < num_elements; i++) {
        b[i] *= a[i] > 0.0f ? 1.0f : 0.1f; 
    }
}

void dot_prod(float* a, float* b, float* c, size_t a_r, size_t a_c, size_t b_r, size_t b_c, bool clear) {

    // TODO: implement clear functionallity
    for(size_t i = 0; i < a_r; i++) {
        for(size_t j = 0; j < b_r; j++) {
            const __m256 a = _mm256_load_ps(&a[i * a_r + j]);

            size_t k = 0;
            for(; k <= b_c; k += 8) {
                const __m256 b = _mm256_load_ps(&b[j * b_r + k]);
                const __m256 c = _mm256_load_ps(&c[i * b_c + k]);
                const __m256 res = _mm256_fmadd_ps(a, b, c);

                _mm256_store_ps(&c[i * b_c + k], res);
            }

            for(; k < b_c; k++) {
                c[i * b_c + k] += a[i * a_r + j] * b[j * b_r + k];
            }
        }
    }
}

void train_network(int epochs, int batch_size, float learning_rate, int validation_fr) {
    // TODO: actually implement it

    const int iterations = batch_size;
    for(int e = 0; e < epochs; e++) {
        
        for(int i = 0; i < iterations; i++) {
        }

        if (e % validation_fr == 0) {
            
        }
    }
}
