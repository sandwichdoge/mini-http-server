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

print "Content-type: text/html\n"
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
print '<h>'
print "Message of the day: "
print content
print '</h>'
print '<a href="change-msg.py">Change</a>'
print '<br/><br/>'
print '<h>=Botnet=</h><br/>'
print '<a href="https://mail.google.com">Gmail</a><br/>'
print '<a href="https://www.youtube.com">Youtube</a><br/>'
print '<br/>'
print '<h>=Reddit=</h><br/>'
print '<a href="https://www.reddit.com/r/Warframe">r/Warframe</a><br/>'
print '<a href="https://www.reddit.com/r/Guildwars2">r/Guildwars2</a><br/>'
print '<br/>'
print '<h>=4chan=</h><br/>'
print '<a href="https://boards.4chan.org/g/catalog">/g/ - Technology</a><br/>'
print '<a href="https://boards.4chan.org/wg/catalog">/wg/ - Wallpapers</a><br/>'
print '<br/>'
print '<h>=News=</h><br/>'
print '<a href="https://vnexpress.net">VnExpress</a><br/>'
print '<br/>'
print '<h>=Github=</h><br/>'
print '<a href="https://github.com/sandwichdoge">Github</a><br/>'
print '</center>'
print "</html>\n"
