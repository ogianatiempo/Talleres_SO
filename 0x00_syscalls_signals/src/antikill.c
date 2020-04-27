#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/syscall.h>
#include <sys/signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char* argv[]) {
	int status;
	long int orig_rax;
	long int rax;
	long int rdi;
	long int rsi;
	long int data;
	pid_t child;

	if (argc <= 1) {
		fprintf(stderr, "Uso: %s comando [argumentos ...]\n", argv[0]);
		exit(1);
	}

	/* Fork en dos procesos */
	child = fork();
	if (child == -1) { perror("ERROR fork"); return 1; }
	if (child == 0) {
		/* Solo se ejecuta en el Hijo */
		ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		execvp(argv[1], argv+1);
		/* Si vuelve de exec() hubo un error */
		perror("ERROR child exec(...)"); exit(1);
	} else {
		/* Solo se ejecuta en el Padre */
		wait(NULL);

		// Una vez que el execve sucedio detenemos al hijo cada vez que entra o sale de una syscall
		while(1) {
			ptrace(PTRACE_SYSCALL, child, NULL, NULL);	
			
			if (wait(&status) < 0) { perror("waitpid"); break; }
			if (WIFEXITED(status)) break; /* Proceso terminado */

			rax = ptrace(PTRACE_PEEKUSER,
        	        	 child, 8 * RAX,
            	         NULL);

			if (rax == -ENOSYS) {
				// Entrando a la syscall
				orig_rax = ptrace(PTRACE_PEEKUSER,
								  child, 8 * ORIG_RAX,
								  NULL);
				
				rdi = ptrace(PTRACE_PEEKUSER,
							 child, 8 * RDI,
							 NULL);
				
				rsi = ptrace(PTRACE_PEEKUSER,
							 child, 8 * RSI,
							 NULL);

				if (orig_rax == SYS_kill & rsi == SIGKILL) {
					// El proceso hijo esta usando la syscall KILL para enviar SIGKILL
					printf("El proceso %d quiere enviar SIGKILL al proceso %ld y por eso se lo ha detenido.\n", child, rdi);
					ptrace(PTRACE_KILL, child);
					break;
				}				
			}
		}
	}
	return 0;
}
