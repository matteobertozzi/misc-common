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

from argparse import ArgumentParser
from time import sleep

import libtorrent

import sys
import os

def _human_size(size):
    if size >= (1 << 40): return '%.2fTiB' % (float(size) / (1 << 40))
    if size >= (1 << 30): return '%.2fGiB' % (float(size) / (1 << 30))
    if size >= (1 << 20): return '%.2fMiB' % (float(size) / (1 << 20))
    if size >= (1 << 10): return '%.2fKiB' % (float(size) / (1 << 10))
    return '%d' % int(size)

def _print_line(data):
    sys.stdout.write('\b' * 80 + ' ' * 79 + '\b' * 80)
    sys.stdout.write(data)
    sys.stdout.flush()

def _print_status(status):
    states = ['queued', 'checking', 'downloading metadata',
               'downloading', 'finished', 'seeding', 'allocating']

    state = '%s' % status.state
    if isinstance(state, int):
        state = states[state]

    _print_line('%6.2f%% (down: %s/s up: %s/s peers: %d ann: %s) %s' % \
                (status.progress * 100.0, \
                _human_size(status.download_rate), \
                _human_size(status.upload_rate), \
                status.num_peers, status.next_announce, state))

def _parse_opts():
    parser = ArgumentParser()
    parser.add_argument('-e', '--force-encryption', dest='encryption', action='store_true',
                        help='Force RC4 encryption for peer I/O.')
    parser.add_argument('-s', '--seed', dest='seed', action='store_true',
                        help="Don't seed after finish if this is not specified.")
    parser.add_argument('-f', '--finish-script', dest='finish_script', action='store',
                        help='Run specified script when download is finished.')
    parser.add_argument('-d', '--download-dir', dest='download_dir', action='store',
                        default='.',
                        help='Download directory path.')
    parser.add_argument('-p', '--port', dest='port', action='store',
                        default=6881, type=int,
                        help='Port.')
    parser.add_argument('-D', '--download-limit', dest='download_limit', action='store',
                        default=0, type=int,
                        help='Download limit in KiB/sec.')
    parser.add_argument('-U', '--upload-limit', dest='upload_limit', action='store',
                        default=0, type=int,
                        help='Upload limit in KiB/sec.')
    parser.add_argument('torrent', metavar='TORRENT',
                         help='Torrent files')
    options = parser.parse_args()
    return options

if __name__ == '__main__':
    options = _parse_opts()

    if not os.path.exists(options.torrent):
        sys.stderr.write("File '%s' does not exists!\n" % options.torrent)
        sys.exit(1)

    # Initialize Session
    session = libtorrent.session()
    settings = libtorrent.session_settings()
    settings.min_announce_interval = 15

    if options.download_limit > 0 or options.upload_limit > 0:
        if options.download_limit > 0:
            session.set_download_rate_limit(options.download_limit << 10)
        if options.upload_limit > 0:
            session.set_upload_rate_limit(options.upload_limit << 10)

        settings.ignore_limits_on_local_network = False

    # Force Encryption
    if options.encryption:
        pe_settings = libtorrent.pe_settings()
        pe_settings.out_enc_policy = libtorrent.enc_policy.forced
        pe_settings.in_enc_policy = libtorrent.enc_policy.forced
        pe_settings.allowed_enc_level = libtorrent.enc_level.rc4
        pe_settings.prefer_rc4 = True
        session.set_pe_settings(pe_settings)

    session.set_settings(settings)
    session.listen_on(options.port, options.port + 10)

    # Start torrents
    info = libtorrent.torrent_info(libtorrent.bdecode(file(options.torrent).read()))
    torrent_parms = {'ti': info}
    torrent_parms["save_path"] = options.download_dir
    torrent_parms["storage_mode"] = libtorrent.storage_mode_t.storage_mode_sparse
    torrent_parms["paused"] = False
    torrent_parms["auto_managed"] = True
    torrent = session.add_torrent(torrent_parms)

    try:
        # Loop until download is finished
        while not torrent.is_seed():
            _print_status(torrent.status())
            sleep(1)

        _print_line("Torrent complete! %s\n" % options.torrent)
        if options.finish_script:
            env = {'TORRENT_NAME': options.torrent,
                   'TORRENT_DIR': os.path.abspath(options.download_dir)}
            os.spawnvpe(os.P_NOWAIT, options.finish_script, [options.finish_script], env)

        # Keep running if seed flag is specified
        while options.seed:
            _print_status(torrent.status())
            sleep(1)
    except KeyboardInterrupt:
        _print_line("Shutdown! %s\n" % options.torrent)

    # Shutdown the session
    session.remove_torrent(torrent)
    del session
    sleep(3)
