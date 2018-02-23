/* Copyright (C) 2002-2018 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <setjmp.h>
#include <stdlib.h>
#include "pthreadP.h"

/* Cancellation function called by all cancellable syscalls.  */
long int
__syscall_cancel (__syscall_arg_t nr, __syscall_arg_t a1,
		  __syscall_arg_t a2, __syscall_arg_t a3,
		  __syscall_arg_t a4, __syscall_arg_t a5,
		  __syscall_arg_t a6 __SYSCALL_CANCEL7_ARG_DEF)
{
  pthread_t self = (pthread_t) THREAD_SELF;
  volatile struct pthread *pd = (volatile struct pthread *) self;
  long int result;

  /* If cancellation is not enabled, call the syscall directly.  */
  if (pd->cancelstate == PTHREAD_CANCEL_DISABLE)
    {
      INTERNAL_SYSCALL_DECL (err);
      result = INTERNAL_SYSCALL_NCS_CALL (nr, err, a1, a2, a3, a4, a5, a6
					  __SYSCALL_CANCEL7_ARG7);
      if (INTERNAL_SYSCALL_ERROR_P (result, err))
	return -INTERNAL_SYSCALL_ERRNO (result, err);
      return result;
    }

  /* Call the arch-specific entry points that contains the globals markers
     to be checked by SIGCANCEL handler.  */
  result = __syscall_cancel_arch (&pd->cancelhandling, nr, a1, a2, a3, a4, a5,
			          a6 __SYSCALL_CANCEL7_ARG7);

  if ((result == -EINTR)
      && (atomic_load_relaxed (&pd->cancelhandling) & THREAD_CANCELED)
      && (pd->cancelstate != PTHREAD_CANCEL_DISABLE))
    __syscall_do_cancel ();

  return result;
}
libc_hidden_def (__syscall_cancel)

/* Since __do_cancel is a always inline function, this creates a symbol the
   arch-specific symbol can call to cancel the thread.  */
void
__cleanup_fct_attribute attribute_hidden __attribute ((noreturn))
__syscall_do_cancel (void)
{
  __do_cancel ();
}
