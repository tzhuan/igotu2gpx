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

#ifndef _IGOTU2GPX_SRC_IGOTU_COMMONMESSAGES_H_
#define _IGOTU2GPX_SRC_IGOTU_COMMONMESSAGES_H_

#include <QCoreApplication>

namespace igotu
{

// Translations are done using launchpad.net. With Qt, the class is used as
// context in the po files, but identical strings are not merged, which results
// in errors from "msgfmt -c". So strings that are used multiple times are moved
// to Common::tr(). use "make podcheck" to check for them.
class Common
{
    Q_DECLARE_TR_FUNCTIONS(Common)
};

} // namespace igotu

#endif
