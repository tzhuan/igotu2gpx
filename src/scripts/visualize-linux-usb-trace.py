#!/usr/bin/python

###############################################################################
# Copyright (C) 2010  Michael Hofmann <mh21@piware.de>                        #
#                                                                             #
# This program is free software; you can redistribute it and/or modify        #
# it under the terms of the GNU General Public License as published by        #
# the Free Software Foundation; either version 3 of the License, or           #
# (at your option) any later version.                                         #
#                                                                             #
# This program is distributed in the hope that it will be useful,             #
# but WITHOUT ANY WARRANTY; without even the implied warranty of              #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               #
# GNU General Public License for more details.                                #
#                                                                             #
# You should have received a copy of the GNU General Public License along     #
# with this program; if not, write to the Free Software Foundation, Inc.,     #
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.                 #
###############################################################################

import matplotlib.pyplot as plt
import sys
import fileinput

y1 = []
x1 = []
y2 = []
x2 = []
labels = []

limit = 100;
figurenum = 1;
mintime = -1;
urbs = {};

for line in fileinput.input():
    if fileinput.lineno() > limit:
        fileinput.nextfile()
        continue

    if fileinput.isfirstline():
        plt.figure(figurenum)
        plt.axes()
        plt.title(fileinput.filename())
        figurenum += 1

    record = line.strip().split(' ', 4)
    if len(record) < 5:
        continue

    time = record[1]
    inttime = int(time) / 1000.0
    type = record[2]
    endpoint = record[3]
    data = record[4]

    if mintime == -1:
        mintime = inttime

    if type == 'S':
        urbs[endpoint] = {'data': data, 'time': time, 'inttime' : inttime}
    else:
        if not urbs.has_key(endpoint):
            print('Unknown URB, ignoring')
            continue

        submit = urbs[endpoint]
        if endpoint.startswith('C'):
            plt.plot([0, -1, -1, 0], [-submit['inttime'], -submit['inttime'],
                -inttime, -inttime], color='blue')
            text = ' '.join(submit['data'].split(' ')[8:]);
            plt.text(-1.1, -(submit['inttime'] + inttime) / 2.0,
                    '%s : %.1fms' % (text, inttime - submit['inttime']),
                    horizontalalignment='right', verticalalignment='center')
        else:
            plt.plot([0, +1, +1, 0], [-submit['inttime'], -submit['inttime'],
                -inttime, -inttime], color='red')
            if data.startswith('-2:'):
                text = 'timeout';
            else:
                text = ' '.join(data.split(' ')[3:]);
            plt.text(1.1, -(submit['inttime'] + inttime) / 2.0,
                    '%.1fms : %s' % (inttime - submit['inttime'], text),
                    horizontalalignment='left', verticalalignment='center')

plt.plot([0, 0], [-inttime, -mintime], color='black')
plt.xlim(-5, 5)
#plt.ylim(-inttime, -mintime)
plt.text(-.5, -mintime, '->device', horizontalalignment='center',
        verticalalignment='bottom')
plt.text(.5, -mintime, '->host', horizontalalignment='center',
        verticalalignment='bottom')
plt.show()
