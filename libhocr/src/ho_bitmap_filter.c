/***************************************************************************
 *            ho_bitmap_filter.c
 *
 *  Fri Aug 12 20:13:33 2005
 *  Copyright  2005-2007  Yaacov Zamir
 *  <kzamir@walla.co.il>
 ****************************************************************************/

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "ho_common.h"
#include "ho_bitmap.h"
#include "ho_objmap.h"
#include "ho_bitmap_filter.h"

ho_bitmap *
ho_bitmap_filter_by_size (const ho_bitmap * m,
			  ho_uint min_height, ho_uint max_height,
			  ho_uint min_width, ho_uint max_width)
{
  ho_objmap *m_obj;
  ho_bitmap *m_out;

  /* create a new objmap */
  m_obj = ho_objmap_new_from_bitmap (m);

  if (!m_obj)
    return HO_NULL;

  m_out = ho_objmap_to_bitmap_by_size (m_obj,
				       min_height, max_height,
				       min_width, max_width);

  /* free objmap */
  ho_objmap_free (m_obj);

  return m_out;
}

ho_bitmap *
ho_bitmap_filter_boxes (const ho_bitmap * m)
{
  ho_objmap *m_obj;
  ho_bitmap *m_out;
  ho_usint index;
  ho_uint x, y, width, height;

  /* allocate memory */
  m_obj = ho_objmap_new_from_bitmap (m);
  if (!m_obj)
    return HO_NULL;
  m_out = ho_bitmap_new (m->width, m->height);
  if (!m_out)
    {
      ho_objmap_free (m_obj);
      return HO_NULL;
    }

  /* loop over all the objects and box them */
  for (index = 0; index < m_obj->obj_list->size; index++)
    {
      x = (((m_obj->obj_list)->objects)[index]).x;
      y = (((m_obj->obj_list)->objects)[index]).y;
      width = (((m_obj->obj_list)->objects)[index]).width;
      height = (((m_obj->obj_list)->objects)[index]).height;

      ho_bitmap_draw_box (m_out, x, y, width, height);
    }

  return m_out;
}

ho_bitmap *
ho_bitmap_filter_fill (const ho_bitmap * m)
{
  ho_objmap *m_obj;
  ho_bitmap *m_out;
  ho_bitmap *m_temp1;
  ho_bitmap *m_temp2;
  ho_usint index;
  ho_uint width, height;

  /* allocate memory */
  m_obj = ho_objmap_new_from_bitmap (m);
  if (!m_obj)
    return HO_NULL;

  m_out = ho_bitmap_new (m->width, m->height);
  if (!m_out)
    {
      ho_objmap_free (m_obj);
      return HO_NULL;
    }

  /* loop over all the objects and box them */
  for (index = 0; index < m_obj->obj_list->size; index++)
    {
      /* get a dimention factor */
      width = (((m_obj->obj_list)->objects)[index]).width;
      height = (((m_obj->obj_list)->objects)[index]).height;

      /* copy only the current object to a new bitmap */
      m_temp1 = ho_objmap_to_bitmap_by_index (m_obj, index);

      /* fill the current object */
      m_temp2 = ho_bitmap_hlink (m_temp1, width / 4);
      ho_bitmap_free (m_temp1);
      m_temp1 = ho_bitmap_vlink (m_temp2, height / 4);
      ho_bitmap_free (m_temp2);

      /* add to matrix out */
      ho_bitmap_or (m_out, m_temp1);
      ho_bitmap_free (m_temp1);
    }

  return m_out;
}

ho_bitmap *
ho_bitmap_filter_obj_max_height (const ho_bitmap * m, const ho_uint height)
{
  ho_objmap *m_obj;
  ho_bitmap *m_out;
  ho_bitmap *m_temp1;
  ho_bitmap *m_temp2;
  ho_usint index;

  /* allocate memory */
  m_obj = ho_objmap_new_from_bitmap (m);
  if (!m_obj)
    return HO_NULL;

  m_out = ho_bitmap_new (m->width, m->height);
  if (!m_out)
    {
      ho_objmap_free (m_obj);
      return HO_NULL;
    }

  /* loop over all the objects and box them */
  for (index = 0; index < m_obj->obj_list->size; index++)
    {
      /* copy only the current object to a new bitmap */
      m_temp1 = ho_objmap_to_bitmap_by_index (m_obj, index);

      /* take height pixels from this object */
      m_temp2 = ho_bitmap_max_height (m_temp1, 0, height);
      ho_bitmap_free (m_temp1);

      /* add to matrix out */
      ho_bitmap_or (m_out, m_temp2);
      ho_bitmap_free (m_temp2);
    }

  return m_out;
}

ho_bitmap *
ho_bitmap_filter_hlink (ho_bitmap * m, ho_uint size,
			       ho_uint max_height)
{
  ho_bitmap *m_out;
  ho_bitmap *m_temp;

  /* this function use objects by the "_by_size" function
      this is why it is a filter and not regular bitmap function */
  
  /* get only the thin objects */
  m_temp = ho_bitmap_filter_by_size (m, 5, max_height, 5, m->width / 2);

  /* hlink the thin objects */
  m_out = ho_bitmap_hlink (m_temp, size);

  /* add the rest of the large objects */
  ho_bitmap_or (m_out, m);

  ho_bitmap_free (m_temp);

  return m_out;
}

ho_bitmap *
ho_bitmap_filter_remove_dots (const ho_bitmap * m,
			      const ho_uchar erosion_n,
			      const ho_uchar dilation_n)
{
  ho_uint x, y;
  ho_uchar sum;
  ho_bitmap *m_temp;
  ho_bitmap *m_out;
  ho_objmap *m_obj;
  ho_usint index;
  ho_uint width, height;

  /* allocate memory */
  m_out = ho_bitmap_new (m->width, m->height);
  if (!m_out)
    return HO_NULL;

  /* connect all the small dots */
  m_temp = ho_bitmap_dilation (m);

  /* check the size of objects */
  m_obj = ho_objmap_new_from_bitmap (m_temp);
  ho_bitmap_free (m_temp);

  for (x = 1; x < (m->width - 1); x++)
    for (y = 1; y < (m->height - 1); y++)
      {

	/* check the size of this pixel's object */
	index = ho_objmap_get (m_obj, x, y);
	if (index)
	  {
	    width = (((m_obj->obj_list)->objects)[index - 1]).width;
	    height = (((m_obj->obj_list)->objects)[index - 1]).height;
	  }
	else
	  {
	    width = 0;
	    height = 0;
	  }

	/* in a big object do erosion */
	if (width > m->width / 4 || height > m->height / 4)
	  {
	    if (ho_bitmap_get (m, x, y))	/* black pixel */
	      {
		sum = ho_bitmap_get (m, x - 1, y - 1) +
		  ho_bitmap_get (m, x - 1, y) +
		  ho_bitmap_get (m, x - 1, y + 1) +
		  ho_bitmap_get (m, x, y - 1) +
		  ho_bitmap_get (m, x, y + 1) +
		  ho_bitmap_get (m, x + 1, y - 1) +
		  ho_bitmap_get (m, x + 1, y) +
		  ho_bitmap_get (m, x + 1, y + 1);
		/* n number of white pixels or more */
		if ((8 - sum) < erosion_n)
		  ho_bitmap_set (m_out, x, y);
	      }
	  }
	else			/* if in a small object do dilation */
	  {
	    if (!ho_bitmap_get (m, x, y))	/* white pixel */
	      {
		sum = ho_bitmap_get (m, x - 1, y - 1) +
		  ho_bitmap_get (m, x - 1, y) +
		  ho_bitmap_get (m, x - 1, y + 1) +
		  ho_bitmap_get (m, x, y - 1) +
		  ho_bitmap_get (m, x, y + 1) +
		  ho_bitmap_get (m, x + 1, y - 1) +
		  ho_bitmap_get (m, x + 1, y) +
		  ho_bitmap_get (m, x + 1, y + 1);
		/* n number of black neighbors or more */
		if (sum >= dilation_n)
		  ho_bitmap_set (m_out, x, y);
	      }
	    else
	      ho_bitmap_set (m_out, x, y);
	  }
      }

  ho_objmap_free (m_obj);

  return m_out;
}
