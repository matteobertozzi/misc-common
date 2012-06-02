#!/usr/bin/env python
#
# Copyright (c) 2012, Matteo Bertozzi
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#   * Neither the name of the <organization> nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
from socket import getfqdn, gethostname, gethostbyname
from collections import defaultdict
from urllib import unquote_plus
from urlparse import parse_qsl
from datetime import datetime
from binascii import hexlify
from time import time

from bencode import bencode

import sys

# You can tune this table to have
# country -> rack -> machines level
IP_SORT_ORDER = [
    (0, '31.193.'),
    (1, '37.59.'),
    (2, '216.65.'),
    (3, '62.133.'),
    (5, '216.94.'),
]

def _ip_code(ip, port):
    code = port
    shift = 16
    for v in ip.split('.'):
        code += int(v) << shift
        shift += 8
    return code

def _ip_location_index(ip):
    index = 0
    for w, addr in IP_SORT_ORDER:
        if ip.startswith(addr):
            return w
        if w > index:
            index = w
    return index << 1

def _ip_sort_by_location_key(myhost, host):
    return (abs(host.location - myhost.location), host.code)

def _ip_sort_by_location(myhost, hosts):
    return sorted(hosts, key=lambda h,mh=myhost: _ip_sort_by_location_key(mh, h))

class Host(object):
    __slots__ = ('ip', 'port', 'code', 'location', 'last_update', 'files')

    def __init__(self, ip, port=0, code=None):
        if code is None: code = _ip_code(ip, port)
        self.ip = ip
        self.port = port
        self.code = code
        self.location = _ip_location_index(ip)
        self.last_update = time()
        self.files = set()

    def add_file(self, info_hash):
        self.files.add(info_hash)
        self.last_update = time()

    def __hash__(self):
        return self.code

    def __eq__(self, host):
        return self.ip == host[0] and self.port == host[1]

    def __repr__(self):
        return 'Host(ip=%s, port=%d)' % (self.ip, self.port)

class FileStat(object):
    __slots__ = ('uploaded', 'downloaded', 'left', 'event', 'last_update')

    def __init__(self, uploaded, downloaded, left, event):
        self.update(uploaded, downloaded, left, event)

    def update(self, uploaded, downloaded, left, event):
        self.uploaded = uploaded
        self.downloaded = downloaded
        self.left = left
        self.event = event
        self.last_update = time()

    def is_finished(self):
        return self.event == 'completed' or self.left == 0


class TrackerError(Exception):
    def __init__(self, code, message):
        self.code = code
        super(TrackerError, self).__init__(message)

class Tracker(object):
    REQUEST_INTERVAL = 15

    def __init__(self):
        self.files = defaultdict(dict)
        self.hosts = {}

    def add(self, ip, port, info_hash, uploaded, downloaded, left, event):
        host_code = _ip_code(ip, port)

        # Lookup host
        host_info = self.hosts.get(host_code)
        if host_info is None:
            host_info = Host(ip, port, host_code)
            self.hosts[host_code] = host_info

        # Add info_hash to host
        host_info.add_file(info_hash)

        # Add File Stat to the tracker
        file_peers = self.files[info_hash]
        file_stat = file_peers.get(host_code)
        if file_stat is None:
            file_peers[host_code] = FileStat(uploaded, downloaded, left, event)
        else:
            file_stat.update(uploaded, downloaded, left, event)

        return host_info

    def remove(self, ip, port):
        host_code = _ip_code(ip, port)
        host_info = self.hosts.pop(host_code, None)
        if host_info is None:
            return Host(ip, port, host_code)

        for hash_info in host_info.files:
            peer_files = self.files[hash_info]
            if len(peer_files) < 2:
                del self.files[hash_info]
            else:
                del peer_files[host_code]

    def get_peers(self, myhost, info_hash):
        file_peers = self.files.get(info_hash, None)
        if file_peers is None:
            return

        # retrieve max download/upload/left
        d = u = l = 1
        for st in file_peers.itervalues():
            if l < st.left: l = st.left
            if u < st.uploaded: u = st.uploaded
            if d < st.downloaded: d = st.downloaded
        d = float(d)
        u = float(u)
        l = float(l)

        # Sort by:
        # - completed/left
        # - machine location
        # - upload/download
        def _fstat_key(item):
            h, st = item
            h = self.hosts[h]
            hlocation = abs(h.location - myhost.location)
            lft = int((st.left / l) * 10)
            upl = int((st.uploaded / u) * 10)
            dwl = int((st.downloaded / d) * 10)
            return (not st.is_finished(), lft, hlocation, h.code, upl, dwl)

        for hcode, file_stat in sorted(file_peers.iteritems(), key=_fstat_key):
            if hcode != myhost.code:
                yield self.hosts[hcode], file_stat

def tracker_announce(tracker, request):
    query = dict(request.query_arguments())

    # Peer Info
    peer_id = query.get('peer_id')
    if peer_id is None: raise TrackerError(102, 'missing peer_id')
    if len(peer_id) != 20: raise TrackerError(151, 'peer_id is not 20bytes long')

    port = int(query.get('port'))
    if port is None: raise TrackerError(103, 'missing port')
    ip = query.get('ip') or request.ip_address()

    # Torrent Info
    info_hash = query.get('info_hash')
    if info_hash is None: raise TrackerError(101, 'missing info_hash')
    if len(info_hash) != 20: raise TrackerError(150, 'info_hash is not 20bytes long')
    uploaded = int(query.get('uploaded', 0))
    downloaded = int(query.get('downloaded', 0))
    left = int(query.get('left', 0))
    event = query.get('event')
    if event is not None: event = event.lower()

    # Request
    numwant = int(query.get('numwant', 80))

    # Add information to the tracker
    if event == 'stopped':
        host = tracker.remove(ip, port)
    else:
        host = tracker.add(ip, port, info_hash, uploaded, downloaded, left, event)

    # Pack peers
    peers = []
    completed = 0
    for peer, st in tracker.get_peers(host, info_hash):
        if numwant == 0: break
        peers.append({'ip': peer.ip, 'port': peer.port})
        completed += int(st.is_finished())
        numwant -= 1

    # Send response!
    request.send_response(200)
    request.send_header("content-type", 'text/plain')
    request.end_headers()
    request.wfile.write(bencode({'interval': tracker.REQUEST_INTERVAL,
                                 'complete': completed,
                                 'incomplete': len(peers) - completed,
                                 'peers': peers}))

def tracker_scrape(tracker, request):
    query = request.query_arguments()

    response = {}
    for k, info_hash in query:
        if k != 'info_hash':
            continue

        # Get information about the file
        completed = 0
        downloaded = 0
        incomplete = 0
        file_peers = tracker.files.get(info_hash, None)
        if file_peers is not None:
            for st in file_peers.itervalues():
                if st.left == 0:
                    completed += 1
                if st.event == 'completed':
                    downloaded += 1
                elif st.left != 0:
                    incomplete += 1

        response[info_hash] = info = {}
        info['complete'] = completed
        info['downloaded'] = downloaded
        info['incomplete'] = incomplete

    # Send response!
    request.send_response(200)
    request.send_header("content-type", 'text/plain')
    request.end_headers()
    request.wfile.write(bencode(response))

def human_size(size):
    if size >= (1 << 40): return '%.3fTiB' % (float(size) / (1 << 40))
    if size >= (1 << 30): return '%.3fGiB' % (float(size) / (1 << 30))
    if size >= (1 << 20): return '%.3fMiB' % (float(size) / (1 << 20))
    if size >= (1 << 10): return '%.3fKiB' % (float(size) / (1 << 10))
    return size

class HtmlPage(object):
    def __init__(self, stream, title):
        self.stream = stream
        self.stream.write('<html><head>')
        self.stream.write('<title>%s</title>' % title)
        self.stream.write('<style type="text/css">body,table{font: 10px "Lucida Grande", "Lucida Sans Unicode", Helvetica, Arial, Verdana, sans-serif;}table, td, th, tr{border-collapse:collapse;border: 1px solid #ccc;}td,th{padding: 2px 6px 2px 6px;}tr th{background: #eee;}</style>')
        self.stream.write('<head><body>')

    def write_title(self, title):
        self.stream.write('<h1>%s</h1>' % title)

    def write_paragraph(self, text):
        self.stream.write('<p>%s</p>' % text)

    def close(self):
        self.stream.write('</body></html>')

class HtmlTable(object):
    def __init__(self, stream):
        self.stream = stream
        self.stream.write('<table>')

    def close(self):
        self.stream.write('</table>')

    def write_header(self, *columns):
        self.stream.write('<tr>')
        for c in columns:
            self.stream.write('<th>%s</th>' % c)
        self.stream.write('</tr>')

    def write_row(self, *columns):
        self.stream.write('<tr>')
        for c in columns:
            self.stream.write('<td>%s</td>' % c)
        self.stream.write('</tr>')

def tracker_info(tracker, request):
    request.send_response(200)
    request.end_headers()

    ts2date = lambda ts: datetime.fromtimestamp(ts).ctime()
    def _host_name(host):
        name = getfqdn(host.ip)
        if name != host.ip:
            name += ' (%s)' % host.ip
        return name

    _, port = request.server.server_address
    page = HtmlPage(request.wfile, 'Tracker Info')
    page.write_title('Tracker Info')
    page.write_paragraph('Tracker running on %s (%s:%d)' % (gethostname(), gethostbyname(gethostname()), port))

    # Peers
    page.write_paragraph('%d Connected peers' % len(tracker.hosts))
    table = HtmlTable(request.wfile)
    table.write_header('ip', 'port', 'last update')
    for host in sorted(tracker.hosts.itervalues(), key=lambda h: (h.code, h.last_update)):
        table.write_row(_host_name(host), host.port, ts2date(host.last_update))
    table.close()

    # Files
    page.write_paragraph('%d Files and peers information' % len(tracker.files))
    table = HtmlTable(request.wfile)
    table.write_header('info hash', 'host', 'port',
                       'uploaded', 'downloaded', 'left',
                       'event', 'last update')
    for info_hash, peer_info in tracker.files.iteritems():
        info_hash = hexlify(info_hash)
        for host_code, st in peer_info.iteritems():
            host = tracker.hosts[host_code]
            table.write_row(info_hash, _host_name(host), host.port,
                            human_size(st.uploaded),
                            human_size(st.downloaded),
                            human_size(st.left),
                            st.event, ts2date(st.last_update))
            info_hash = ''
    table.close()

    page.close()

class TrackerHttpHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        try:
            if self.path.startswith('/announce'):
                tracker_announce(self.server.tracker, self)
            elif self.path.startswith('/scrape'):
                tracker_scrape(self.server.tracker, self)
            else:
                tracker_info(self.server.tracker, self)
        except TrackerError, e:
            self.send_error(e.code, e.message)
        except Exception, e:
            print e
            self.send_response(900)

    def query_arguments(self):
        query_index = self.path.find('?')
        if query_index >= 0:
            query = parse_qsl(self.path[query_index+1:])
            query = [(k, unquote_plus(v)) for k, v in query]
        else:
            query = []
        return query

    def ip_address(self):
        return self.client_address[0]

class TrackerHttpServer(HTTPServer):
    def __init__(self, tracker, address):
        self.tracker = tracker
        HTTPServer.__init__(self, address, TrackerHttpHandler)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print >> sys.stderr, 'Usage: tracker <port>'
        sys.exit(1)

    server = None
    try:
        tracker = Tracker()
        server = TrackerHttpServer(tracker, ('', int(sys.argv[1])))
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        if server is not None:
            server.shutdown()

