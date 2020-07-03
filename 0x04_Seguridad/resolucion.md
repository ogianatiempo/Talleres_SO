# Taller de seguridad

El taller de seguridad consiste en una serie de deafíos al estilo captura de la bandera (CTF) implementados en una [máquina virtual](https://www-2.dc.uba.ar/staff/rbaader/so-labo-seginf2020.ova). Por cada desafío hay un usuario llamado `nivelXX` y otro llamado `flagXX`, donde `XX` es el número de desafío. Tenemos acceso a cada usuario `nivelXX` pero debemos explotar alguna falla de seguridad o error de configuración para lograr obtener privilegios del usuario `flagXX` correspondiente. Una vez conseguido eso debemos ejecutar como el usuario `flagXX` el binario `/bin/checkflag` que nos dará la bandera o flag de cada nivel.

## Nivel00

En este nivel tenemos que encontrar un binario que tenga seteado el bit SUID cuyo dueño sea el usuario `flag00`. Para esto vamos a usar el comando `find` que permite buscar archivos y tiene infinitas opciones para filtrar los resultados. En este caso vamos a usar: `find / -perm -u=s -user flag00 2>/dev/null`. Esto busca un archivo que tenga seteado el SUID bit y su dueño sea `flag00`. Usamos `2>/dev/null` para "tirar" lo que se imprima por `stderr`. De esta forma encontramos el binario `/bin/.../flag00`. Si lo ejecutamos nos da una nueva shell y si chequeamos que usuario somos con `id` o `whoami`, vamos aver que somos `flag00`. Esto es porque al tener seteado el SUID bit, el binario nos permite ejecutar el código como si fuésemos su dueño, que en este caso es `flag00`. Desde esta shell podemos ejecutar `/bin/checkflag` para ganar el nivel.

## Nivel 01

Tenemos un binario en `/home/flag01` con el siguiente código fuente:

```c
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>

int main(int argc, char **argv, char **envp)
{
    gid_t gid;
    uid_t uid;
    gid = getegid();
    uid = geteuid();

    setresgid(gid, gid, gid);
    setresuid(uid, uid, uid);

    system("/usr/bin/env echo y ahora qué?");
}
```

Si vemos los permisos del binario usando `ls -la` podemos ver que también tiene el SUID bit por la s en sus permisos de ejecución del dueño `-rwsr-x---`. En este caso el dueño es `flag01`, es decir que cuando lo ejecutemos nuestro id de usuario efectivo (`euid`) va a ser el de `flag01`. Mirando el código podemos ver que el programa setea el id de usuario real al valor del `euid` usando `setresuid`. Cada vez que un proceso hijo se lanza, este se ejecuta con el id de usuario efectivo igual al id usuario real (salvo que tenga el bit SUID como dijimos anteriormente) y el id efectivo es el que se usa para hacer los chequeos de seguridad. Al setear el id real al valor del efectivo permite que al llamar a `system` el proceso hijo se ejecute también como el usuario `flag01`. Pero system no ejecuta `/bin/echo` sino que ejecuta `/usr/bin/env echo`. Esto hace que busque en los directorios listados en la variable de entorno `PATH` algún ejecutable llamado `echo`. El problema es que nosotros, como `nivel01` tenemos control sobre esa variable y podemos agregar al `PATH` un directorio en el cual coloquemos un ejecutable que se llame `echo`. Si hacemos eso, este ejecutable será usado en lugar de `/bin/echo`. Por ejemplo, podemos agregar al path nuestro directorio `home` usando `export PATH=/home/nivel01:$PATH`. Podemos ver el como queda haciendo `echo $PATH`. Sólo resta crear un ejecutable en ese directorio que se llame `echo` que haga lo que queremos y hacerlo ejecutable con `chmod +x echo`. Puede ser un script de bash que nos de una shell o que directamente ejecute `/bin/checkflag`:

```bash
#!/bin/sh
/bin/checkflag
```

## Nivel 02

Nuevamente tenemos un binario en `/home/flag02` con el siguiente código fuente:

```c
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>

int main(int argc, char **argv, char **envp)
{
    char *buffer;
    gid_t gid;
    uid_t uid;

    gid = getegid();
    uid = geteuid();

    setresgid(gid, gid, gid);
    setresuid(uid, uid, uid);

    buffer = NULL;

    asprintf(&buffer, "/bin/echo %s es genial", getenv("USER"));
    printf("a punto de invocar system(\"%s\")\n", buffer);

    system(buffer);
}
```

El código usa la misma estrategia que el anterior para ejecutar system como `flag02`. En este caso, define un buffer y guarda en ese buffer un string que despues va a ser el argumento de `system`. El problema es que en este string va una variable de entorno llamada `USER` sobre la que tenemos control. Nuevamente podemos hacer `export USER=sarasa` para cambiar el valor de la variable. En este caso vamos a inyectar código cambiando la variable a `;/bin/sh;`. Esto funciona porque `;` permite separar dos comandos, entonces se va a ejecutar `echo` y luego `sh`. El segundo `;` es para que `sh` no interprete lo que sigue como un argumento. 

## Nivel 03

En este nivel hay una tarea programada de cron del usuario `flag03`. Esta tarea ejecuta `writable.sh`. Si vemos el código, ejecuta cada archivo del directorio `writable.d` pero sólo por 5 segundos. El problema es que nosotros, como `nivel03` podemos escribir en ese directorio. Entonces podemos crear un script dentro de ese directorio que ejecute `/bin/checkflag` y guarde el output en un archivo que podemos leer. Es importante cambiar los permisos de ese archivo con `chmod` para que efectivamente lo podamos leer, porque va a ser creado por `flag03`. Un posible script sería:

```bash
#!/bin/sh
/bin/checkflag > /home/flag03/res
chmod 777 /home/flag03/res
```

## Nivel 04

Nuevamente tenemos un binario en `/home/flag04`. Nuestro objetivo es usar el binario para leer el archivo `token`. No tenemos permiso para leerlo directamente porque pertenece a `flag04` pero el binario, cuyo dueño también es `flag04`, tiene el SUID bit y nos permite abrir un archivo. El problema es que el binario chequea que el nombre de dicho archivo no contenga `token`. Esto puede ser bypaseado creando un link al archivo `token` con un nombre distinto y abriendo ese link. Por ejemplo podemos hacer `ln -s /home/flag04/token /tmp/gato` y luego `./flag04 /tmp/gato`.