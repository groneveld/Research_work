#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <time.h>

int n = 100;

int min( int a, int b ) {
	int result = (a < b) ? a : b;
	if (a == 0 && b > 0) result = b;
	if (b == 0 && a > 0) result = a;
   	return result;
}

int get_chunk(int commsize, int rank) {
	int q = n / commsize;
	if(n % commsize)
	    q++;
	int r = commsize * q - n;
	int chunk = q;
	if (rank >= commsize - r)
	    chunk = q - 1;
	return chunk;
}

int main () {
	MPI_Init(NULL, NULL);
	int commsize, rank;
	MPI_Comm_size(MPI_COMM_WORLD, &commsize);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	srand(rank);
	int points_per_proc = n / commsize;
	int amount_of_max = points_per_proc;
	if (n % commsize) amount_of_max++;
	amount_of_max = commsize - (commsize * amount_of_max - n);
	int nrows = get_chunk(commsize, rank);
	int max_nrows = get_chunk(commsize, 0);
	int min_nrows = get_chunk(commsize, commsize - 1);
	int difference = max_nrows - min_nrows;
	int lb, ub;
	if (nrows == max_nrows) {
	    lb = rank * nrows;
	    ub = lb + nrows - 1;
	} else {
	    lb = rank * nrows + difference * amount_of_max;
	    ub = lb + nrows - 1;
	}
	double t;
	int *matrix = malloc(sizeof(*matrix) * (n * nrows));
	for (int i = 0; i < nrows; i++) {
	    for (int j = 0; j < n; j++){
		if((lb + i) != j)
		    matrix[i * n + j] = rand() % 100;
		else
		    matrix[i * n + j] = 0;
	    }
	}
	int k_row[n];
	int index = 0;
	int recive;
	t = MPI_Wtime();
	for (int k = 0; k < n; k++)
	{
		if(k >= lb && k <= ub)
		{
			for (int i = 0; i < n; i++){
				index = (rank == 0) ? k : k % lb;
				k_row[i] = matrix[index * n + i];
			}
			index = (rank == 0) ? k : k % lb;
			MPI_Bcast(matrix + (index * n), n, MPI_INT, rank, MPI_COMM_WORLD);
			for (int i = 0; i < nrows; i++) {
			    for (int j = 0; j < n; j++) {
			        if ((lb + i) != j)
				matrix[i * n + j] = min(matrix[i * n + j], matrix [i * n + k] + matrix[index * n + j]);
			    }
			}
		}
		if (k < lb || k > ub) {
			if (k < max_nrows * amount_of_max) recive = k / max_nrows;
			else recive = (k - difference * amount_of_max) / min_nrows;
			MPI_Bcast(k_row, n, MPI_INT, recive, MPI_COMM_WORLD);
			for (int i = 0; i < nrows; i++) {
			    for (int j = 0; j < n; j++) {
				if ((lb + i) != j)
			    	matrix[i * n + j] = min(matrix[i * n + j], matrix [i * n + k] + k_row[j]);
			    }
			}
		}
	}
	t = MPI_Wtime() - t;
	if (rank == 0) printf("time: %f, comm: %d\n", t, commsize);
	MPI_Finalize();
	return 0;
}