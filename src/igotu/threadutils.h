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

#ifndef _IGOTU2GPX_SRC_IGOTU_THREADUTILS_H_
#define _IGOTU2GPX_SRC_IGOTU_THREADUTILS_H_

#include "global.h"

#include <QThread>

/** Thread that calls exec() in its #run() method. Allows the use of
 *
 * @code
 * // create the producer and consumer and plug them together
 * Producer producer;
 * Consumer consumer;
 * producer.connect(&consumer, SIGNAL(consumed()), SLOT(produce()));
 * consumer.connect(&producer, SIGNAL(produced(QByteArray*)),
 *     SLOT(consume(QByteArray*)));
 *
 * // they both get their own thread
 * EventThread producerThread;
 * producer.moveToThread(&producerThread);
 * EventThread consumerThread;
 * consumer.moveToThread(&consumerThread);
 *
 * // go!
 * producerThread.start();
 * consumerThread.start();
 * @endcode
 *
 * This class can go away with Qt 4.4.
 *
 * @see labs.trolltech.com/blogs/2006/12/04/threading-without-the-headache/
 * @ingroup threads
 */
class EventThread : public QThread
{
public:
    EventThread(QObject *parent = NULL) :
        QThread(parent)
    {
    }

protected:
    /** Calls exec(). */
    void run()
    {
        exec();
    }
};

#endif
