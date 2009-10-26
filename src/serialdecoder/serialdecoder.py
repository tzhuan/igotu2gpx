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
remotemode = False;
with open(sys.argv[1], 'r') as f:
    firstline = True;
    responseline = False;
    for line in f.xreadlines():
        if firstline:
            firstline = False;
            if line.startswith('['):
                remotemode = True;
                continue
        if remotemode:
            if responseline:
                newtokens = line.rstrip().split('  ')
                if len(newtokens) < 4:
                    if len(tokens) > 5:
                        data = tokens[5]
                    else:
                        data = ''
                else:
                    data = newtokens[3]
                responseline = False
            else:
                tokens = line.rstrip().split('  ')
                command = tokens[3]
                responseline = True
                continue
        else:
            tokens = line.split('\t')
            command = tokens[3]
            data = tokens[6].rstrip()
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

        # NmeaSwitchCommand
        if query.startswith('\x93\x01\x01'):
            print 'NmeaSwitchCommand(enable = %d)' % (query[3] == 0)
        # IdentificationCommand
        elif query.startswith('\x93\x0a'):
            r = unpack_from('<IBB4s', response)
            print 'IdentificationCommand()'
            print '  serialNumber() -> %d' % (r[0])
            print '  firmwareVersion() -> %u.%02u' % (r[1], r[2])
        # CountCommand
        elif query.startswith('\x93\x0b\x03') and query[4] == '\x1d':
            r = unpack_from('>xH', response)
            print 'CountCommand()'
            print '  trackPointCount() -> %d' % (r[0])
        # ModelCommand
        elif query.startswith('\x93\x05\x04\x00\x03\x01\x9f'):
            r = unpack_from('>BH', response)
            print 'ModelCommand()'
            print '  modelName() -> 0x%06x' % (r[0] * 0x10000 + r[1])
        # ReadCommand
        elif query.startswith('\x93\x05\x07') and query[5:7] == '\x04\x03':
            r = unpack_from('>xxxHxxBH', query)
            print 'ReadCommand(pos = 0x%06x, size = 0x%04x)' % (r[1] * 0x10000 +
                    r[2], r[0])
        # WriteCommand
        elif query.startswith('\x93\x06\x07') and query[5] == '\x04':
            r = unpack_from('>xxxHxBBH', query)
            print 'WriteCommand(mode = 0x%02x, pos = 0x%06x, size = 0x%04x)' % (r[1],
                    r[2] * 0x1000 + r[3], r[0])
            rawdatapackages = (r[0] + 6) / 7
        # TimeCommand
        elif query.startswith('\x93\x09\x03'):
            r = unpack_from('>xxxBBB', query)
            print 'TimeCommand(time = time(%02u, %02u, %02u)' % (r[0], r[1],
                    r[2])
        # UnknownPurgeCommand1
        elif query.startswith('\x93\x0c\x00'):
            r = unpack_from('>xxxB', query)
            print 'UnknownPurgeCommand1(mode = 0x%02x)' % (r[0])
        # UnknownPurgeCommand2
        elif query.startswith('\x93\x08\x02'):
            print 'UnknownPurgeCommand2()'
        # UnknownWriteCommand1
        elif query.startswith('\x93\x06\x04\x00') and query[5:7] == '\x01\x06':
            r = unpack_from('>xxxxB', query)
            print 'UnknownWriteCommand1(mode = 0x%02x)' % (r[0])
        # UnknownWriteCommand2
        elif query.startswith('\x93\x05\x04') and query[5:7] == '\x01\x05':
            r = unpack_from('>xxxH', query)
            print 'UnknownWriteCommand2(size = 0x%04x)' % (r[0])
        # UnknownWriteCommand3
        elif query.startswith('\x93\x0d\x07'):
            print 'UnknownWriteCommand3()'
        else:
            print 'Unknown query: %s, returned %d' % (format_query(query, 15),
                    size)
        print '  %s, returned %d' % (format_query(query, 15), size)
    if len(response) > 0:
        print format_hex(response),
