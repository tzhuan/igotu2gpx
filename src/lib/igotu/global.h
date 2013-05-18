/******************************************************************************
 * Copyright (C) 2007  Michael Hofmann <mh21@mh21.de>                         *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation; either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License along    *
 * with this program; if not, write to the Free Software Foundation, Inc.,    *
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.                *
 ******************************************************************************/

#ifndef _IGOTU2GPX_SRC_IGOTU_GLOBAL_H_
#define _IGOTU2GPX_SRC_IGOTU_GLOBAL_H_

#include <QtGlobal>

#define IGOTU_VERSION_STR "0.3.90"

#if defined(Q_CC_MSVC)
    #define DECLARE_DEPRECATED __declspec(deprecated)
#else
    #define DECLARE_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifdef IGOTU_MAKEDLL
    #define IGOTU_EXPORT Q_DECL_EXPORT
#else
    #define IGOTU_EXPORT Q_DECL_IMPORT
#endif

#define RETURN_IF_FAIL(test) if (!(test)) { qCritical("CRITICAL: %s: !(%s)", Q_FUNC_INFO, #test); return; }
#define RETURN_VAL_IF_FAIL(test, val) if (!(test)) { qCritical("CRITICAL: %s: !(%s)", Q_FUNC_INFO, #test); return val; }

#endif

