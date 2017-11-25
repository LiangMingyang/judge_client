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


posturl(12, 345)
    
