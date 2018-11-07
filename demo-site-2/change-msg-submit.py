#!/bin/python2

import cgi, cgitb
#import sys


form = cgi.FieldStorage()

msg = form.getvalue('msg')
f = open('msg.txt', 'w')
f.write(msg)
f.close()



print "HTTP/1.1 307 TEMPORARY REDIRECT"
#print "HTTP/1.1 200 OK"
print 'Location: /index.py'
print 'Content-Type: text/html; charset=utf-8\n'
print "<html>"
print form
print "Updated MOTD.<br/>"
print '<a href="/index.py">Click here to go back</a>'
print "</html>"
