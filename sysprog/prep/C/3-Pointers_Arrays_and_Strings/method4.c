#include <stdio.h>
#include <stdlib.h>

int main(void) 
{

    int **rptr;
    int *aptr;
    int *testptr;
    int k;
    int nrows = 5;  /* Both nrows and ncols could be evaluated */
    int ncols = 10; /* or read in at run time */
    int row, col;
    
    /* we now allocate the memory for the array */
    aptr = malloc(nrows * ncols * sizeof(int));
    if (aptr == NULL) {
        puts("Failure to allocate room for the array");
        exit(0);
    }
    
    /* next we allocate room for the pointers to the rows */
    rptr = malloc(nrows * sizeof(int *));
    if (rptr == NULL) {
        puts("Failure to allocate room for pointers");
        exit(0);
    }
    
    /* and now we 'point' the pointers */
    for (k = 0; k < nrows; k++) {
        rptr[k] = aptr + (k * ncols);
    }

    /* Now we illustrate how the row pointers are incremented */
    printf("Illustrating how row pointers are incremented\n");
    printf("\nIndex Pointer(hex) Diff.(dec)");
    for (row = 0; row < nrows; row++) {
        printf("\n%5d %11p", row, rptr[row]);
        if (row > 0)
            printf(" %9d", (int) (rptr[row] - rptr[row-1]));
    }
    printf("\n\nAnd now we print out the array\n");
    for (row = 0; row < nrows; row++) {
        for (col = 0; col < ncols; col++) {
            rptr[row][col] = row + col;
            printf("%2d ", rptr[row][col]);
        }
        putchar('\n');
    }
    puts("\n");
    
    /* and here we illustrate that we are, in fact, dealing with 
    a 2 dimensional array in a contiguous block of memory. */
    printf("And now we demonstrate that they are contiguous in memory\n");
    testptr = aptr;
    for (row = 0; row < nrows; row++) {
        for (col = 0; col < ncols; col++) {
            printf("%2d ", *(testptr++));
        }
        putchar('\n');
    }
    
    /* finally, we deallocate the memory */
    free(rptr);
    free(aptr);
    
    return 0;
}
