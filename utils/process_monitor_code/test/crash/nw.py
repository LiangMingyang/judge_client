import urllib2, urllib
import sys

def posturl(a, b):
    data = {'aa': 'bb'}
    url = 'http://localhost:8000/%d/%d/' % (a, b)
    try:
        print >>sys.stderr, 'url:', url
        req = urllib2.Request(url, urllib.urlencode(data))
        p = urllib2.urlopen(req)
#        print >>sys.stderr, p.read()
    except Exception, e:
        print >>sys.stderr, type(e), ':', e

while (True):
    try:
        s = raw_input()
    except:
        break
    if s.strip() == '': break;
    sa, sb = s.split(' ', 1)
    print >>sys.stderr, "high!!!"
    a = int(sa)
    b = int(sb)
    print (a + b) / (a - b)
    posturl(a, b)
    
