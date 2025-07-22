#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <gmp.h>
#include <getopt.h>
#include <unistd.h>

#define MAX_DIGITS 256

void factorial(mpz_t result, unsigned int n) {
    mpz_set_ui(result, 1);
    for (unsigned int i = 2; i <= n; i++) {
        mpz_mul_ui(result, result, i);
    }
}

void read_raw_binary(FILE *fp, mpz_t result) {
    unsigned char buf[MAX_DIGITS];
    size_t len = fread(buf, 1, MAX_DIGITS, fp);
    if (len == 0) {
        fprintf(stderr, "Error: Failed to read binary input.\n");
        exit(EXIT_FAILURE);
    }
    mpz_import(result, len, 1, 1, 1, 0, buf);
}

void write_raw_binary(FILE *fp, mpz_t number) {
    size_t count;
    void *data = mpz_export(NULL, &count, 1, 1, 1, 0, number);
    fwrite(data, 1, count, fp);
    free(data);
}

void number_to_factoradic(mpz_t number, int **digits_ptr, int *length) {
    mpz_t fact, quotient, remainder;
    mpz_inits(fact, quotient, remainder, NULL);

    unsigned int i = 0;
    factorial(fact, i);
    while (mpz_cmp(fact, number) <= 0) {
        i++;
        factorial(fact, i);
    }
    i--;

    if (i + 1 > MAX_DIGITS) {
        fprintf(stderr, "Error: Factoradic digit count exceeds maximum of %d\n", MAX_DIGITS);
        exit(EXIT_FAILURE);
    }

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

void write_array(FILE *fp, const char *label, int *arr, int len) {
    if (label) fprintf(fp, "%s: [", label);
    for (int i = 0; i < len; i++) {
        fprintf(fp, "%d", arr[i]);
        if (i < len - 1) fprintf(fp, ", ");
    }
    fprintf(fp, "]\n");
}

int read_digits_input(FILE *fp, int **digits_ptr) {
    char buffer[8192];
    if (!fgets(buffer, sizeof(buffer), fp)) return -1;

    int *digits = malloc(MAX_DIGITS * sizeof(int));
    int count = 0;
    char *token = strtok(buffer, ",\n");
    while (token) {
        if (count >= MAX_DIGITS) {
            fprintf(stderr, "Error: Too many digits in input. Maximum is %d.\n", MAX_DIGITS);
            free(digits);
            return -1;
        }
        digits[count++] = atoi(token);
        token = strtok(NULL, ",\n");
    }
    *digits_ptr = digits;
    return count;
}

int main(int argc, char *argv[]) {
    int mode = 0, fixed = 0;
    char *infile = NULL, *outfile = NULL, *informat = "dec", *outformat = "dec";

    int opt;
    while ((opt = getopt(argc, argv, "m:i:o:f:s:S:")) != -1) {
        switch (opt) {
            case 'm': mode = atoi(optarg); break;
            case 'i': infile = optarg; break;
            case 'o': outfile = optarg; break;
            case 'f': fixed = atoi(optarg); break;
            case 's': informat = optarg; break;
            case 'S': outformat = optarg; break;
            default:
                fprintf(stderr, "Usage: -m <mode> -i <input> -o <output> -f <fixed_length> -s <input_format> -S <output_format>\n");
                return 1;
        }
    }

    FILE *fin = infile ? fopen(infile, strcmp(informat, "raw") == 0 ? "rb" : "r") : stdin;
    FILE *fout = outfile ? fopen(outfile, strcmp(outformat, "raw") == 0 ? "wb" : "w") : stdout;
    if (!fin || !fout) {
        perror("File error");
        return 1;
    }

    mpz_t n;
    int *digits = NULL, len;
    int is_raw_in = strcmp(informat, "raw") == 0;
    int is_raw_out = strcmp(outformat, "raw") == 0;

    switch (mode) {
        case 1:
            mpz_init(n);
            if (is_raw_in) {
                read_raw_binary(fin, n);
            } else {
                gmp_fscanf(fin, "%Zd", n);
            }
            number_to_factoradic(n, &digits, &len);
            write_array(fout, "Factoradic", digits, len);
            free(digits);
            mpz_clear(n);
            break;

        case 2:
            len = read_digits_input(fin, &digits);
            mpz_init(n);
            factoradic_to_number(n, digits, len);
            if (is_raw_out) {
                write_raw_binary(fout, n);
            } else {
                gmp_fprintf(fout, "Number: %Zd\n", n);
            }
            free(digits);
            mpz_clear(n);
            break;

        default:
            fprintf(stderr, "Error: Only modes 1 and 2 support raw input/output for now.\n");
            break;
    }

    if (fin != stdin) fclose(fin);
    if (fout != stdout) fclose(fout);
    return 0;
}

