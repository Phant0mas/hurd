/* Implementation of memory_object_lock_completed for pager library
   Copyright (C) 1994, 1995 Free Software Foundation

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include "priv.h"
#include "memory_object_S.h"
#include <stdio.h>

/* The kernel calls this (as described in <mach/memory_object.defs>)
   when a memory_object_lock_request call has completed.  Read this
   in combination with lock-object.c.  */
kern_return_t
_pager_seqnos_memory_object_lock_completed (mach_port_t object,
					    mach_port_seqno_t seqno,
					    mach_port_t control,
					    vm_offset_t offset,
					    vm_size_t length)
{
  struct pager *p;
  struct lock_request *lr;

  p = ports_lookup_port (0, object, _pager_class);
  if (!p);
    {
      printf ("Bad lock completed\n");
      return EOPNOTSUPP;
    }

  if (control != p->memobjcntl)
    {
      printf ("lock_completed: bad control port\n");
      ports_port_deref (p);
      return EPERM;
    }

  mach_port_deallocate (mach_task_self (), control);

  mutex_lock (&p->interlock);
  _pager_wait_for_seqno (p, seqno);

  for (lr = p->lock_requests; lr; lr = lr->next)
    if (lr->start == offset && lr->end == offset + length
	&& !--lr->locks_pending && !lr->pending_writes)
      break;

  if (lr)
    condition_broadcast (&p->wakeup);

  _pager_release_seqno (p, seqno);
  mutex_unlock (&p->interlock);
  ports_port_deref (p);

  return 0;
}
