#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

int logicalExp(int a, int b, int c){
	return (a && b) || (a && !c);
}

int printLogicalExp(int a, int b, int c){
	int result = logicalExp(a, b, c);
	printf("a = %d, b = %d, c = %d: %d\n", a, b, c, result);
	return result;
}


int main(){
	pid_t a = 0, b = 0, c = 0;
	a = fork();
	b = fork();
	c = fork();
	printLogicalExp(a, b, c);
	wait(NULL);
	return 0;
}