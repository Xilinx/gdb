/* GNU/Linux/PowerPC specific low level interface, for the remote server for
   GDB.
   Copyright (C) 1995, 1996, 1998, 1999, 2000, 2001, 2002, 2005
   Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#include "server.h"
#include "linux-low.h"

#include <asm/ptrace.h>

#include "gdb_proc_service.h"

static int microblaze_regmap[] =
 {       -1,     PT_GPR(1),     PT_GPR(2),     PT_GPR(3),
  PT_GPR(4),     PT_GPR(5),     PT_GPR(6),     PT_GPR(7),
  PT_GPR(8),     PT_GPR(9),     PT_GPR(10),    PT_GPR(11),
  PT_GPR(12),    PT_GPR(13),    PT_GPR(14),    PT_GPR(15),
  PT_GPR(16),    PT_GPR(17),    PT_GPR(18),    PT_GPR(19),
  PT_GPR(20),    PT_GPR(21),    PT_GPR(22),    PT_GPR(23),
  PT_GPR(24),    PT_GPR(25),    PT_GPR(26),    PT_GPR(27),
  PT_GPR(28),    PT_GPR(29),    PT_GPR(30),    PT_GPR(31),
  PT_PC,         PT_MSR,        PT_EAR,        PT_ESR,
  PT_FSR
  };

#define microblaze_num_regs (sizeof microblaze_regmap / sizeof microblaze_regmap[0])

/* Defined in auto-generated file reg-microblaze.c.  */
void init_registers_microblaze (void);

static int
microblaze_cannot_store_register (int regno)
{
  if (microblaze_regmap[regno] == -1)
    return 1;

  return 0;
}

static int
microblaze_cannot_fetch_register (int regno)
{
  if (find_regno ("r0") == regno)
    return 1;
  return 0;
}

static CORE_ADDR
microblaze_get_pc (struct regcache *regcache)
{
  unsigned long pc;

  collect_register_by_name (regcache, "pc", &pc);
  return (CORE_ADDR) pc;
}

static void
microblaze_set_pc (struct regcache *regcache, CORE_ADDR pc)
{
  unsigned long newpc = pc;

  supply_register_by_name (regcache, "pc", &newpc);
}

/* dbtrap insn */
/* brki r16, 0x18; */
static const unsigned long microblaze_breakpoint = 0xba0c0018;
#define microblaze_breakpoint_len 4

static int
microblaze_breakpoint_at (CORE_ADDR where)
{
  unsigned long insn;

  (*the_target->read_memory) (where, (unsigned char *) &insn, 4);
  if (insn == microblaze_breakpoint)
    return 1;
  /* If necessary, recognize more trap instructions here.  GDB only uses the
     one.  */
  return 0;
}

/* Provide only a fill function for the general register set.  ps_lgetregs
   will use this for NPTL support.  */

static void microblaze_fill_gregset (struct regcache *regcache, void *buf)
{
  int i;

  for (i = 0; i < 32; i++)
    collect_register (regcache, i, (char *) buf + microblaze_regmap[i]);
}

static CORE_ADDR
microblaze_reinsert_addr (struct regcache *regcache)
{
  unsigned long pc;
  collect_register_by_name (regcache, "r15", &pc);
  return pc;
}

struct linux_target_ops the_low_target = {
  init_registers_microblaze,
  microblaze_num_regs,
  microblaze_regmap,
  NULL,
  microblaze_cannot_fetch_register,
  microblaze_cannot_store_register,
  NULL, /* fetch_register */
  microblaze_get_pc,
  microblaze_set_pc,
  (const unsigned char *) &microblaze_breakpoint,
  microblaze_breakpoint_len,
  microblaze_reinsert_addr,
  0,
  microblaze_breakpoint_at,
};
