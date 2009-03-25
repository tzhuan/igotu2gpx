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

#include "verbose.h"

#include <QMutex>

namespace igotu
{

class VerbosePrivate
{
public:
    VerbosePrivate() :
        level(0)
    {
    }

    int verbose()
    {
        QMutexLocker locker(&lock);
        return level;
    }

    void setVerbose(int value)
    {
        QMutexLocker locker(&lock);
        level = value;
    }

private:
    int level;
    QMutex lock;
};

Q_GLOBAL_STATIC(VerbosePrivate, verbosePrivate)

int Verbose::verbose()
{
    return verbosePrivate()->verbose();
}

void Verbose::setVerbose(int value)
{
    verbosePrivate()->setVerbose(value);
}

} // namespace igotu
