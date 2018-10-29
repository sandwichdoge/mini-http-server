#!/bin/python2

#import cgi, cgitb
import sys

def get_field_value(raw, field_name):
    #change.py?msg=aaa&date=Sep20
    field_name += '='
    field_len = len(field_name)
    start_pos = raw.find(field_name) + field_len
    end_pos = raw[start_pos:].find('&')
    if (end_pos > 0):
        ret = raw[start_pos:end_pos]
    else:
        ret = raw[start_pos:]
    return ret


'''
#form = cgi.FieldStorage()

msg = form.getvalue('msg')
f = open('msg.txt', 'w')
f.write(msg)
f.close()
'''

raw = sys.argv[3] #request body
msg = get_field_value(raw, 'msg')
f = open('msg.txt', 'w+')
f.write(msg)
f.close()

print "HTTP/1.1 307 TEMPORARY REDIRECT"
print 'Location: /index.py'
print 'Content-Type: text/html; charset=utf-8\n'
print "<html>"
print "Updated MOTD.<br/>"
print '<a href="/index.py">Click here to go back</a>'
print "</html>"
