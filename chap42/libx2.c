/* libx2.c - Example library to demonstrate library unloading on main.c
 *
 * The `main.c` file in this directory is linked against `libx1.c` and
 * `libx2.c`. See the `main.c` file for more information.
 *
 * Compilation
 *
 *    $ gcc -c -fPIC -Wall -Wextra -o libx2.o libx2.c
 *    $ gcc -Wall -Wextra -shared -o libx2.so libx2.o
 *
 * Author: Renato Mascarenhas Costa
 */

#include <stdio.h>

void
__attribute__ ((constructor)) libx2_loaded(void) {
	printf("Libx2: loaded\n");
}

void
__attribute__ ((destructor)) libx2_unloaded(void) {
	printf("Libx2: unloaded\n");
}

void
libx2_f1(void) {
	printf("CALL libx2_f1\n");
}

void
libx2_f2(void) {
	printf("CALL libx2_f2\n");
}
