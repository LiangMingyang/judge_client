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

data_dirname = "data"
submission_dirname = "submission"
utils_dirname = "utils"

if __name__ == '__main__':
    id = sys.argv[1]
    memory_limit = sys.argv[2] #单位M
    cpu_set = sys.argv[3]      #cpu的限制
    utils_path = sys.argv[4]   #utils,必要的库
    work_path = sys.argv[5]    #work的主要路径
    file_path = sys.argv[6]    #文件的路径

    dirname = os.path.join(work_path, data_path)
    if not os.path.exists(dirname): os.system('mkdir -p %s' % dirname)
    os.system('tar -xzvf %s -C %s > /dev/null' % (dirname, file_path) )

    os.system('chmod -R u=rwx,g=rwx,o= %s', work_path)


    opt = ''
    opt += '-i '
    opt += '--rm '
    opt += '-m %sM --memory-swap %sM ' % (memory_limit, memory_limit)
    opt += '--ulimit nproc=32:64 '
    opt += '--cpuset-cpus="%s"' % cpu_set
    opt += '-v %s:%s:ro' % ( os.path.join(work_path, data_dirname), os.path.join('/', data_dirname) )
    opt += '-v %s:%s:ro' % ( os.path.join(work_path, submission_dirname), os.path.join('/', submission_dirname) )
    opt += '-v %s:%s:ro' % ( utils_path, os.path.join('/', utils_dirname) )

    cmd = 'sh /utils/run.sh'

    print 'docker run %s oj4th/judge_container %s' % (opt,cmd)
    #os.system('docker run %s oj4th/judge_container %s' % (opt,cmd))


