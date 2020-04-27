#include <signal.h> /* constantes como SIGINT*/
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// Esta variable es usada por el hijo para contar cuantas
// veces recibio SIGURG.
int count = 0;

void sigurg_handler(int sig){
	// Cada vez que recibe SIGURG imprime "ya va!"
	// y cuenta.
	write(1, "ya va!\n", 7);
	count++;
}

void Subrutina_proceso_hijo(char const *argv[]){
	// Redefine el handler de SIGURG
	signal(SIGURG, sigurg_handler);
	// Hasta que la cuenta no alcance 5, queda atrapado
	// en este while
	while (count < 5){

	};
	// Luego, consigue el id del padre y le manda SIGINT
	pid_t ppid = getppid();
	kill(ppid, SIGINT);
	// Ejecuta el argumento que habia recibido el padre
	// y termina.
	execvp(argv[1], (char*const*)(argv+1));
	exit(EXIT_SUCCESS);
}

void sigint_handler(int sig){
	// Una vez que el padre recive SIGINT espera a que
	// termine el hijo y luego termina.
	wait(NULL);
	exit(EXIT_SUCCESS);
}

void Subrutina_proceso_padre(pid_t pid){
	// Redefine el handler de SIGINT
	signal(SIGINT, sigint_handler);
	// Espera 1 segundo, escribe "sup!" y manda SIGURG
	// al hijo ad infinitum.
	while (1) {
		sleep(1);
		write(1, "sup!\n", 5);
		kill(pid, SIGURG);
	}
}

int main(int argc, char const *argv[]){
  pid_t pid = fork();
  // si no hay error, pid vale 0 para el hijo
  // y el valor del process id del hijo para el padre
  if (pid == -1) exit(EXIT_FAILURE);  
  // si es -1, hubo un error
  else if (pid == 0) {
     Subrutina_proceso_hijo(argv);
  }
  else {
     Subrutina_proceso_padre(pid);
  }
}