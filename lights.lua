local socket = require'socket'
local t={
"power",
"power2",
"lan1",
"lan2",
"lan3",
"lan4",
"wifi",
"adsl",
"internet",
"internet2",
"voip",
"phone",
"phone2",
"usb",
"usb2",
"usb3",
"unlabeled",
}

local fps={}

for _,f in next,t do
	f=io.open("/sys/devices/gpio-leds.5/leds/"..f..'/brightness','wb')
	if not f then error"?1" end
	table.insert(fps,f)
end
f=nil

local state={}
local function set(i,f,on)
	if on~=state[i] then
		state[i]=on
		if on then
			f:write'1\n'
		else
			f:write'0\n'
		end
		f:flush()
	end
end

local s = socket.udp()
s:setsockname('*', 1234)

local rxtx,ip,port=...

if rxtx=="rx" then
	rxtx=true
elseif rxtx=="tx" then
	rxtx=false
else
	print"Usage: rx/tx [ip] [port]"
	return
end

if ip and ip:len()>3 and port then
	print("Forwarding to",ip,port)
else
	ip=false
end

local lights=#fps
while true do
	local ret,err = s:receive(3)
	if ret == nil then return end
	if ret:len()~=2 then
		print"wtflen"
		break
	end
	if ip then
		local ret,err=s:sendto(ret,ip,port)
		if not ret then
			print(err)
		end
	end
	
	local rx=string.byte(ret:sub(1,1))
	local tx=string.byte(ret:sub(2,2))
	
	rx=rx<0 and 0 or rx>100 and 100 or rx
	tx=tx<0 and 0 or tx>100 and 100 or tx
	local rxl=rx/100*lights
	local txl=tx/100*lights
	rxl=math.floor(rxl+0.5)
	txl=math.floor(txl+0.5)
	print(string.format("tx: %3d%%   rx: %3d%% ",rx,tx))
	
	for i,f in next,fps do
		set(i,f,i<=(rxtx and rxl or txl))
	end
	
	
end