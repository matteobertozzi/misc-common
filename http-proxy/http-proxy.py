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

from threading import activeCount as thread_count
from thread import start_new_thread
from select import select
from sys import stdout, stderr

import socket
import sys

PROXY_METHOD_OTHERS = ('OPTIONS', 'GET', 'HEAD', 'POST', 'PUT', 'DELETE', 'TRACE')
PROXY_METHOD_CONNECT = ('CONNECT',)
RECV_BUFSIZE = 8192
TIMEOUT = 10

def log_info(data):
    stdout.write(data)
    stdout.flush()

def log_warn(data):
    stderr.write(data)
    stderr.flush()

def socket_connect_to_host(host, port):
    for sock_family, _, _, _, address in socket.getaddrinfo(host, port):
        try:
            sock = socket.socket(sock_family)
            sock.connect(address)
            log_info('Threads %2d - CONNECT TO %s %s:%d\n' % (thread_count(), address, host, port))
            return sock
        except:
            pass
    return None

def _proxy_run(sock, address):
    target = _proxy_handle_handshake(sock)
    if target is None:
        sock.close()
        return

    socks = (sock, target)
    wsocks = tuple()

    try:
        is_alive = True
        while is_alive:
            rlist, _, xlist = select(socks, wsocks, socks, TIMEOUT)
            if len(xlist) > 0 or len(rlist) == 0:
                is_alive = False
                break

            for rsock in rlist:
                data = rsock.recv(RECV_BUFSIZE)
                if not data:
                    is_alive = False
                    break

                out = target if rsock is sock else sock
                out.sendall(data)
    except Exception, e:
        log_warn('_proxy_run(): %s\n' % e)
    finally:
        target.close()
        sock.close()

def _proxy_handle_handshake(sock):
    target = None
    rbuffer = []
    while True:
        data = sock.recv(RECV_BUFSIZE)
        if not data:
            break

        index = data.find('\n')
        if index > 0:
            index += 1
            if len(rbuffer) > 0:
                rbuffer.append(data[:index])
                first_line = ''.join(rbuffer)
            else:
                first_line = data[:index]
            rbuffer = [data[index:]]

            # Parse the header and try to connect to the host
            method, path, protocol = first_line.split()
            target = _proxy_handle_method(sock, method, path, protocol, ''.join(rbuffer))
            break
        else:
            rbuffer.append(data)

    return target

def _proxy_handle_method(sock, method, path, protocol, rbuffer):
    if method in PROXY_METHOD_CONNECT:
        return _proxy_handle_method_connect(sock, path, protocol, rbuffer)
    if method in PROXY_METHOD_OTHERS:
        return _proxy_handle_method_others(method, path, protocol, rbuffer)
    return None

def _proxy_handle_method_connect(sock, path, protocol, rbuffer):
    target = _proxy_connect_to_host(path)
    if target is not None:
        sock.sendall('%s 200 Connection established\r\nProxy-agent: thz\r\n\r\n' % protocol)
    return target

def _proxy_handle_method_others(method, path, protocol, rbuffer):
    req_path = path[7:]
    index = req_path.find('/')
    host = req_path[:index]
    path = req_path[index:]

    target = _proxy_connect_to_host(host)
    if target is not None:
        target.sendall('%s %s %s\r\n%s' % (method, path, protocol, rbuffer))
    return target

def _proxy_connect_to_host(host):
    index = host.find(':')
    if index > 0:
        port = int(host[index + 1:])
        host = host[:index]
    else:
        port = 80

    return socket_connect_to_host(host, port)

def proxy_run(host, port):
    socksrv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    socksrv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    socksrv.bind((host, port))
    socksrv.listen(0)
    socksrv.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

    try:
        log_info('Proxy is running on %s:%d\n' % (host, port))
        while True:
            client = socksrv.accept()
            start_new_thread(_proxy_run, client)
    except KeyboardInterrupt:
        pass
    finally:
        socksrv.close()

if __name__ == '__main__':
    host = '0.0.0.0'
    port = 8080

    if len(sys.argv) >= 2:
        port = int(sys.argv[1])
    if len(sys.argv) >= 3:
        host = sys.argv[2]

    proxy_run(host, port)

