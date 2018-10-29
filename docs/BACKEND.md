When a client requests an executable resource from server:

- mini-http-server will parse the requested file and look for approriate 
interpreter (e.g.: #!/bin/python which is specified at the start of requested script)

- Then the interpreter program will be invoked in a new child process, these
parameters are passed down to the this interpreter program: method, rawURI, body, cookie.

e.g.: /bin/python /var/www/mywebsite/get-user.py POST /index.py name=Tyrone&age=19 "id=125; cart=rope,mask"

- The output are whatever the interpreter prints to stdout. This data 
will be sent to client.
