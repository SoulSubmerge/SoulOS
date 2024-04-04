#include <iostream>

void printMatrix(int** matrix, int& m, int& n)
{
    std::cout << "* * * Initial output matrix * * * \n" << std::endl;
    for(int i = 0; i < m;i++)
    {
        for(int j = 0; j < n; j++)
        {
            std::cout << matrix[i][j] << "\t";;
        }
        std::cout << std::endl;
    }
    std::cout << "* * * Matrix print complete * * *\n" << std::endl;
}

int **transponse(int** matrix, int matrixSize, int* matrixColSize, int* returnSize, int** returnColumnSizes)
{
    int m = matrixSize, n = matrixColSize[0];
    int** transposed = (int**)malloc(sizeof(int*) * n);
    *returnSize = n;
    *returnColumnSizes = (int*)malloc(sizeof(int) * n);
    for(int i = 0; i<n ;i++)
    {
        transposed[i] = (int*)malloc(sizeof(int) * m);
        (*returnColumnSizes)[i] = m;
    }
    for(int i=0;i<m;i++)
    {
        for(int j=0;j<n;j++)
        {
            transposed[j][i] = matrix[i][j];
        }
    }
    return transposed;
}


int main()
{
    int m = 5, n = 6;
    int** matrix = (int**)malloc(sizeof(int*) * m);
    for(int i=0;i<m;i++)
    {
        *(matrix + i) = (int*)malloc(sizeof(int*) * n);
    }
    int num = 1;
    for(int i = 0; i < m;i++)
    {
        for(int j = 0; j < n; j++)
        {
            matrix[i][j] = (num++);
        }
    }
    printMatrix(matrix,m,n);
    int returnSize = 0;
    int **returnColumnSizes = (int**)malloc(sizeof(int) * n);
    int **transponsed = transponse(matrix, m, &n, &returnSize, returnColumnSizes);
    printMatrix(transponsed, returnSize, (*returnColumnSizes)[0]);
    return 0;
}

