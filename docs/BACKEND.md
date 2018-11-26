When a client requests an executable resource from server:

- mini-http-server will look for appropriate interpreter associated with requested uri
from http.conf

- If none is found in config file, mini-http-server will parse the requested file and look for approriate 
interpreter which is specified at the start of requested script (e.g.: #!/bin/python)

- Then the interpreter program will be invoked in a new child process, these
parameters are passed down to the this interpreter program via env vars:
REQUEST_METHOD, HTTP_COOKIE, SCRIPT_URI, HTTP_ACCEPT, QUERY_STRING, CONTENT_LENGTH

- The output are whatever the interpreter prints to stdout. This data 
will be sent directly to the receiving client.

- CGI programming is supported.

- You have to parse data from multipart/form-data uploads yourself. The request body is 
piped with interpreter's STDIN