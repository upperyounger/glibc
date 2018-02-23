/* Default cancellation syscall bridge.
   Copyright (C) 2017 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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

#include <sysdep.h>
#include <pthreadP.h>

#define ADD_LABEL(__label)		\
  asm volatile (			\
    ".global " __label "\t\n"		\
    ".type " __label ",\%function\t\n" 	\
    __label ":\n");

/* This is the generic version of the cancellable syscall code which
   adds the label guards (__syscall_cancel_arch_{start,end}) used
   on SIGCANCEL handler (sigcancel_handler at nptl-init.c) to check if
   the cancelled syscall have side-effects that need to be signaled to
   program.

   An important contrainst should be observed when using this generic
   implementation: the __syscall_cancel_arch_end should point to the
   immediate next instruction after the syscall one.  This is because
   kernel will signal interrupted syscall with side effects by setting
   the signal frame Program Counter right after the syscall instruction.

   If the INTERNAL_SYSCALL_NCS macro use more instructions to get the
   error condition from kernel (as for powerpc and sparc), uses an
   out of the line helper (as for ARM thumb), or uses a kernel helper
   gate (as for i686 or ia64) the architecture should either adjust the
   macro or provide a custom __syscall_cancel_arch implementation.   */
long int
__syscall_cancel_arch (volatile int *ch, __syscall_arg_t nr,
		       __syscall_arg_t a1, __syscall_arg_t a2,
		       __syscall_arg_t a3, __syscall_arg_t a4,
		       __syscall_arg_t a5, __syscall_arg_t a6
		       __SYSCALL_CANCEL7_ARG_DEF)
{
  ADD_LABEL ("__syscall_cancel_arch_start");
  if (__glibc_unlikely (*ch & CANCELED_BITMASK))
    __syscall_do_cancel();

  INTERNAL_SYSCALL_DECL(err);
  long int result = INTERNAL_SYSCALL_NCS_CALL (nr, err, a1, a2, a3, a4, a5,
					       a6 __SYSCALL_CANCEL7_ARG7);
  ADD_LABEL ("__syscall_cancel_arch_end");
  if (INTERNAL_SYSCALL_ERROR_P (result, err))
    return -INTERNAL_SYSCALL_ERRNO (result, err);
  return result;
}
libc_hidden_def (__syscall_cancel_arch)
