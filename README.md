# mini-http-server
Minimal HTTP server in C with SSL and backend scripting support.


# Compile
Requirements: OpenSSL, OpenSSL-dev
```
make
make config #generate http.conf file
make ssl-ca #if you want a new keypair for https
make clean
```

# Configure
Edit 'http.conf' file.

# Run
```
sudo ./start-server
```
