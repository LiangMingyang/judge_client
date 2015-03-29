#!/usr/bin/python
# coding: utf8
import re
import os
import sys
import signal
import multiprocessing
import time
import datetime
import hashlib
import urllib
import urllib2
import random
import subprocess

################################################################################
# Work_Station
################################################################################


TEST_MASTER = '8193'
tmp_dirname_prefix  = 'tmp'
work_dirname_prefix = 'work'
default_tmpfs_size  = 500 #MB

def write_file(filepath, content, own = TEST_MASTER, own_group = TEST_MASTER, umask = 'u=rw,g=r,o='):
    dirname = os.path.dirname(filepath)
    if not os.path.exists(dirname): os.system('mkdir -p %s' % dirname)
    f = file(filepath, 'w')
    content = re.sub(r'\r\n|\r|\n', '\n', content) # normalize newlines
    f.write(content)
    f.close()
    os.system('chmod %s %s' % (umask, filepath))
    if own != '' or own_group != '':
        os.system('chown %s%s %s' % (own, (':' + own_group if own_group != '' else ''), filepath))

class Work_Station:
    def __init__(self, id, tmpfs_size = default_tmpfs_size, *args, **kwargs):
        self.id = id
        self.tmpfs_size = tmpfs_size
        self.tmp_dir    = os.path.join(data_root, '%s_%d' % (tmp_dirname_prefix,  self.id))
        self.root       = os.path.join(data_root, '%s_%d' % (work_dirname_prefix, self.id))

    def mount(self):
        os.system('mkdir -p %s' % self.tmp_dir)
        os.system('mkdir -p %s' % self.root)
        os.system('mount -t tmpfs judge_tmpfs_%d %s -o size=%dM' % (self.id, self.tmp_dir, self.tmpfs_size))
        os.system('mount -t aufs  judge_root_%d  %s -o br:%s:/=ro'  % (self.id, self.root, self.tmp_dir))
        os.system('mount -t proc  judge_proc_%d  %s/proc' % (self.id, self.root))

    def umount(self):
        os.system('umount judge_proc_%d'  % self.id)
        os.system('umount judge_root_%d'  % self.id)
        os.system('umount judge_tmpfs_%d' % self.id)
        os.system('rm -r -f %s' % self.tmp_dir)
        os.system('rm -r -f %s' % self.root)

    def new_file(self, filename, content, own = TEST_MASTER, own_group = TEST_MASTER, umask = 'u=rw,g=r,o='):
        filepath = os.path.join(self.root, filename)
        write_file(filepath, content, own, own_group, umask)

    def read_file(self, filename):
        filepath = os.path.join(self.root, filename)
        try:
            f = file(filepath, 'r')
            ret = f.read()
            f.close()
        except Exception, e:
            ret = ''
        return ret

    def clean_all(self):
        os.system('rm %s/* -r -f' % self.tmp_dir)

if __name__ == '__main__':
    if sys.argv[1] == 'mount':
        id = sys.argv[2]
        tmpfs_size = sys.argv[3]
        ws = WorkStation(id, tmpfs_size)
        ws.mount()
    elif sys.argv[1] == 'clean_all':
        id = sys.argv[2]
        tmpfs_size = sys.argv[3]
        ws = WorkStation(id, tmpfs_size)
        ws.clean_all()
    elif sys.argv[1] == 'unmount':
        id = sys.argv[2]
        tmpfs_size = sys.argv[3]
        ws = WorkStation(id, tmpfs_size)
        ws.unmount()
        
	
