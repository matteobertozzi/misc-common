#!/usr/bin/env python
#
#   Copyright 2012 Matteo Bertozzi
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from struct import unpack as binary_unpack
from struct import pack as binary_pack
from cStringIO import StringIO
import io

class Readable:
    def read(self, data):
        raise NotImplementedError

    def readInt8(self):
        return next(iter(binary_unpack('>b', self.read(1))))

    def readUInt8(self):
        return next(iter(binary_unpack('>B', self.read(1))))

    def readInt16(self):
        return next(iter(binary_unpack('>h', self.read(2))))

    def readUInt16(self):
        return next(iter(binary_unpack('>H', self.read(2))))

    def readInt32(self):
        return next(iter(binary_unpack('>l', self.read(4))))

    def readUInt32(self):
        return next(iter(binary_unpack('>L', self.read(4))))

    def readInt64(self):
        return next(iter(binary_unpack('>q', self.read(8))))

    def readUInt64(self):
        return next(iter(binary_unpack('>Q', self.read(8))))

    def readVInt(self):
        # Signed int uses zig-zag encoding.
        n = self.readVUInt()
        return ((n >> 1) ^ -(n & 1))

    def readVUInt(self):
        shift = 0
        result = 0
        while True:
            b = self.readUInt8()
            result |= ((b & 0x7f) << shift)
            if not (b & 0x80):
                break
            shift += 7
        return result

    def readVData(self):
        length = self.readVUInt()
        return self.read(length)

class Writable:
    def write(self, data):
        raise NotImplemented

    def writeInt8(self, value):
        return self.write(binary_pack('>b', value))

    def writeUInt8(self, value):
        return self.write(binary_pack('>B', value))

    def writeInt16(self, value):
        return self.write(binary_pack('>h', value))

    def writeUInt16(self, value):
        return self.write(binary_pack('>H', value))

    def writeInt32(self, value):
        return self.write(binary_pack('>l', value))

    def writeUInt32(self, value):
        return self.write(binary_pack('>L', value))

    def writeInt64(self, value):
        return self.write(binary_pack('>q', value))

    def writeUInt64(self, value):
        return self.write(binary_pack('>Q', value))

    def writeVInt(self, value):
        # Use zig-zag encoding...
        return self.writeVUInt((value << 1) ^ (value >> 63))

    def writeVUInt(self, value):
        length = 1
        while value >= 0x80:
            self.writeUInt8((value & 0x7f) | 0x80)
            length += 1
            value >>= 7
        self.writeUInt8(value)
        return length

    def writeVData(self, data):
        n = self.writeVUInt(len(data))
        n += self.write(data)
        return n

class DataStream(object):
    def __init__(self, stream):
        assert isinstance(stream, (io.BufferedIOBase, StringIO))
        self.stream = stream

    @classmethod
    def open(cls, filename, mode, buffering=-1):
        return cls(io.open(filename, mode, buffering))

    def close(self):
        return self.stream.close()

    def flush(self):
        return self.stream.flush()

    def seek(self, offset, whence=io.SEEK_SET):
        assert self.stream.seekable()
        return self.stream.seek(offset, whence)

    def skip(self, length):
        return self.seek(self.tell() + length)

    def tell(self):
        return self.stream.tell()

    def length(self):
        origin = self.tell()
        length = self.seek(0, io.SEEK_END)
        self.seek(origin, io.SEEK_SET)
        return length

    def read(self, length):
        return self.stream.read(length)

    def write(self, data):
        return self.stream.write(data)

class BinaryDataStream(DataStream, Readable, Writable):
    """
        BinaryStream(io.open('test.dat', 'wb'))
        BinaryStream(gzip.open('test.dat', 'wb'))
        BinaryStream(bz2.open('test.dat', 'wb'))
    """
    pass

if __name__ == '__main__':
    def _matchValue(readed, expected):
        if readed != expected:
            raise ValueError("readed %d != expected %d" % (readed, expected))

    with io.open('test.stream', 'w+b') as fd:
        stream = BinaryDataStream(fd)
        print stream.writeInt8(- (2 ** 7 - 1))
        print stream.writeUInt8(2 ** 8 - 1)
        print stream.writeInt16(-(2 ** 15 - 1))
        print stream.writeUInt16(2 ** 16 - 1)
        print stream.writeInt32(-(2 ** 31 - 1))
        print stream.writeUInt32(2 ** 32 - 1)
        print stream.writeInt64(-(2 ** 63 - 1))
        print stream.writeUInt64(2 ** 64 - 1)
        print stream.writeVInt(-(2 ** 63 - 1))
        print stream.writeVUInt(2 ** 64 - 1)

        stream.seek(0)
        _matchValue(stream.readInt8(), -(2 ** 7 - 1))
        _matchValue(stream.readUInt8(), (2 ** 8 - 1))
        _matchValue(stream.readInt16(), -(2 ** 15 - 1))
        _matchValue(stream.readUInt16(), (2 ** 16 - 1))
        _matchValue(stream.readInt32(), -(2 ** 31 - 1))
        _matchValue(stream.readUInt32(), (2 ** 32 - 1))
        _matchValue(stream.readInt64(), -(2 ** 63 - 1))
        _matchValue(stream.readUInt64(), (2 ** 64 - 1))
        _matchValue(stream.readVInt(), -(2 ** 63 - 1))
        _matchValue(stream.readVUInt(), (2 ** 64 - 1))

