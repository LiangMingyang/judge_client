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
data_root           = '/usr/share/oj4th'
storage_root        = '/usr/share/oj4th/storage/%s/'

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
        self.id = int(id)
        self.tmpfs_size = int(tmpfs_size)
        self.tmp_dir    = os.path.join(data_root, '%s_%d' % (tmp_dirname_prefix,  self.id))
        self.root       = os.path.join(data_root, '%s_%d' % (work_dirname_prefix, self.id))

    def mount(self):
        os.system('mkdir -p %s' % self.tmp_dir)
        os.system('mkdir -p %s' % self.root)
        os.system('mount -t tmpfs judge_tmpfs_%d %s -o size=%dM' % (self.id, self.tmp_dir, self.tmpfs_size))
        os.system('mount -t aufs  judge_root_%d  %s -o br:%s:/=ro'  % (self.id, self.root, self.tmp_dir))
        os.system('mount -t proc  judge_proc_%d  %s/proc' % (self.id, self.root))

    def umount(self):
        os.system('umount -f judge_proc_%d judge_root_%d judge_tmpfs_%d'  % (self.id, self.id, self.id))
        os.system('rm -Rf %s' % self.tmp_dir)
        os.system('rm -Rf %s' % self.root)

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

################################################################################
# Judge
################################################################################

submission_dirname  = 'submission'
data_dirname        = 'data'

class Judge:
    def __init__(self, id, tmpfs_size, cpu_mask, *args, **kwargs):
        self.id = id
        self.cpu_mask = cpu_mask
        self.fs = Work_Station(id, tmpfs_size)

    def __child_preexec(self):
        os.chroot(self.run_cwd)
        os.chdir('/')

    def judge(self):
        self.run_cwd = os.path.abspath(self.fs.root)
        cpu_mask = self.cpu_mask
        command = ['judge_cpu_limiter', cpu_mask, '/__judge__']
        env = os.environ.copy()
        env.pop('LANG', None)
        return subprocess.call(command, preexec_fn = self.__child_preexec,
            cwd = self.run_cwd, env = env, close_fds = True, stderr = file('/dev/null'))

    def prepare_submission(self, source_code, source_lang, test_setting):
        self.fs.new_file(('%s/__main__' % submission_dirname), source_code)
        self.fs.new_file(('%s/__lang__' % submission_dirname), source_lang)
        submission_dir = os.path.join(self.fs.root, submission_dirname)
        os.system('chown %s:%s %s' % (TEST_MASTER, TEST_MASTER, submission_dir))
        os.system('chmod o= %s' % submission_dir)
        data_dir = os.path.join(self.fs.root, data_dirname)
        os.system('mkdir %s -p' % data_dir)
        write_file('%s/__setting_code__' % data_dir, test_setting)
    
    def prepare_res(self, res_path):
        data_dir = os.path.join(self.fs.root, data_dirname)
        os.system('mkdir %s -p' % data_dir)
        os.system('tar -xzvf %s -C %s' % (res_path, data_dir) )
        os.system('chown %s:%s %s' % (TEST_MASTER, TEST_MASTER, data_dir))
        os.system('chmod o= %s' % data_dir) 
    
    def prepare_test_script(self, test_script_path):
        fr = open(test_script_path,"r")
        self.fs.new_file('__judge__', fr.read(), own='0', own_group='0', umask='u=rwx,g=rx,o=')
        fr.close()


if __name__ == '__main__':
    opt = sys.argv[1]
    id = sys.argv[2]
    tmpfs_size = sys.argv[3]
    cpu_mask = sys.argv[4]
    judge = Judge(id, tmpfs_size, cpu_mask)
    if opt == 'mount':
        judge.fs.mount()
    elif opt == 'clean_all':
        judge.fs.clean_all()
    elif opt == 'umount':
        judge.fs.umount()
    elif opt == 'judge':
        judge.judge()
    elif opt == 'prepare':
        source_code = sys.argv[5]
        source_lang = sys.argv[6]
        test_setting = sys.argv[7]
        test_script_path = sys.argv[8]
        judge.prepare_submission(source_code, source_lang, test_setting)
        judge.prepare_test_script(test_script_path)
    elif opt == 'resource':
        res_path = sys.argv[5]
        judge.prepare_res(res_path)
        
        
	
