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

public abstract class Writable {
  private byte[] buffer = new byte[11];

  public abstract void flush() throws IOException;
  public abstract void write(byte[] data, int off, int len) throws IOException;

  public void write(byte[] data) throws IOException {
    write(data, 0, data.length);
  }

  public void writeInt8(int v) throws IOException {
    writeUInt8(v);
  }

  public void writeUInt8(int v) throws IOException {
    buffer[0] = (byte)(v & 0xff);
    write(buffer, 0, 1);
  }

  public void writeInt16(int v) throws IOException {
    writeUInt16(v);
  }

  public void writeUInt16(int v) throws IOException {
    buffer[0] = (byte)(0xff & (v >>> 8));
    buffer[1] = (byte)(0xff & v);
    write(buffer, 0, 2);
  }

  public void writeInt32(int v) throws IOException {
    writeUInt32(v);
  }

  public void writeUInt32(int v) throws IOException {
    buffer[0] = (byte)(0xff & (v >>> 24));
    buffer[1] = (byte)(0xff & (v >>> 16));
    buffer[2] = (byte)(0xff & (v >>> 8));
    buffer[3] = (byte)(0xff & v);
    write(buffer, 0, 4);
  }

  public void writeInt64(long v) throws IOException {
    buffer[0] = (byte)(0xff & (v >>> 56));
    buffer[1] = (byte)(0xff & (v >>> 48));
    buffer[2] = (byte)(0xff & (v >>> 40));
    buffer[3] = (byte)(0xff & (v >>> 32));
    buffer[4] = (byte)(0xff & (v >>> 24));
    buffer[5] = (byte)(0xff & (v >>> 16));
    buffer[6] = (byte)(0xff & (v >>> 8));
    buffer[7] = (byte)(0xff & v);
    write(buffer, 0, 8);
  }

  public void writeUInt64(BigInteger v) throws IOException {
    BigInteger bff = BigInteger.valueOf(255);
    buffer[0] = (byte)(v.shiftRight(56).and(bff).longValue());
    buffer[1] = (byte)(v.shiftRight(48).and(bff).longValue());
    buffer[2] = (byte)(v.shiftRight(40).and(bff).longValue());
    buffer[3] = (byte)(v.shiftRight(32).and(bff).longValue());
    buffer[4] = (byte)(v.shiftRight(24).and(bff).longValue());
    buffer[5] = (byte)(v.shiftRight(16).and(bff).longValue());
    buffer[6] = (byte)(v.shiftRight(8).and(bff).longValue());
    buffer[7] = (byte)v.and(bff).longValue();
    write(buffer, 0, 8);
  }

  public void writeVInt(long v) throws IOException {
      writeVUInt(BigInteger.valueOf(v).shiftLeft(1).xor(BigInteger.valueOf(v >> 63)));
  }

  public void writeVUInt(long v) throws IOException {
    int n = 0;
    while (v >= 0x80) {
      buffer[n++] = (byte)((v & 0x7f) | 0x80);
      v >>>= 7;
    }
    buffer[n++] = (byte)v;
    write(buffer, 0, n);
  }

  public void writeVUInt(BigInteger v) throws IOException {
    BigInteger b128 = BigInteger.valueOf(128);
    BigInteger b127 = BigInteger.valueOf(127);
    int n = 0;
    while (v.compareTo(b128) >= 0) {
      buffer[n++] = (byte)(v.and(b127).longValue() | 0x80);
      v = v.shiftRight(7);
    }
    buffer[n++] = (byte)v.longValue();
    write(buffer, 0, n);
  }
}

