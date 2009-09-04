/******************************************************************************
 * Copyright (C) 2007  Michael Hofmann <mh21@piware.de>                       *
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

#define IGOTU_VERSION_STR "0.2.91"

#define GCC_VERSION (__GNUC__ * 100                                         \
                   + __GNUC_MINOR__ * 10                                    \
                   + __GNUC_PATCHLEVEL)

#if defined(Q_CC_MSVC) || defined(Q_OS_WIN32)
    #define EXPORT_DECL __declspec(dllexport)
    #define IMPORT_DECL __declspec(dllimport)
#else
    // Test for GCC >= 4.0.0
    #if GCC_VERSION >= 400
        #define EXPORT_DECL __attribute__ ((visibility("default")))
        #define IMPORT_DECL
    #else
        #define EXPORT_DECL
        #define IMPORT_DECL
    #endif
#endif

#if defined(Q_CC_MSVC)
    #define DECLARE_DEPRECATED __declspec(deprecated)
#else
    #define DECLARE_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifdef IGOTU_MAKEDLL
    #define IGOTU_EXPORT EXPORT_DECL
#else
    #define IGOTU_EXPORT IMPORT_DECL
#endif

#endif

