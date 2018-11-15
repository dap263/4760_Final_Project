import socket

# base code taken from CS 4410 lecture with Robbert Van Renesse
skt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
skt.bind(('',5000))
skt.listen(5)

while True:
    cltskt, addr = skt.accept()
    msg = cltskt.recv(100)
    print("got '%s' from %s" % (msg, addr))

    body = "<html><body>Hello {}</body></html>".format(addr)
    response = "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\nConnection: Closed\r\n\r\n" % len(body)
    cltskt.send(response + body)
    cltskt.close()