#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
//#define MAKEGNU
#define _TIME
#include <sys/time.h>
#include <mpi.h>
#define _REENTRANT
#define AT 0.15
#define HT 1
#define HX 1
#define HY 1
#define LEFT_CONSTRAINT 100
#define RIGTH_CONSTRAINT 100
#define TOP_CONSTRAINT 0
#define BOTTOM_CONSTRAINT 50

struct timeval tv1,tv2,dtv;
struct timezone tz;

void time_start() {
	gettimeofday(&tv1, &tz);
}

clock_t time_stop() {
	gettimeofday(&tv2, &tz);
	dtv.tv_sec = tv2.tv_sec -tv1.tv_sec;
	dtv.tv_usec =tv2.tv_usec-tv1.tv_usec;
	if (dtv.tv_usec<0) { 
		dtv.tv_sec--; 
		dtv.tv_usec+=1000000; 
	}
	return dtv.tv_sec*1000+dtv.tv_usec/1000;
}

void init_matrix(double *A0, double *A1, int n, int m) {
	int i, j;
	for(i = 0; i < n; i++) {
	A1[i] = A0[i] = BOTTOM_CONSTRAINT;
	A1[(n*m - n) + i] = A0[(n*m - n) + i] = TOP_CONSTRAINT;
	}
	for(i = 0, j = 0; i < m; i++) {
		A1[j] = A0[j] = LEFT_CONSTRAINT;
		A1[(j+n) - 1] = A0[(j+n) - 1] = RIGTH_CONSTRAINT;
		j += n;
	}
}

void send_line(double *a0, double *a_neighbour_up, double *a_neighbour_down, int n, int m, int total, int myrank) {
	int i, j, next, prev;
	next = myrank + 1;
	prev = myrank - 1;
	if (myrank == 0) 
		prev = 0;
	if (myrank == (total - 1)) 
		next = total - 1;
	MPI_Sendrecv((void*)&a0[n*m - n], n, MPI_DOUBLE, next, 0, (void*)a_neighbour_down, n, MPI_DOUBLE, next, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Sendrecv((void*)&a0[0], n, MPI_DOUBLE, prev, 0, (void*)a_neighbour_up, n, MPI_DOUBLE, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

void solve(double *a1, double *a0, double *a_neighbour_up, double *a_neighbour_down, int n, int m, int myrank, int total) {
	int i, j;
	for(i = 0; i < m; i++) {
		if(myrank == 0 && i == 0) 
			continue; //начинаем сразу со второй строки (в первой граничные условия)
		if((myrank == (total - 1)) && i == (m - 1)) 
			break; //не рассматриваем последнюю строку последнего потока (в ней граничные условия)
		for(j = 1; j < (n-1); j++) { //также не рассматриваем боковые границы (тоже граничные условия)
			if(i == (m - 1)) { //если нижняя строка, то нужно брать из соседа
				a1[i*n + j] = AT*HT* ((a0[i*n + (j+1)] - 2*a0[i*n + j] + a0[i*n + (j-1)])/(HX*HX) + (a_neighbour_down[j] - 2*a0[i*n + j] + a0[(i-1)*n + j])/(HY*HY)) + a0[i*n + j];
				continue;
			}
			if(i == 0) { //если верхняя строка, то нужно брать из соседа
				a1[i*n + j] = AT*HT*( (a0[i*n + (j+1)] - 2*a0[i*n + j] + a0[i*n + (j-1)])/(HX*HX) + (a0[(i+1)*n + j] - 2*a0[i*n + j] + a_neighbour_up[j])/(HY*HY)) + a0[i*n + j];
				continue;
			}
			a1[i*n + j] = AT*HT*( (a0[i*n + (j+1)] - 2*a0[i*n + j] + a0[i*n + (j1)])/(HX*HX) + (a0[(i+1)*n + j] - 2*a0[i*n + j] + a0[(i-1)*n + j])/(HY*HY)) + a0[i*n + j];
		}
	}
	if(myrank == 0)
		for(j = 0; j < n; j++)
			a1[j] = HY*BOTTOM_CONSTRAINT + a1[j + n];
}

void print_matrix(double *M, int n, int m) {
	int i, j;
	for(i = 0; i < m; i++) {
		for(j = 0; j < n; j++)
			printf("%lf ", M[i*n+j]);
		putchar('\n');
	}
}

void make_gnu(double *M, FILE *fds1, int n, int m) {
	int i, j, k;
	for(i = 0, k = m - 1; i < m; i++, k--){
		for(j = 0; j < n; j++)
			fprintf(fds1," %d %d %3lf\n", j, k, M[i*n+j]);
		fprintf(fds1,"\n");
	}
	fprintf(fds1,"\n\n");
}

int main(int argc, char **argv) {
	#ifdef _TIME
		clock_t timeWork;
	#endif
	int n, m, t, count_time;
	double *A0, *A1, *a0, *a1, *a_neighbour_up, *a_neighbour_down;
	int i, j, total, myrank;
	int intBuf[3];
	#ifdef MAKEGNU
		FILE *fds1, *fds2;
		fds1 = fopen("result.txt", "w");
		fds2 = fopen("file.gnu", "w");
	#endif
	MPI_Init(&argc, &argv);
	MPI_Comm_size (MPI_COMM_WORLD, &total);
	MPI_Comm_rank (MPI_COMM_WORLD, &myrank);
	if(!myrank) {
		n = atoi(argv[1]);
		m = atoi(argv[2]);
		t = atoi(argv[3]);
		#ifdef MAKEGNU
			fprintf(fds2, "set pm3d map \nset cbrange[0:%d]\ndo for [i=0:%d]{\nsplot\'result.txt\' index i using 1:2:3 with pm3d\npause 0.5}\npause -1", 150, t);
		#endif
		intBuf[0] = n;
		intBuf[1] = m/total;
		intBuf[2] = t;
		A0 = (double *)malloc(sizeof(double)*n*m);
		A1 = (double *)malloc(sizeof(double)*n*m);
		init_matrix(A0, A1, n, m);
		#ifdef MAKEGNU
			make_gnu(A0, fds1, atoi(argv[1]), atoi(argv[2]));
		#endif
		//print_matrix(A0, n, m); //Вывод исходной задачи
	}
	MPI_Bcast((void *)intBuf, 3, MPI_INT, 0, MPI_COMM_WORLD);
	n = intBuf[0];
	m = intBuf[1];
	t = intBuf[2];
	a0 = (double *)malloc(sizeof(double)*n*m);
	a1 = (double *)malloc(sizeof(double)*n*m);
	a_neighbour_up = (double *)malloc(sizeof(double)*n);
	a_neighbour_down = (double *)malloc(sizeof(double)*n);
	MPI_Scatter((void *)A0, n*m, MPI_DOUBLE, (void *)a0, n*m, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Scatter((void *)A1, n*m, MPI_DOUBLE, (void *)a1, n*m, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	if(!myrank) {
	#ifdef _TIME
		time_start(); //Начать вычисление времени выполнения программы
	#endif
	}
	for(count_time = 0; count_time < t; count_time++) { //цикл по времени
		send_line(a0, a_neighbour_up, a_neighbour_down, n, m, total, myrank);
		solve(a1, a0, a_neighbour_up, a_neighbour_down, n, m, myrank, total);
		MPI_Barrier(MPI_COMM_WORLD);
		#ifdef MAKEGNU
			MPI_Gather((void *)a1, n*m, MPI_DOUBLE, (void *)A1, n*m, MPI_DOUBLE, 0, MPI_COMM_WORLD);
			MPI_Scatter((void *)A1, n*m, MPI_DOUBLE, (void *)a0, n*m, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		#endif
		if(!myrank) {
			#ifdef MAKEGNU
				make_gnu(A1, fds1, atoi(argv[1]), atoi(argv[2]));
			#endif
		}
	}
	if(!myrank) {
		#ifdef _TIME
			timeWork = time_stop();
			printf("Time: %lf\n",(double)timeWork);
		#endif
	}
	MPI_Finalize();
	exit(0);
}