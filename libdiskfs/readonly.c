/* Change to/from read-only

   Copyright (C) 1995, 1996 Free Software Foundation, Inc.

   Written by Miles Bader <miles@gnu.ai.mit.edu>

   This file is part of the GNU Hurd.

   The GNU Hurd is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   The GNU Hurd is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include <fcntl.h>

#include "priv.h"

int _diskfs_diskdirty;

int
diskfs_check_readonly ()
{
  if (diskfs_readonly)
    return 1;
  else
    {
      if (!_diskfs_diskdirty)
	{
	  diskfs_set_hypermetadata (1, 0);
	  _diskfs_diskdirty = 1;
	}
      return 0;
    }
}

/* Change an active filesystem between read-only and writable modes, setting
   the global variable DISKFS_READONLY to reflect the current mode.  If an
   error is returned, nothing will have changed.  The user should hold
   DISKFS_FSYS_LOCK while calling this routine.  */
error_t
diskfs_set_readonly (int readonly)
{
  error_t err = 0;

  if (readonly != diskfs_readonly)
    {
      err = ports_inhibit_class_rpcs (diskfs_protid_class);
      if (! err)
	{
	  if (readonly)
	    {
	      error_t peropen_writable (void *pi)
		{
		  if (((struct port_info *)pi)->class == diskfs_protid_class
		      && (((struct protid *)pi)->po->openstat & O_WRITE))
		    return EBUSY;
		  else
		    return 0;
		}

	      /* Any writable open files?  */
	      err = ports_bucket_iterate (diskfs_port_bucket, peropen_writable);

	      /* Any writable pagers?  */
	      if (!err && (diskfs_max_user_pager_prot () & VM_PROT_WRITE))
		err = EBUSY;

	      if (!err)
		/* Sync */
		{
		  diskfs_sync_everything (1);
		  diskfs_set_hypermetadata (1, 1);
		  _diskfs_diskdirty = 0;
		}
	    }

	  if (!err)
	    {
	      diskfs_readonly = readonly;
	      diskfs_readonly_changed (readonly);
	    }

	  ports_resume_class_rpcs (diskfs_protid_class);
	}
    }

  return err;
}
