/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2007,2008 Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/machine/memory.h>
#include <grub/ieee1275/ieee1275.h>
#include <grub/types.h>
#include <grub/misc.h>

GRUB_EXPORT(grub_machine_mmap_iterate);

grub_err_t
grub_machine_mmap_iterate (int (*hook) (grub_uint64_t, grub_uint64_t,
					grub_uint32_t, void *closure),
			   void *closure)
{
  grub_ieee1275_phandle_t root;
  grub_ieee1275_phandle_t memory;
  grub_uint32_t available[32];
  grub_ssize_t available_size;
  grub_uint32_t address_cells = 1;
  grub_uint32_t size_cells = 1;
  char copyright[128];
  int i;

  /* Determine the format of each entry in `available'.  */
  grub_ieee1275_finddevice ("/", &root);
  grub_ieee1275_get_integer_property (root, "#address-cells", &address_cells,
				      sizeof address_cells, 0);
  grub_ieee1275_get_integer_property (root, "#size-cells", &size_cells,
				      sizeof size_cells, 0);

  grub_ieee1275_get_property (root, "copyright", copyright,
			      sizeof copyright, 0);

  if (size_cells > address_cells)
    address_cells = size_cells;

  /* Apple ppc g4, g5 /memory/available[] uses 32bit offset and size,
     including for >4GB fitted RAM,
     and does not use root cell sizes. Maybe some other ieee1275 Apple
     uses 64bit offset and size? */
  if (grub_strstr (copyright, "Apple"))
    address_cells = size_cells;

  /* Load `/memory/available'.  */
  if (grub_ieee1275_finddevice ("/memory", &memory))
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE,
		       "couldn't find /memory node");
  if (grub_ieee1275_get_integer_property (memory, "available", available,
					  sizeof available, &available_size))
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE,
		       "couldn't examine /memory/available property");

  /* Decode each entry and call `hook'.  */
  i = 0;
  available_size /= sizeof (grub_uint32_t);
  while (i < available_size)
    {
      grub_uint64_t address;
      grub_uint64_t size;

      address = available[i++];
      if (address_cells == 2)
	address = (address << 32) | available[i++];

      size = available[i++];
      if (size_cells == 2)
	size = (size << 32) | available[i++];

      if (hook (address, size, GRUB_MACHINE_MEMORY_AVAILABLE, closure))
	break;
    }

  return grub_errno;
}
