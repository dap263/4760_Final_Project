// will probably have to escape all the quotes with \


printf("srv=net.createConnection(net.TCP,0)")
printf("srv:on("receive",function(sck,c) print(c) end)")
printf("srv:on("connection",function(sck,c)")
// change this to index.php probably 
// just copy for PuTTY
printf("sck:send("GET 0.0045, 56.21 HTTP /1.1\r\nHost: 192.168.4.2\r\nConnection: close\r\nAccept: */*\r\n\r\n")")
// what to actually use
//printf("sck:send(\"GET 0.0045, 56.21 HTTP /1.1\r\nHost: 192.168.4.2\r\nConnection: close\r\nAccept: */*\r\n\r\n\")")
printf("end)")
printf("srv:connect(5000,"192.168.4.2")")
