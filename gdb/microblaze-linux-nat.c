/* Microblaze GNU/Linux native support.

   Copyright (C) 1988-1989, 1991-1992, 1994, 1996, 2000-2012 Free
   Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "gdb_string.h"
#include "observer.h"
#include "frame.h"
#include "inferior.h"
#include "gdbthread.h"
#include "gdbcore.h"
#include "regcache.h"
#include "gdb_assert.h"
#include "target.h"
#include "linux-nat.h"

#include <stdint.h>
#include <sys/types.h>
#include <sys/param.h>
#include <signal.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include "gdb_wait.h"
#include <fcntl.h>
#include <sys/procfs.h>
#include <sys/ptrace.h>

/* Prototypes for supply_gregset etc.  */
#include "gregset.h"
#include "microblaze-tdep.h"

/* Non-zero if our kernel may support the PTRACE_GETREGS and
   PTRACE_SETREGS requests, for reading and writing the
   general-purpose registers.  Zero if we've tried one of
   them and gotten an error.  */
int have_ptrace_getsetregs = 1;

const struct microblaze_gregset microblaze_linux_core_gregset;

void
supply_gregset (struct regcache *regcache, const gdb_gregset_t *gregsetp)
{
  microblaze_supply_gregset (&microblaze_linux_core_gregset, regcache,
                             -1, gregsetp);
}

void
fill_gregset (const struct regcache *regcache,
	      gdb_gregset_t *gregsetp, int regno)
{
  if (regno == -1)
    memset (gregsetp, 0, sizeof (*gregsetp));
  microblaze_collect_gregset (&microblaze_linux_core_gregset, regcache,
                              regno, gregsetp);
}

void
supply_fpregset (struct regcache *regcache, const gdb_fpregset_t * fpregsetp)
{
  /* FIXME. */
}

void
fill_fpregset (const struct regcache *regcache,
	       gdb_fpregset_t *fpregsetp, int regno)
{
  /* FIXME. */
}

void _initialize_microblaze_linux_nat (void);

void
_initialize_microblaze_linux_nat (void)
{
  struct target_ops *t;

  /* Fill in the generic GNU/Linux methods.  */
  t = linux_target ();

  /* Register the target.  */
  linux_nat_add_target (t);
}
