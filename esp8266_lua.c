// will probably have to escape all the quotes with \

// connect to network
wifi.setmode(wifi.STATION)
delay(1000)
station_cfg = {}
station_cfg.ssid="FBI"
station_cfg.pwd="verystrong"
station_cfg.save=true
wifi.sta.config(station_cfg)
wifi.sta.connect()
print(wifi.sta.getip())

printf("srv=net.createConnection(net.TCP,0)")
printf("srv:on("receive",function(sck,c) print(c) end)")
printf("srv:on("connection",function(sck,c)")
// 192.168.4.2 in softap mode
// ra, dec: x.xxxxx, x.xxxxx
// tts: 'words to speak'
printf("sck:send("GET tts: hello HTTP /1.1\r\nHost: 192.168.43.1\r\nConnection: close\r\nAccept: */*\r\n\r\n")")
// what to use in softap
//printf("sck:send(\"GET 0.0045, 56.21 HTTP /1.1\r\nHost: 192.168.4.2\r\nConnection: close\r\nAccept: */*\r\n\r\n\")")
printf("end)")
printf("srv:connect(5000,"192.168.43.14")")
