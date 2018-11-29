

function cb_connected(c)
  print('sending')
  --c:send("GET tts: hello HTTP /1.1\r\nHost: 192.168.43.14\r\nConnection: close\r\nAccept: */*\r\n\r\n")
  print('conn sent')
  --f = file.open('temp.wav', w+)
  _path = nil
end

function cb_disconnected()
     fd:close()
     --drv = pcm.new(pcm.SD, 8)
     --drv:play(pcm.RATE_8K)
end

function data_received(c, data)

  if _skip_headers then
    -- simple logic to filter the HTTP headers
    _chunk = data
    local i, j = string.find(_chunk, '\r\n\r\n')
    if i then
      _skip_headers = false
      data = string.sub(_chunk, j+1, -1)
      _chunk = nil
    end
  end

  --if not _skip_headers then
    fd:write(data)
  --end

end 

_skip_headers = true
ip = "192.168.43.14"
port = 5000

_conn = net.createConnection(net.TCP, 0)
_conn:on("receive", data_received)
_conn:on("disconnection", cb_disconnected)
--_conn:connect(port, ip, cb_connected)
_conn:on("connection",function(sck,c)
sck:send("GET tts : hello\r\n HTTP /1.1\r\nHost: 192.168.43.1\r\nConnection: close\r\nAccept: */*\r\n\r\n")
end)
_conn:connect(5000,"192.168.43.14")
--tmr.delay(10000000)
fd = file.open("temp.wav", "w+")

