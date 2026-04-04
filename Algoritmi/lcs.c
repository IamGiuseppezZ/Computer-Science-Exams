#include <stdio.h>
#include <string.h>

int max(int a, int b) {
    return (a > b) ? a : b;
}

// Returns length of LCS for X[0..m-1], Y[0..n-1]
int lcs(char* X, char* Y, int m, int n) {
    int L[m + 1][n + 1]; // DP Table

    // Build the L[m+1][n+1] table in bottom-up fashion
    for (int i = 0; i <= m; i++) {
        for (int j = 0; j <= n; j++) {
            if (i == 0 || j == 0) {
                L[i][j] = 0; // Base case: empty strings
            }
            else if (X[i - 1] == Y[j - 1]) {
                L[i][j] = L[i - 1][j - 1] + 1; // Match found, add 1 to diagonal
            }
            else {
                L[i][j] = max(L[i - 1][j], L[i][j - 1]); // No match, take max of left or top
            }
        }
    }
    
    return L[m][n]; // Bottom-right cell holds the final answer
}

int main() {
    char X[] = "AGGTAB";
    char Y[] = "GXTXAYB";
    
    int m = strlen(X);
    int n = strlen(Y);
    
    printf("Length of LCS is %d\n", lcs(X, Y, m, n));
    return 0;
}