#!/usr/bin/python

###############################################################################
# Copyright (C) 2009  Michael Hofmann <mh21@piware.de>                        #
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

from __future__ import with_statement
from struct import unpack, unpack_from
import re, sys

def decode_hex(data):
    return re.sub(r'Length \d+:', '', data).replace(' ', '').decode('hex')

def handle_part(part):
    query = part['query']
    response =  part['response']
    if len(response) < 3:
        raise Exception('Response too short: %s, %s' % (query.encode('hex'),
            response.encode('hex')))
    if response[0] != '\x93':
        raise Exception('Unknown response: %s' % response.encode('hex'))
    responsesize = unpack('>h', response[1:3])[0];
    return query, responsesize, response[3:]

def format_query(query, length = 0, checkchecksum = True):
    if checkchecksum:
        checksum = sum(ord(i) for i in query) % 256
        if checksum != 0:
            raise Exception('Wrong checksum: got %d for %s' % (checksum,
                query.encode('hex')))
        query = query[0:-1]
    stripped = query.rstrip('\x00')
    if length > 0 and len(stripped) < length:
        stripped += '\x00' * (length - len(stripped))
    return stripped.encode('hex')

def format_hex_line(data):
    result = ''
    hexresponse = data.encode('hex')
    for j in range(0, len(data)):
        if j % 8 == 0:
            result += ' '
        result += hexresponse[j * 2:(j + 1) * 2] + ' '
    result += ''.join([i if (ord(i) >= 0x20 and ord(i) <= 0x7e) else '.' for
        i in data])
    return result

def format_hex(data):
    result = ''
    for i in range(0, (len(data) + 15) / 16):
        result += '  ' + format_hex_line(data[i * 16:(i + 1) * 16]) + '\n'
    return result

readdata = '';
writedata = '';
parts = [];
with open(sys.argv[1], 'r') as f:
    for line in f.xreadlines():
        tokens = line.split('\t')
        command = tokens[3]
        data = tokens[6]
        if command == 'IRP_MJ_WRITE':
            if len(writedata) > 0:
                parts += [{'query': writedata, 'response': readdata}]
            readdata = ''
            writedata = ''
        if command == 'IRP_MJ_READ':
            readdata += decode_hex(data)
        if command == 'IRP_MJ_WRITE':
            writedata = decode_hex(data)
if len(writedata) > 0:
    parts += [{'query': writedata, 'response': readdata}]

rawdatapackages = 0
tempqueryparts = []
while len(parts) > 0:
    query, size, response = handle_part(parts.pop(0))
    if size < 0:
        print 'Query with error: %s' % format_query(query, 8, False)
        tempqueryparts = []
    elif rawdatapackages > 0:
        print 'Data write: (%s, returned %d)' % (format_query(query, 7), size)
        print format_hex(query[0:7]),
        rawdatapackages -= 1
    elif len(tempqueryparts) == 0:
        tempqueryparts += [query]
    else:
        query = tempqueryparts.pop() + query

        # TODO
        #    if response1 != '':
        #        raise Exception('Non-empty response: %s' % response1.encode('hex'))
        #    if responsesize != len(response) - 3:
        #        raise Exception('Wrong response size: expected %d, got %d' %
        #                (responsesize, len(response) - 3))
        #    return query, response[3:]

        # NMEA
        if query.startswith('\x93\x01\x01'):
            print 'NMEA disabled: %02x (%s, returned %d)' % (ord(query[3]),
                    format_query(query, 4), size)
        # Identification
        elif query.startswith('\x93\x0a'):
            idresponse = unpack_from('<IBB4s', response)
            print 'Identification: s/n %d, version %u.%02u, unknown %s (%s, returned %d)' % (idresponse[0],
                    idresponse[1], idresponse[2], idresponse[3].encode('hex'),
                    format_query(query, 2), size)
        # Count
        elif query.startswith('\x93\x0b\x03') and query[4] == '\x1d':
            countresponse = unpack_from('>xH', response)
            print 'Trackpoint count: count %d (%s, returned %d)' % (
                    countresponse[0], format_query(query, 5), size)
        # Memory read
        elif query.startswith('\x93\x0b'):
            readquery = unpack_from('>xxBH', query)
            print 'Memory read: pos %04x, size %02x (%s, returned %d)' % (
                    readquery[1], readquery[0],
                    format_query(query, 5), size)
        # Read
        elif query.startswith('\x93\x05\x07') and query[5:7] == '\x04\x03':
            readquery = unpack_from('>xxxHxxBH', query)
            print 'Block read: pos %06x, size %04x (%s, returned %d)' % (readquery[1] *
                    0x10000 + readquery[2], readquery[0],
                    format_query(query, 10), size)
        # Write
        elif query.startswith('\x93\x06\x07') and query[5:7] == '\x04\x02':
            writequery = unpack_from('>xxxHxxxH', query)
            print 'Block write: pos %04x, size %04x (%s, returned %d)' % (writequery[1],
                    writequery[0], format_query(query, 10), size)
            rawdatapackages = (writequery[0] + 6) / 7
        else:
            print 'Unknown query: %s, returned %d' % (format_query(query, 15),
                    size)
    if len(response) > 0:
        print format_hex(response),
