/******************************************************************************
 * Copyright (C) 2009  Michael Hofmann <mh21@piware.de>                       *
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

#ifndef _IGOTU2GPX_SRC_TESTS_TESTS_H_
#define _IGOTU2GPx_SRC_TESTS_TESTS_H_

#include <QtTest>

#define ARRAYCOMP(a, b, n)                                                     \
    do {                                                                       \
        for (unsigned compLoop = 0; compLoop < n; ++compLoop)                  \
            if (a[compLoop] != b[compLoop])                                    \
                QFAIL(qPrintable(QString::fromLatin1                           \
                ("Array values not the same at index %1: was %2, expected %3") \
                            .arg(compLoop)                                     \
                            .arg(uchar(a[compLoop]))                           \
                            .arg(uchar(b[compLoop]))));                        \
    } while (0)

class Tests: public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void igotuConfig();
};

#endif
