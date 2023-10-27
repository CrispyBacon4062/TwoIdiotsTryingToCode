import socket

sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)      # For UDP

udp_host = socket.gethostname()            # Host IP
udp_port = 4210                     # specified port to connect

sock.bind((udp_host,udp_port))
print ("Waiting for client...")

HeaderDelimitChar = "$"

while True:
  data,addr = sock.recvfrom(1024)         #receive data from client
  print("data #",data,"#")
  Msg = data.decode('utf-8')
  print ("Received Message: #",Msg,"# from",addr)
  EndOfHeader = Msg.find(HeaderDelimitChar)
  HeaderBytes = Msg[0:EndOfHeader]
  FileName = HeaderBytes + ".txt"
  print("Filename #",FileName,"#")
  myFile = open(FileName, "a+")
  EndOfStr = data.find(0)
  MsgToWrite = Msg[EndOfHeader + 1 :1024] + '\r'
  myFile.write(MsgToWrite);
  myFile.close()
  print ("Data #",MsgToWrite,"#")
