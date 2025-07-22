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
    mpz_t fact, quotient, remainder, temp;
    mpz_inits(fact, quotient, remainder, temp, NULL);

    // Find the largest i such that i! <= number
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

    mpz_clears(fact, quotient, remainder, temp, NULL);
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
            printf("Error: Factoradic digit %d out of bounds at position %d\n", index, i);
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
            printf("Error: Invalid or duplicate value %d in permutation.\n", perm[i]);
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

// Helper: read comma-separated digits from user
int read_digits_input(int **digits_ptr) {
    char buffer[8192];
    printf("Enter comma-separated digits: ");
    if (!fgets(buffer, sizeof(buffer), stdin)) return -1;

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

void print_array(const char *label, int *arr, int len) {
    printf("%s: [", label);
    for (int i = 0; i < len; i++) {
        printf("%d", arr[i]);
        if (i < len - 1) printf(", ");
    }
    printf("]\n");
}

int main() {
    int mode;
    printf("Select mode:\n");
    printf("1 = Decimal to Factoradic\n");
    printf("2 = Factoradic to Decimal\n");
    printf("3 = Factoradic to Permutation\n");
    printf("4 = Permutation to Factoradic\n");
    printf("Choice: ");
    scanf("%d", &mode);
    getchar(); // Consume newline

    if (mode == 1) {
        mpz_t number;
        mpz_init(number);
        printf("Enter decimal number: ");
        gmp_scanf("%Zd", number);
        int *digits, len;
        dec_to_factoradic(number, &digits, &len);
        print_array("Factoradic", digits, len);
        free(digits);
        mpz_clear(number);

    } else if (mode == 2) {
        int *digits;
        int len = read_digits_input(&digits);

        if (len > 0) {
            mpz_t result;
            mpz_init(result);
            factoradic_to_dec(result, digits, len);
            gmp_printf("Decimal: %Zd\n", result);
            mpz_clear(result);
            free(digits);
        }

    } else if (mode == 3) {
        int *digits;
        int len = read_digits_input(&digits);
        int *perm = malloc(len * sizeof(int));
        factoradic_to_permutation(digits, len, perm);
        print_array("Permutation", perm, len);
        free(digits);
        free(perm);

    } else if (mode == 4) {
        int *perm;
        int len = read_digits_input(&perm);
        int *factoradic = malloc(len * sizeof(int));
        permutation_to_factoradic(perm, len, factoradic);
        print_array("Factoradic", factoradic, len);
        free(perm);
        free(factoradic);

    } else {
        printf("Invalid choice.\n");
    }

    return 0;
}

