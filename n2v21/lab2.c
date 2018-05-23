#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define Z 3

typedef struct {
	pthread_barrier_t* syncLayer;
	int ij[2];
} ThreadRecord;

double dt = 1, dx = 1, dy = 1, dz = 1;
double a = 1;
double*** matrix;
int N = 0, M = 0, T = 0;

double force_impact(double x, double y, double t) {
  if ((int)(t * 5 / T) % 2)
    return 1;
  return -1;
	//return 1;
}

double next_time_x(double z20, double z10, double z00) {
	return (z20 - 2*z10 + z00) / dx;
}

double next_time_y(double z02, double z01, double z00) {
	return (z02 - 2*z01 + z00) / dy;
}

int next_time_t(int i, int j, int t) {
	matrix[i][j][(t+1) % Z] = a*a*dt*((next_time_x(matrix[i+1][j][t % Z], matrix[i][j][t % Z], matrix[i-1][j][t % Z]) + 
									   next_time_y(matrix[i][j+1][t % Z], matrix[i][j][t % Z], matrix[i][j-1][t % Z])) + 
									   force_impact(i, j, t)) + 2*matrix[i][j][t % Z] - matrix[i][j][(t-1)%Z];
	return 0;
}

void* next_time_t_thread(void* arg) {
	/*perror("Thread created");*/
	ThreadRecord* data = (ThreadRecord*) arg;
	for (int k = 1; k < T; ++k){
		int ij = data->ij[0];
		int ij_end = data->ij[1];
		/*fprintf(stderr, "ij: %d, ij_end: %d\n", ij, ij_end);*/
		for (int i = ij; i < ij_end; i++){
			if (i % M == 0 || (i + 1) % M == 0 || i / M == 0 || (i + 1) / M == 0 || i / M == N - 1)
				continue;
			/*fprintf(stderr, "i = %d, j = %d\n", i / M, i % M);*/
			next_time_t(i / M, i % M, k);
		}
		
		pthread_barrier_wait(data->syncLayer);
		/*perror("Sync");*/
		pthread_barrier_wait(data->syncLayer);
	}
	return NULL;
}

void init() {
	printf("Enter N, M, T\n");
	scanf("%d%d%d", &N, &M, &T);

	matrix = (double***)calloc(N, sizeof(double));

	for (int i = 0; i < N; ++i) {
		*(matrix + i) = (double**)calloc(M, sizeof(double));
		for (int k = 0; k < M; ++k) {
			*(*(matrix+i) + k) = (double*)calloc(Z, sizeof(double));
		}
	}

	printf("Enter a, dx, dy, dz, dt \n");
	scanf("%lf%lf%lf%lf%lf", &a, &dx, &dy, &dz, &dt);
	return;
}

int main(int argc, char const *argv[]) {
	
    // Initialize N, M, T, a, dx, dy, dz, dt and allocate memory for matrix.
    init();

    int threadCount;
    pthread_t* threads = NULL;
    ThreadRecord* args = NULL;
    pthread_barrier_t syncLayer;
    pthread_attr_t pattr;

    // Initialize attributes
    pthread_attr_init(&pattr);
    pthread_attr_setscope(&pattr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_JOINABLE);

    // How much threads will be?
    printf("How much threads you want?\n");
    scanf("%d", &threadCount);

    int elemOnThread = N * M / threadCount;

    if (elemOnThread * threadCount != N * M) {
	    fprintf(stderr, "Thread count must be a multiple of N*M! Exit...\n");
	    exit(2);
    }
    
    pthread_barrier_init(&syncLayer, NULL, threadCount+1);

    args = (ThreadRecord*) calloc(threadCount, sizeof(ThreadRecord));
    threads = (pthread_t*) calloc(threadCount, sizeof(pthread_t));

    for (int c = 0; c < threadCount; c++) {
	    args[c].syncLayer = &syncLayer;
	    args[c].ij[0] = elemOnThread * c + 1;
	    args[c].ij[1] = elemOnThread * (c + 1);
    }

    //perror("Arguments are initialized");
    for (int c = 0; c < threadCount; c++) {
	    if (pthread_create(&(threads[c]), &pattr, next_time_t_thread, (void *) &(args[c])))
		perror("pthread_create");
    }
	
    FILE * gnuplotPipe = popen("gnuplot -persistent", "w");
    fprintf(gnuplotPipe, "set terminal gif animate delay 50\n");
    fprintf(gnuplotPipe, "set output 'animate3.gif'\n");
    
    fprintf(gnuplotPipe, "set ticslevel 0\n");
    //fprintf(gnuplotPipe, "set dgrid3d 40, 40 splines\n");
    fprintf(gnuplotPipe, "set xrange[0:%d]\n", N-1);
    fprintf(gnuplotPipe, "set yrange[0:%d]\n", M-1);
    fprintf(gnuplotPipe, "set zrange[-12:0]\n");
    
    fprintf(gnuplotPipe, "set dgrid3d\n");
    fprintf(gnuplotPipe, "set hidden3d\n");
    
    fprintf(gnuplotPipe, "do for [i=1:%d] {\n", T);
    fprintf(gnuplotPipe, "splot '-' with lines\n");
    fprintf(gnuplotPipe, "}\n");

    for (int k = 1; k < T; ++k) {
	pthread_barrier_wait(&syncLayer);
	for (int i = 0; i < N; ++i) {
	    for (int j = 0; j < M; ++j) {
	      //fprintf(stderr, "%lf %lf %lf\n", (double)i, (double)j, matrix[i][j][(T+1)%3]);
	      fprintf(gnuplotPipe, "%lf %lf %lf\n", (double)i, (double)j, matrix[i][j][(k+1)%3]);
	    }
	}
	fprintf(gnuplotPipe, "e\n");
	pthread_barrier_wait(&syncLayer);
    }
    pclose(gnuplotPipe);

    for (int c = 0; c < threadCount; c++) {
    pthread_join(threads[c], NULL);
    }
    //pthread_barrier_destroy(&syncLayer);

    /*for (int i = 0; i < N; ++i) {
	for (int j = 0; j < M; ++j) {
	    printf("%lf, %lf, %.2lf \n",(double)i, (double)j, matrix[i][j][(T+1)%3]);
	}
	printf("\n");
    }*/

    return 0;
}
