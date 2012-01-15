/*
 *   Copyright 2012 Matteo Bertozzi
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

import java.io.IOException;
import java.math.BigInteger;

public abstract class Readable {
  private byte[] buffer = new byte[8];

  public abstract int read(byte[] b, int off, int len) throws IOException;

  public int read(byte[] b) throws IOException {
    return(read(b, 0, b.length));
  }

  public int readFully(byte[] b, int off, int len) throws IOException {
    int n = 0;
    int rd;

    while (len > 0) {
      if ((rd = read(b, off, len)) < 0)
        break;
      len -= rd;
      off += rd;
      n += rd;
    }

    return(n);
  }

  public int readInt8() throws IOException {
    int v = readUInt8();
    return((v > 127) ? -(256 - v) : v);
  }

  public int readUInt8() throws IOException {
    readFully(buffer, 0, 1);
    return(buffer[0] & 0xff);
  }

  public int readInt16() throws IOException {
    int v = readUInt16();
    return((v > 32767) ? -(65536 - v) : v);
  }

  public int readUInt16() throws IOException {
    int v;

    readFully(buffer, 0, 2);
    v  = ((buffer[0] & 0xff) << 8);
    v += ((buffer[1] & 0xff));

    return(v);
  }

  public int readInt32() throws IOException {
    long v = readUInt32();
    return((int)((v > 2147483647) ? -(4294967296L - v) : v));
  }

  public long readUInt32() throws IOException {
    long v;

    readFully(buffer, 0, 4);
    v  = ((long)(buffer[0] & 0xff) << 24);
    v += ((long)(buffer[1] & 0xff) << 16);
    v += ((long)(buffer[2] & 0xff) << 8);
    v += ((long)(buffer[3] & 0xff));

    return(v);
  }

  public long readInt64() throws IOException {
    return(readUInt64().longValue());
  }

  public BigInteger readUInt64() throws IOException {
    long v;

    readFully(buffer, 0, 8);
    v  = ((long)(buffer[1] & 0xff) << 48);
    v += ((long)(buffer[2] & 0xff) << 40);
    v += ((long)(buffer[3] & 0xff) << 32);
    v += ((long)(buffer[4] & 0xff) << 24);
    v += ((long)(buffer[5] & 0xff) << 16);
    v += ((long)(buffer[6] & 0xff) << 8);
    v += ((long)(buffer[7] & 0xff));

    return(BigInteger.valueOf(buffer[0] & 0xff).shiftLeft(56).add(BigInteger.valueOf(v)));
  }

  public long readVInt() throws IOException {
    BigInteger a = readVUInt();
    BigInteger b = a.and(BigInteger.valueOf(1)).negate();
    return(a.shiftRight(1).xor(b).longValue());
  }

  public BigInteger readVUInt() throws IOException {
    BigInteger result = BigInteger.ZERO;
    int shift = 0;
    int b;
    while (true) {
      b = readUInt8();
      result = result.or(BigInteger.valueOf(b & 0x7f).shiftLeft(shift));
      if ((b & 0x80) == 0)
        break;
      shift += 7;
    }
    return(result);
  }
}

