#include <stdio.h>

static int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

static long long fibonacci(int n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    long long a = 0, b = 1;
    for (int i = 2; i <= n; i++) {
        long long tmp = a + b;
        a = b;
        b = tmp;
    }
    return b;
}

static double power(double base, int exp) {
    double result = 1.0;
    for (int i = 0; i < exp; i++)
        result *= base;
    return result;
}

int main(void) {
    printf("Hello from SlopReveal example!\n\n");

    printf("=== Factorials ===\n");
    for (int i = 1; i <= 10; i++)
        printf("  %2d! = %d\n", i, factorial(i));

    printf("\n=== Fibonacci sequence (first 15 terms) ===\n");
    for (int i = 1; i <= 15; i++)
        printf("  F(%2d) = %lld\n", i, fibonacci(i));

    printf("\n=== Powers of 2 ===\n");
    for (int i = 0; i <= 12; i++)
        printf("  2^%2d = %.0f\n", i, power(2.0, i));

    printf("\n=== Arithmetic ===\n");
    int a = 42, b = 17;
    printf("  %d + %d = %d\n", a, b, a + b);
    printf("  %d - %d = %d\n", a, b, a - b);
    printf("  %d * %d = %d\n", a, b, a * b);
    printf("  %d / %d = %d (remainder %d)\n", a, b, a / b, a % b);

    printf("\nDone.\n");
    return 0;
}
