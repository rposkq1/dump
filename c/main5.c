#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <gmp.h>
#include <getopt.h>
#include <unistd.h>

// Helper to compute factorial
void factorial(mpz_t result, unsigned int n) {
    mpz_set_ui(result, 1);
    for (unsigned int i = 2; i <= n; i++) {
        mpz_mul_ui(result, result, i);
    }
}

// Convert number to factoradic (with fixed length)
void number_to_factoradic(mpz_t number, int *digits, int length) {
    mpz_t temp, div, rem;
    mpz_inits(temp, div, rem, NULL);
    mpz_set(temp, number);

    for (int i = 1; i < length; i++) {
        factorial(div, i);
    }

    for (int i = length - 1; i >= 0; i--) {
        factorial(div, i);
        mpz_fdiv_qr(div, rem, temp, div);
        digits[length - i - 1] = mpz_get_ui(div);
        mpz_set(temp, rem);
    }

    mpz_clears(temp, div, rem, NULL);
}

// Convert factoradic digits to number
void factoradic_to_number(mpz_t number, int *digits, int length) {
    mpz_t term, fact;
    mpz_inits(term, fact, NULL);
    mpz_set_ui(number, 0);
    for (int i = 0; i < length; i++) {
        factorial(fact, length - 1 - i);
        mpz_mul_ui(term, fact, digits[i]);
        mpz_add(number, number, term);
    }
    mpz_clears(term, fact, NULL);
}

// Convert factoradic to permutation
void factoradic_to_permutation(int *factoradic, int length, int *perm) {
    bool used[256] = {0};
    int avail[256], i, j, k;
    for (i = 0; i < length; i++) avail[i] = i;

    for (i = 0; i < length; i++) {
        k = factoradic[i];
        perm[i] = avail[k];
        for (j = k; j < length - i - 1; j++) {
            avail[j] = avail[j + 1];
        }
    }
}

// Convert permutation to factoradic
void permutation_to_factoradic(int *perm, int length, int *factoradic) {
    int avail[256];
    for (int i = 0; i < length; i++) avail[i] = i;

    for (int i = 0; i < length; i++) {
        int j = 0;
        while (avail[j] != perm[i]) j++;
        factoradic[i] = j;
        for (int k = j; k < length - 1 - i; k++) {
            avail[k] = avail[k + 1];
        }
    }
}

// Read digits from comma-separated string
int parse_digits(char *line, int *digits) {
    int count = 0;
    char *token = strtok(line, ",\n");
    while (token) {
        digits[count++] = atoi(token);
        token = strtok(NULL, ",\n");
    }
    return count;
}

// Read permutation from input
int read_perm(FILE *fp, int *digits) {
    char line[8192];
    if (!fgets(line, sizeof(line), fp)) return -1;
    return parse_digits(line, digits);
}

// Write array to output
void write_array(FILE *fp, int *arr, int len) {
    for (int i = 0; i < len; i++) {
        fprintf(fp, "%d%s", arr[i], i + 1 == len ? "\n" : ",");
    }
}

int main(int argc, char *argv[]) {
    int mode = 0, fixed = 0;
    char *infile = NULL, *outfile = NULL;
    FILE *in = stdin, *out = stdout;

    int opt;
    while ((opt = getopt(argc, argv, "m:i:o:f:h")) != -1) {
        switch (opt) {
            case 'm': mode = atoi(optarg); break;
            case 'i': infile = optarg; break;
            case 'o': outfile = optarg; break;
            case 'f': fixed = atoi(optarg); break;
            case 'h':
                printf("Modes:\n");
                printf(" 1: number -> factoradic\n");
                printf(" 2: factoradic -> number\n");
                printf(" 3: factoradic -> permutation\n");
                printf(" 4: permutation -> factoradic\n");
                printf(" 5: number -> permutation (needs -f)\n");
                return 0;
        }
    }

    if (infile) in = fopen(infile, "r");
    if (outfile) out = fopen(outfile, "w");

    if (mode == 1) {
        mpz_t num;
        mpz_init(num);
        gmp_fscanf(in, "%Zd", num);
        int *digits = malloc(fixed * sizeof(int));
        number_to_factoradic(num, digits, fixed);
        write_array(out, digits, fixed);
        free(digits);
        mpz_clear(num);

    } else if (mode == 2) {
        int digits[256], len = read_perm(in, digits);
        mpz_t result;
        mpz_init(result);
        factoradic_to_number(result, digits, len);
        gmp_fprintf(out, "%Zd\n", result);
        mpz_clear(result);

    } else if (mode == 3) {
        int digits[256], len = read_perm(in, digits);
        int *perm = malloc(len * sizeof(int));
        factoradic_to_permutation(digits, len, perm);
        write_array(out, perm, len);
        free(perm);

    } else if (mode == 4) {
        int perm[256], len = read_perm(in, perm);
        int *digits = malloc(len * sizeof(int));
        permutation_to_factoradic(perm, len, digits);
        write_array(out, digits, len);
        free(digits);

    } else if (mode == 5) {
        if (fixed <= 0) {
            fprintf(stderr, "-f required for mode 5\n");
            return 1;
        }
        mpz_t num;
        mpz_init(num);
        gmp_fscanf(in, "%Zd", num);
        int *digits = malloc(fixed * sizeof(int));
        number_to_factoradic(num, digits, fixed);
        int *perm = malloc(fixed * sizeof(int));
        factoradic_to_permutation(digits, fixed, perm);
        write_array(stdout, perm, fixed);
        if (out != stdout) write_array(out, perm, fixed);
        free(digits);
        free(perm);
        mpz_clear(num);
    }

    if (infile) fclose(in);
    if (outfile) fclose(out);
    return 0;
}

