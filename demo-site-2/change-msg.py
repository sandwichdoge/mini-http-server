#!/bin/python2

def readMsgOfTheDay():
	try:
		f = open("msg.txt", "r")
		content = f.read()
		f.close()
	except IOError:
		content = "No message file detected!"
	return content


content = readMsgOfTheDay()

print "Content-Type: text/html;\n\n"
print "<html>"
print '<br/>'
print '<style>'
print 'body {'
print 'background-color:#323234;'
print '<!--background-image: url("aa.jpg");-->'
print '<!--background-size: cover;-->'
print 'color:#C0C0C0;'
print '}'
print ''
print 'h {'
print 'color:#ffffff'
print '}'
print ''
print 'a:link {'
print 'color:#80bfff'
print '}'
print 'a:visited {'
print 'color:#80bfff'
print '}'
print '</style>'
print ''
print '<center>'
print '<form action = "/change-msg-submit.py" method = "post">'
print 'New Message: <input type = "text" name = "msg">'
print '<input type = "submit" value = "Update"/>'
print '</form><br/>'
print '</center>'
print "</html>\n"
