printf("srv=net.createConnection(net.TCP,0)")
printf("srv:on("receive",function(sck,c) print(c) end)")
printf("srv:on("connection",function(sck,c))
// change this to index.php probably 
printf("sck:send("GET /index.html HTTP /1.1\r\nHost: http://127.0.0.1\r\nConnection: close\r\nAccept: */*\r\n\r\n")
printf("end)")
printf("srv:connect(80,"192.168.4.2")")
