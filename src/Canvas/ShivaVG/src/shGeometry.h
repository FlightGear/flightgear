/*
 * Copyright (c) 2007 Ivan Leben
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library in the file COPYING;
 * if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __SH_GEOMETRY_H

#include "shDefs.h"
#include "shContext.h"
#include "shVectors.h"
#include "shPath.h"

void shFlattenPath(SHPath *p, SHint surfaceSpace);
void shStrokePath(VGContext* c, SHPath *p);
void shTransformVertices(SHMatrix3x3 *m, SHPath *p);
void shFindBoundbox(SHPath *p);

#endif /* __SH_GEOMETRY_H */
