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

#ifndef _IGOTU_SRC_IGOTU_XMLUTILS_H_
#define _IGOTU_SRC_IGOTU_XMLUTILS_H_

#include "global.h"

namespace igotu
{

/** Returns indentation for XML writers. Returns @a indentSize * @a
 * indentLevel space characters.
 *
 * @param indentLevel indentation level
 * @param indentSize indentation size
 * @return string with @a indentSize * @a indentLevel space characters
 */
IGOTU_EXPORT QString xmlIndent(unsigned indentLevel,
        unsigned indentSize = 4);

/** Escapes text for direct output to an XML file. Replaces lower than,
 * greater than and ampersands.
 *
 * @param text input text
 * @return text with entities replaced
 */
IGOTU_EXPORT QString xmlEscapedText(const QString &text);

/** Escapes text for use as an XML attribute. Additionally to the work done
 * by #xmlEscapedText(), it replaces all existing double quotes and encloses
 * the whole string in double quotes.
 *
 * @param text input text
 * @return text with entities replaced and enclosed in quotes
 */
IGOTU_EXPORT QString xmlEscapedAttribute(const QString &text);

} // namespace igotu

#endif

