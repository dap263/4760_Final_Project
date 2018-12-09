-- callback to send termination character
function cb_disconnected()
    print("\a")  
end
-- callback to send serial data and dma start character
function cb_connected(sck, c)  
    print(c)  
    if (i == 0) then  
        print("\b")  
    end        
    i = i + 1        
end      
-- initialize global counter
i = 0       
srv=net.createConnection(net.TCP,0)  
-- register receive packet, disconnect, and connect callbacks
srv:on("receive", cb_connected)  
srv:on("disconnection", cb_disconnected)  
srv:on("connection",function(sck,c)  
    -- you may need to change the ip address of Host
sck:send("GET tts: Hello, World\r\n HTTP /1.1\r\nHost: 192.168.43.1\r\nConnection: close\r\nAccept: */*\r\n\r\n")
end)  
-- connect to local host, port 5000
srv:connect(5000,"192.168.43.14")  