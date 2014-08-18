$port = new-Object System.IO.Ports.SerialPort COM3,115200,None,8,one
$port.Open()
$port.Write("r")
start-sleep -m 50
$port.Close()
