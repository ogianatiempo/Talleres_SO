#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wait.h>

#define NELEMS(a) (sizeof(a) / sizeof((a)[0]))

static int run(const char ***progs, size_t count){

	//TODO: Guardar el PID de cada proceso hijo creado en children[i]
	pid_t children[count];
	int pipes[count-1][2];
	int i, j, status;

	// Crear pipes:
	for (i = 0; i < count-1; i++) {
		if (pipe(pipes[i]) == -1) {
			perror("pipe");
			exit(1);
		}
	}

	// Para cada proceso hijo:
	for (i = 0; i < count; i++) {
		children[i] = fork();
		if (children[i] == -1) exit(EXIT_FAILURE);
		if (children[i] == 0) {
			//1. Redireccionar los file descriptors adecuados al proceso
			if (i == 0) {
				// Primer proceso: solo pipeo stdout
				dup2(pipes[0][1], 1);
				for (j = 0; j < count-1; j++) {
					close(pipes[j][0]);
					close(pipes[j][1]);
				}
			} else if (i == count-1) {
				// Ultimo proceso: solo pipeo stdin
				dup2(pipes[i-1][0], 0);
				for (j = 0; j < count-1; j++) {
					close(pipes[j][0]);
					close(pipes[j][1]);
				}
			} else {
				// Resto: pipeo stdin y stdout
				dup2(pipes[i-1][0], 0);
				dup2(pipes[i][1], 1);
				for (j = 0; j < count-1; j++) {
					close(pipes[j][0]);
					close(pipes[j][1]);
				}
			}
			//2. Ejecutar el programa correspondiente
			execvp(progs[i][0], (char * const*) progs[i]);
			exit(EXIT_FAILURE);
		} 
	}
	
	// Cierro pipes en el padre
	for (i = 0; i < count-1; i++) {
		close(pipes[i][0]);
		close(pipes[i][1]);
	}

	//El padre espera a que terminen todos los procesos hijos que ejecutan los programas
	for (i = 0; i < count; i++) {
		if (waitpid(children[i], &status, 0) == -1) {
			perror("waitpid");
			return -1;
		}
		if (!WIFEXITED(status)) {
			fprintf(stderr, "proceso %d no terminó correctamente [%d]: ",
			    (int)children[i], WIFSIGNALED(status));
			perror("");
			return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv){
	const char *lscmd[] = { "ls", "-al", NULL };
	const char *wccmd[] = { "wc", NULL };
	const char *awkcmd[] = { "awk", "{ print $2 }", NULL };
	const char **progs[] = { lscmd, wccmd, awkcmd };

	printf("status: %d\n", run(progs, NELEMS(progs)));
	fflush(stdout);
	fflush(stderr);

	return 0;
}
