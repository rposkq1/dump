#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <gmp.h>

// Compute factorial(n) and store in result
void factorial(mpz_t result, unsigned int n) {
    mpz_set_ui(result, 1);
    for (unsigned int i = 2; i <= n; i++) {
        mpz_mul_ui(result, result, i);
    }
}

// Convert decimal to factoradic
void dec_to_factoradic(mpz_t number, int **digits_ptr, int *length) {
    mpz_t fact, quotient, remainder;
    mpz_inits(fact, quotient, remainder, NULL);

    unsigned int i = 0;
    factorial(fact, i);
    while (mpz_cmp(fact, number) <= 0) {
        i++;
        factorial(fact, i);
    }
    i--;

    int *digits = malloc((i + 1) * sizeof(int));
    int index = 0;

    for (; i >= 1; i--) {
        factorial(fact, i);
        mpz_fdiv_qr(quotient, remainder, number, fact);
        digits[index++] = mpz_get_ui(quotient);
        mpz_set(number, remainder);
    }
    digits[index++] = mpz_get_ui(number);

    *digits_ptr = digits;
    *length = index;

    mpz_clears(fact, quotient, remainder, NULL);
}

// Convert factoradic to decimal
void factoradic_to_dec(mpz_t result, int *digits, int length) {
    mpz_t fact, term;
    mpz_inits(fact, term, NULL);
    mpz_set_ui(result, 0);

    for (int i = 0; i < length; i++) {
        factorial(fact, length - 1 - i);
        mpz_mul_ui(term, fact, digits[i]);
        mpz_add(result, result, term);
    }

    mpz_clears(fact, term, NULL);
}

// Convert factoradic to permutation
void factoradic_to_permutation(int *factoradic, int length, int *permutation) {
    int *available = malloc(length * sizeof(int));
    for (int i = 0; i < length; i++) available[i] = i;

    for (int i = 0; i < length; i++) {
        int index = factoradic[i];
        if (index < 0 || index >= length - i) {
            fprintf(stderr, "Error: Factoradic digit %d out of bounds at position %d\n", index, i);
            free(available);
            exit(EXIT_FAILURE);
        }
        permutation[i] = available[index];
        for (int j = index; j < length - i - 1; j++) {
            available[j] = available[j + 1];
        }
    }
    free(available);
}

// Convert permutation to factoradic
void permutation_to_factoradic(int *perm, int length, int *factoradic) {
    bool *seen = calloc(length, sizeof(bool));
    for (int i = 0; i < length; i++) {
        if (perm[i] < 0 || perm[i] >= length || seen[perm[i]]) {
            fprintf(stderr, "Error: Invalid or duplicate value %d in permutation.\n", perm[i]);
            free(seen);
            exit(EXIT_FAILURE);
        }
        seen[perm[i]] = true;
    }
    free(seen);

    int *available = malloc(length * sizeof(int));
    for (int i = 0; i < length; i++) available[i] = i;

    for (int i = 0; i < length; i++) {
        int index = 0;
        while (available[index] != perm[i]) index++;
        factoradic[i] = index;
        for (int j = index; j < length - 1 - i; j++) {
            available[j] = available[j + 1];
        }
    }
    free(available);
}

// Read comma-separated digits from file or stdin if fp == NULL
int read_digits_input(FILE *fp, int **digits_ptr) {
    char buffer[8192];
    if (fp == NULL) {
        printf("Enter comma-separated digits: ");
        if (!fgets(buffer, sizeof(buffer), stdin)) return -1;
    } else {
        if (!fgets(buffer, sizeof(buffer), fp)) return -1;
    }

    int *digits = malloc(4096 * sizeof(int));
    int count = 0;
    char *token = strtok(buffer, ",\n");
    while (token) {
        digits[count++] = atoi(token);
        token = strtok(NULL, ",\n");
    }
    *digits_ptr = digits;
    return count;
}

// Read decimal number from file or stdin if fp == NULL
int read_decimal_input(FILE *fp, mpz_t number) {
    if (fp == NULL) {
        printf("Enter decimal number: ");
        if (gmp_scanf("%Zd", number) != 1) return -1;
    } else {
        char buffer[8192];
        if (!fgets(buffer, sizeof(buffer), fp)) return -1;
        if (mpz_set_str(number, buffer, 10) != 0) return -1;
    }
    return 0;
}

void print_array(FILE *fp, const char *label, int *arr, int len) {
    if (label) fprintf(fp, "%s: [", label);
    for (int i = 0; i < len; i++) {
        fprintf(fp, "%d", arr[i]);
        if (i < len - 1) fprintf(fp, ", ");
    }
    if (label) fprintf(fp, "]\n");
    else fprintf(fp, "\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s mode [inputfile] [outputfile]\n", argv[0]);
        fprintf(stderr, "Modes:\n");
        fprintf(stderr, " 1 = Decimal to Factoradic\n");
        fprintf(stderr, " 2 = Factoradic to Decimal\n");
        fprintf(stderr, " 3 = Factoradic to Permutation\n");
        fprintf(stderr, " 4 = Permutation to Factoradic\n");
        return EXIT_FAILURE;
    }

    int mode = atoi(argv[1]);
    if (mode < 1 || mode > 4) {
        fprintf(stderr, "Invalid mode. Must be 1, 2, 3, or 4.\n");
        return EXIT_FAILURE;
    }

    FILE *fin = NULL;
    FILE *fout = NULL;

    if (argc >= 3) {
        fin = fopen(argv[2], "r");
        if (!fin) {
            perror("Failed to open input file");
            return EXIT_FAILURE;
        }
    }
    if (argc >= 4) {
        fout = fopen(argv[3], "w");
        if (!fout) {
            perror("Failed to open output file");
            if (fin) fclose(fin);
            return EXIT_FAILURE;
        }
    }
    // Use stdout if no output file
    if (!fout) fout = stdout;

    if (mode == 1) {
        mpz_t number;
        mpz_init(number);
        if (read_decimal_input(fin, number) != 0) {
            fprintf(stderr, "Failed to read decimal input.\n");
            if (fin) fclose(fin);
            if (fout != stdout) fclose(fout);
            mpz_clear(number);
            return EXIT_FAILURE;
        }

        int *digits = NULL;
        int len = 0;
        dec_to_factoradic(number, &digits, &len);
        print_array(fout, "Factoradic", digits, len);
        free(digits);
        mpz_clear(number);

    } else if (mode == 2) {
        int *digits = NULL;
        int len = read_digits_input(fin, &digits);
        if (len <= 0) {
            fprintf(stderr, "Failed to read factoradic digits input.\n");
            if (fin) fclose(fin);
            if (fout != stdout) fclose(fout);
            return EXIT_FAILURE;
        }
        mpz_t result;
        mpz_init(result);
        factoradic_to_dec(result, digits, len);
        gmp_fprintf(fout, "Decimal: %Zd\n", result);
        mpz_clear(result);
        free(digits);

    } else if (mode == 3) {
        int *digits = NULL;
        int len = read_digits_input(fin, &digits);
        if (len <= 0) {
            fprintf(stderr, "Failed to read factoradic digits input.\n");
            if (fin) fclose(fin);
            if (fout != stdout) fclose(fout);
            return EXIT_FAILURE;
        }
        int *perm = malloc(len * sizeof(int));
        factoradic_to_permutation(digits, len, perm);
        print_array(fout, "Permutation", perm, len);
        free(digits);
        free(perm);

    } else if (mode == 4) {
        int *perm = NULL;
        int len = read_digits_input(fin, &perm);
        if (len <= 0) {
            fprintf(stderr, "Failed to read permutation input.\n");
            if (fin) fclose(fin);
            if (fout != stdout) fclose(fout);
            return EXIT_FAILURE;
        }
        int *factoradic = malloc(len * sizeof(int));
        permutation_to_factoradic(perm, len, factoradic);
        print_array(fout, "Factoradic", factoradic, len);
        free(perm);
        free(factoradic);
    }

    if (fin) fclose(fin);
    if (fout != stdout) fclose(fout);

    return 0;
}

