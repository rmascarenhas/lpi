/* lib1.c - Example library to demonstrate library unloading on main.c
 *
 * The `main.c` file in this directory is linked against `libx1.c` and
 * `libx2.c`. This library is also linked against `lib2.c`. See the `main.c`
 * file for more information.
 *
 * Compilation
 *
 *    $ gcc -fPIC -Wall -Wextra -shared -o libx1.so libx1.c libx2.so
 *
 * Author: Renato Mascarenhas Costa
 */

#include <stdio.h>

void
__attribute__ ((constructor)) libx1_loaded(void) {
	printf("Libx1: loaded\n");
}

void
__attribute__ ((destructor)) libx1_unloaded(void) {
	printf("Libx1: unloaded\n");
}

void
libx1_f1(void) {
	printf("CALL libx1_f1\n");
	libx2_f1();
}
