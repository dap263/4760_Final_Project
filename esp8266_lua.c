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

uart.setup(0, 256000, 8, uart.PARITY_NONE, uart.STOPBITS_1, 1)
_skip_headers = true
local function data_received(sck, c)
  if _skip_headers then
    -- simple logic to filter the HTTP headers
    local i, j = string.find(c, '\r\n\r\n')
    if i then
      _skip_headers = false
      c = string.sub(c, j+1, -1)
    end
  end
  print(c) 
end

srv=net.createConnection(net.TCP,0)
-- change to data_received
srv:on("receive",function(sck,c) print(c) end) 
srv:on("connection",function(sck,c)
// 192.168.4.2 in softap mode
// ra, dec: x.xxxxx, x.xxxxx
// tts: 'words to speak'
sck:send("GET tts: hello HTTP /1.1\r\nHost: 192.168.43.1\r\nConnection: close\r\nAccept: */*\r\n\r\n")
// what to use in softap
//printf("sck:send(\"GET 0.0045, 56.21 HTTP /1.1\r\nHost: 192.168.4.2\r\nConnection: close\r\nAccept: */*\r\n\r\n\")")
end)
srv:connect(5000,"192.168.43.14")
