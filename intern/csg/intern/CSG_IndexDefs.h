#ifndef CSG_INDEXDEFS_H
#define CSG_INDEXDEFS_H
/*
  CSGLib - Software Library for Constructive Solid Geometry
  Copyright (C) 2003-2004  Laurence Bourn

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

  Please send remarks, questions and bug reports to laurencebourn@hotmail.com
*/

//typdefs for lists and things in the CSG library

#include <vector>

typedef std::vector<int> PIndexList;
typedef PIndexList::iterator PIndexIt;
typedef PIndexList::const_iterator const_PIndexIt;

typedef std::vector<int> VIndexList;
typedef VIndexList::iterator VIndexIt;
typedef VIndexList::const_iterator const_VIndexIt;

typedef std::vector< PIndexList > OverlapTable;



#endif


