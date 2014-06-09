/* longjmp_returned_func.c - Performs a `longjmp(2)` to a function that has already returned.
 *
 * On UNIX systems, longjmp(2) can be used with setjmp(2) to perform a nonlocal goto, that is,
 * jump to a point in the code that is not in the same function as the jump call. That's
 * where it differs from standard `goto` calls, performed within the same funcion frame.
 *
 * This program demonstrate that chaos that can take place when you perform a longjmp
 * to a function that has already returned. As longjmp unwinds the stack back to
 * the function that set the jump destination, jumping to a function that already
 * returned tries to find a frame in the stack that doesn't exist anymore, leading
 * to infinite loops, program crashing and others - the behavior is undefined.
 *
 * Run this program with:
 *
 *    $ ./lonjmp_returned_func
 *    # will print its steps towards undefined behavior
 *
 *
 * In practice, this program could actually go back to the returned function
 * as if nothing happened. However, tell the compiler to optimize the code and chaos
 * will appear.
 *
 * Experiments run on Linux 3.11.0-14-generic.
 *
 * Author: Renato Mascarenhas Costa
 */

#include <unistd.h>
#include <setjmp.h>

#include <stdio.h>
#include <stdlib.h>

#ifndef LRF_JMP_SIGNAL
#define LRF_JMP_SIGNAL (1)
#endif

static jmp_buf env; /* declared global since it must be shared across functions */

void installSetJmp();
void tryJump();
void placeholder();

int
main() {
  printf("Program started, starting test.\n");
  installSetJmp();

  printf("Function has returned, now trying to jump back to it.\n");
  tryJump();

  placeholder();
  placeholder();
  tryJump();

  printf("Tried to perform jump, now back to main function. You might not always see this. Finishing.\n");
  return EXIT_SUCCESS;
}

void
installSetJmp() {
  switch (setjmp(env)) {
  case 0:
    printf("`setjmp` called, returning from function.\n");
    break;

  case LRF_JMP_SIGNAL:
    printf("Here we are again, as if the function didn't finish.\n");
    break;
  }

  printf("Continuing function\n");
}

void
tryJump() {
  longjmp(env, LRF_JMP_SIGNAL);
}

void
placeholder() {
  printf("Calling placeholder function, to mess up the stack\n");
}
