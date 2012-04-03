/* Support for printing Fortran values for GDB, the GNU debugger.

   Copyright (C) 1993-1996, 1998-2000, 2003, 2005-2012 Free Software
   Foundation, Inc.

   Contributed by Motorola.  Adapted from the C definitions by Farooq Butt
   (fmbutt@engage.sps.mot.com), additionally worked over by Stan Shebs.

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
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "valprint.h"
#include "language.h"
#include "f-lang.h"
#include "frame.h"
#include "gdbcore.h"
#include "command.h"
#include "block.h"

#if 0
static int there_is_a_visible_common_named (char *);
#endif

extern void _initialize_f_valprint (void);
static void info_common_command (char *, int);
static void list_all_visible_commons (const char *);
static void f77_create_arrayprint_offset_tbl (struct type *,
					      struct ui_file *);
static void f77_get_dynamic_length_of_aggregate (struct type *);

int f77_array_offset_tbl[MAX_FORTRAN_DIMS + 1][2];

/* Array which holds offsets to be applied to get a row's elements
   for a given array.  Array also holds the size of each subarray.  */

/* The following macro gives us the size of the nth dimension, Where 
   n is 1 based.  */

#define F77_DIM_SIZE(n) (f77_array_offset_tbl[n][1])

/* The following gives us the offset for row n where n is 1-based.  */

#define F77_DIM_OFFSET(n) (f77_array_offset_tbl[n][0])

int
f77_get_lowerbound (struct type *type)
{
  if (TYPE_ARRAY_LOWER_BOUND_IS_UNDEFINED (type))
    error (_("Lower bound may not be '*' in F77"));

  return TYPE_ARRAY_LOWER_BOUND_VALUE (type);
}

int
f77_get_upperbound (struct type *type)
{
  if (TYPE_ARRAY_UPPER_BOUND_IS_UNDEFINED (type))
    {
      /* We have an assumed size array on our hands.  Assume that
	 upper_bound == lower_bound so that we show at least 1 element.
	 If the user wants to see more elements, let him manually ask for 'em
	 and we'll subscript the array and show him.  */

      return f77_get_lowerbound (type);
    }

  return TYPE_ARRAY_UPPER_BOUND_VALUE (type);
}

/* Obtain F77 adjustable array dimensions.  */

static void
f77_get_dynamic_length_of_aggregate (struct type *type)
{
  int upper_bound = -1;
  int lower_bound = 1;

  /* Recursively go all the way down into a possibly multi-dimensional
     F77 array and get the bounds.  For simple arrays, this is pretty
     easy but when the bounds are dynamic, we must be very careful 
     to add up all the lengths correctly.  Not doing this right 
     will lead to horrendous-looking arrays in parameter lists.

     This function also works for strings which behave very 
     similarly to arrays.  */

  if (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_ARRAY
      || TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_STRING)
    f77_get_dynamic_length_of_aggregate (TYPE_TARGET_TYPE (type));

  /* Recursion ends here, start setting up lengths.  */
  lower_bound = f77_get_lowerbound (type);
  upper_bound = f77_get_upperbound (type);

  /* Patch in a valid length value.  */

  TYPE_LENGTH (type) =
    (upper_bound - lower_bound + 1)
    * TYPE_LENGTH (check_typedef (TYPE_TARGET_TYPE (type)));
}

/* Function that sets up the array offset,size table for the array 
   type "type".  */

static void
f77_create_arrayprint_offset_tbl (struct type *type, struct ui_file *stream)
{
  struct type *tmp_type;
  int eltlen;
  int ndimen = 1;
  int upper, lower;

  tmp_type = type;

  while ((TYPE_CODE (tmp_type) == TYPE_CODE_ARRAY))
    {
      upper = f77_get_upperbound (tmp_type);
      lower = f77_get_lowerbound (tmp_type);

      F77_DIM_SIZE (ndimen) = upper - lower + 1;

      tmp_type = TYPE_TARGET_TYPE (tmp_type);
      ndimen++;
    }

  /* Now we multiply eltlen by all the offsets, so that later we 
     can print out array elements correctly.  Up till now we 
     know an offset to apply to get the item but we also 
     have to know how much to add to get to the next item.  */

  ndimen--;
  eltlen = TYPE_LENGTH (tmp_type);
  F77_DIM_OFFSET (ndimen) = eltlen;
  while (--ndimen > 0)
    {
      eltlen *= F77_DIM_SIZE (ndimen + 1);
      F77_DIM_OFFSET (ndimen) = eltlen;
    }
}



/* Actual function which prints out F77 arrays, Valaddr == address in 
   the superior.  Address == the address in the inferior.  */

static void
f77_print_array_1 (int nss, int ndimensions, struct type *type,
		   const gdb_byte *valaddr,
		   int embedded_offset, CORE_ADDR address,
		   struct ui_file *stream, int recurse,
		   const struct value *val,
		   const struct value_print_options *options,
		   int *elts)
{
  int i;

  if (nss != ndimensions)
    {
      for (i = 0;
	   (i < F77_DIM_SIZE (nss) && (*elts) < options->print_max);
	   i++)
	{
	  fprintf_filtered (stream, "( ");
	  f77_print_array_1 (nss + 1, ndimensions, TYPE_TARGET_TYPE (type),
			     valaddr,
			     embedded_offset + i * F77_DIM_OFFSET (nss),
			     address,
			     stream, recurse, val, options, elts);
	  fprintf_filtered (stream, ") ");
	}
      if (*elts >= options->print_max && i < F77_DIM_SIZE (nss)) 
	fprintf_filtered (stream, "...");
    }
  else
    {
      for (i = 0; i < F77_DIM_SIZE (nss) && (*elts) < options->print_max;
	   i++, (*elts)++)
	{
	  val_print (TYPE_TARGET_TYPE (type),
		     valaddr,
		     embedded_offset + i * F77_DIM_OFFSET (ndimensions),
		     address, stream, recurse,
		     val, options, current_language);

	  if (i != (F77_DIM_SIZE (nss) - 1))
	    fprintf_filtered (stream, ", ");

	  if ((*elts == options->print_max - 1)
	      && (i != (F77_DIM_SIZE (nss) - 1)))
	    fprintf_filtered (stream, "...");
	}
    }
}

/* This function gets called to print an F77 array, we set up some 
   stuff and then immediately call f77_print_array_1().  */

static void
f77_print_array (struct type *type, const gdb_byte *valaddr,
		 int embedded_offset,
		 CORE_ADDR address, struct ui_file *stream,
		 int recurse,
		 const struct value *val,
		 const struct value_print_options *options)
{
  int ndimensions;
  int elts = 0;

  ndimensions = calc_f77_array_dims (type);

  if (ndimensions > MAX_FORTRAN_DIMS || ndimensions < 0)
    error (_("\
Type node corrupt! F77 arrays cannot have %d subscripts (%d Max)"),
	   ndimensions, MAX_FORTRAN_DIMS);

  /* Since F77 arrays are stored column-major, we set up an 
     offset table to get at the various row's elements.  The 
     offset table contains entries for both offset and subarray size.  */

  f77_create_arrayprint_offset_tbl (type, stream);

  f77_print_array_1 (1, ndimensions, type, valaddr, embedded_offset,
		     address, stream, recurse, val, options, &elts);
}


/* Decorations for Fortran.  */

static const struct generic_val_print_decorations f_decorations =
{
  "(",
  ",",
  ")",
  ".TRUE.",
  ".FALSE.",
  "VOID",
};

/* See val_print for a description of the various parameters of this
   function; they are identical.  */

void
f_val_print (struct type *type, const gdb_byte *valaddr, int embedded_offset,
	     CORE_ADDR address, struct ui_file *stream, int recurse,
	     const struct value *original_value,
	     const struct value_print_options *options)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned int i = 0;	/* Number of characters printed.  */
  struct type *elttype;
  LONGEST val;
  CORE_ADDR addr;
  int index;

  CHECK_TYPEDEF (type);
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_STRING:
      f77_get_dynamic_length_of_aggregate (type);
      LA_PRINT_STRING (stream, builtin_type (gdbarch)->builtin_char,
		       valaddr + embedded_offset,
		       TYPE_LENGTH (type), NULL, 0, options);
      break;

    case TYPE_CODE_ARRAY:
      if (TYPE_CODE (TYPE_TARGET_TYPE (type)) != TYPE_CODE_CHAR)
	{
	  fprintf_filtered (stream, "(");
	  f77_print_array (type, valaddr, embedded_offset,
			   address, stream, recurse, original_value, options);
	  fprintf_filtered (stream, ")");
	}
      else
	{
	  struct type *ch_type = TYPE_TARGET_TYPE (type);

	  f77_get_dynamic_length_of_aggregate (type);
	  LA_PRINT_STRING (stream, ch_type,
			   valaddr + embedded_offset,
			   TYPE_LENGTH (type) / TYPE_LENGTH (ch_type),
			   NULL, 0, options);
	}
      break;

    case TYPE_CODE_PTR:
      if (options->format && options->format != 's')
	{
	  val_print_scalar_formatted (type, valaddr, embedded_offset,
				      original_value, options, 0, stream);
	  break;
	}
      else
	{
	  addr = unpack_pointer (type, valaddr + embedded_offset);
	  elttype = check_typedef (TYPE_TARGET_TYPE (type));

	  if (TYPE_CODE (elttype) == TYPE_CODE_FUNC)
	    {
	      /* Try to print what function it points to.  */
	      print_function_pointer_address (gdbarch, addr, stream,
					      options->addressprint);
	      return;
	    }

	  if (options->addressprint && options->format != 's')
	    fputs_filtered (paddress (gdbarch, addr), stream);

	  /* For a pointer to char or unsigned char, also print the string
	     pointed to, unless pointer is null.  */
	  if (TYPE_LENGTH (elttype) == 1
	      && TYPE_CODE (elttype) == TYPE_CODE_INT
	      && (options->format == 0 || options->format == 's')
	      && addr != 0)
	    i = val_print_string (TYPE_TARGET_TYPE (type), NULL, addr, -1,
				  stream, options);
	  return;
	}
      break;

    case TYPE_CODE_INT:
      if (options->format || options->output_format)
	{
	  struct value_print_options opts = *options;

	  opts.format = (options->format ? options->format
			 : options->output_format);
	  val_print_scalar_formatted (type, valaddr, embedded_offset,
				      original_value, options, 0, stream);
	}
      else
	{
	  val_print_type_code_int (type, valaddr + embedded_offset, stream);
	  /* C and C++ has no single byte int type, char is used instead.
	     Since we don't know whether the value is really intended to
	     be used as an integer or a character, print the character
	     equivalent as well.  */
	  if (TYPE_LENGTH (type) == 1)
	    {
	      LONGEST c;

	      fputs_filtered (" ", stream);
	      c = unpack_long (type, valaddr + embedded_offset);
	      LA_PRINT_CHAR ((unsigned char) c, type, stream);
	    }
	}
      break;

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      /* Starting from the Fortran 90 standard, Fortran supports derived
         types.  */
      fprintf_filtered (stream, "( ");
      for (index = 0; index < TYPE_NFIELDS (type); index++)
        {
          int offset = TYPE_FIELD_BITPOS (type, index) / 8;

          val_print (TYPE_FIELD_TYPE (type, index), valaddr,
		     embedded_offset + offset,
		     address, stream, recurse + 1,
		     original_value, options, current_language);
          if (index != TYPE_NFIELDS (type) - 1)
            fputs_filtered (", ", stream);
        }
      fprintf_filtered (stream, " )");
      break;     

    case TYPE_CODE_REF:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_FLT:
    case TYPE_CODE_VOID:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_UNDEF:
    case TYPE_CODE_COMPLEX:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_CHAR:
    default:
      generic_val_print (type, valaddr, embedded_offset, address,
			 stream, recurse, original_value, options,
			 &f_decorations);
      break;
    }
  gdb_flush (stream);
}

static void
list_all_visible_commons (const char *funname)
{
  SAVED_F77_COMMON_PTR tmp;

  tmp = head_common_list;

  printf_filtered (_("All COMMON blocks visible at this level:\n\n"));

  while (tmp != NULL)
    {
      if (strcmp (tmp->owning_function, funname) == 0)
	printf_filtered ("%s\n", tmp->name);

      tmp = tmp->next;
    }
}

/* This function is used to print out the values in a given COMMON 
   block.  It will always use the most local common block of the 
   given name.  */

static void
info_common_command (char *comname, int from_tty)
{
  SAVED_F77_COMMON_PTR the_common;
  COMMON_ENTRY_PTR entry;
  struct frame_info *fi;
  const char *funname = 0;
  struct symbol *func;

  /* We have been told to display the contents of F77 COMMON 
     block supposedly visible in this function.  Let us 
     first make sure that it is visible and if so, let 
     us display its contents.  */

  fi = get_selected_frame (_("No frame selected"));

  /* The following is generally ripped off from stack.c's routine 
     print_frame_info().  */

  func = find_pc_function (get_frame_pc (fi));
  if (func)
    {
      /* In certain pathological cases, the symtabs give the wrong
         function (when we are in the first function in a file which
         is compiled without debugging symbols, the previous function
         is compiled with debugging symbols, and the "foo.o" symbol
         that is supposed to tell us where the file with debugging symbols
         ends has been truncated by ar because it is longer than 15
         characters).

         So look in the minimal symbol tables as well, and if it comes
         up with a larger address for the function use that instead.
         I don't think this can ever cause any problems; there shouldn't
         be any minimal symbols in the middle of a function.
         FIXME:  (Not necessarily true.  What about text labels?)  */

      struct minimal_symbol *msymbol = 
	lookup_minimal_symbol_by_pc (get_frame_pc (fi));

      if (msymbol != NULL
	  && (SYMBOL_VALUE_ADDRESS (msymbol)
	      > BLOCK_START (SYMBOL_BLOCK_VALUE (func))))
	funname = SYMBOL_LINKAGE_NAME (msymbol);
      else
	funname = SYMBOL_LINKAGE_NAME (func);
    }
  else
    {
      struct minimal_symbol *msymbol =
	lookup_minimal_symbol_by_pc (get_frame_pc (fi));

      if (msymbol != NULL)
	funname = SYMBOL_LINKAGE_NAME (msymbol);
      else /* Got no 'funname', code below will fail.  */
	error (_("No function found for frame."));
    }

  /* If comname is NULL, we assume the user wishes to see the 
     which COMMON blocks are visible here and then return.  */

  if (comname == 0)
    {
      list_all_visible_commons (funname);
      return;
    }

  the_common = find_common_for_function (comname, funname);

  if (the_common)
    {
      if (strcmp (comname, BLANK_COMMON_NAME_LOCAL) == 0)
	printf_filtered (_("Contents of blank COMMON block:\n"));
      else
	printf_filtered (_("Contents of F77 COMMON block '%s':\n"), comname);

      printf_filtered ("\n");
      entry = the_common->entries;

      while (entry != NULL)
	{
	  print_variable_and_value (NULL, entry->symbol, fi, gdb_stdout, 0);
	  entry = entry->next;
	}
    }
  else
    printf_filtered (_("Cannot locate the common block %s in function '%s'\n"),
		     comname, funname);
}

/* This function is used to determine whether there is a
   F77 common block visible at the current scope called 'comname'.  */

#if 0
static int
there_is_a_visible_common_named (char *comname)
{
  SAVED_F77_COMMON_PTR the_common;
  struct frame_info *fi;
  char *funname = 0;
  struct symbol *func;

  if (comname == NULL)
    error (_("Cannot deal with NULL common name!"));

  fi = get_selected_frame (_("No frame selected"));

  /* The following is generally ripped off from stack.c's routine 
     print_frame_info().  */

  func = find_pc_function (fi->pc);
  if (func)
    {
      /* In certain pathological cases, the symtabs give the wrong
         function (when we are in the first function in a file which
         is compiled without debugging symbols, the previous function
         is compiled with debugging symbols, and the "foo.o" symbol
         that is supposed to tell us where the file with debugging symbols
         ends has been truncated by ar because it is longer than 15
         characters).

         So look in the minimal symbol tables as well, and if it comes
         up with a larger address for the function use that instead.
         I don't think this can ever cause any problems; there shouldn't
         be any minimal symbols in the middle of a function.
         FIXME:  (Not necessarily true.  What about text labels?)  */

      struct minimal_symbol *msymbol = lookup_minimal_symbol_by_pc (fi->pc);

      if (msymbol != NULL
	  && (SYMBOL_VALUE_ADDRESS (msymbol)
	      > BLOCK_START (SYMBOL_BLOCK_VALUE (func))))
	funname = SYMBOL_LINKAGE_NAME (msymbol);
      else
	funname = SYMBOL_LINKAGE_NAME (func);
    }
  else
    {
      struct minimal_symbol *msymbol =
	lookup_minimal_symbol_by_pc (fi->pc);

      if (msymbol != NULL)
	funname = SYMBOL_LINKAGE_NAME (msymbol);
    }

  the_common = find_common_for_function (comname, funname);

  return (the_common ? 1 : 0);
}
#endif

void
_initialize_f_valprint (void)
{
  add_info ("common", info_common_command,
	    _("Print out the values contained in a Fortran COMMON block."));
  if (xdb_commands)
    add_com ("lc", class_info, info_common_command,
	     _("Print out the values contained in a Fortran COMMON block."));
}
