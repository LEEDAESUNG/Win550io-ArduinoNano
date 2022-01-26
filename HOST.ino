//////////////////////////////////////////////////////////////////////////////////////////
const uint8_t hostMaxReqCount = 3; //재요청횟수:최소 1~ 변경가능
//////////////////////////////////////////////////////////////////////////////////////////

uint8_t hostReqCount = 0; //카메라로부터 응답패킷 수신대기상태에서 수신안되는 경우 재요청횟수
int hostPeriod = 1000 * hostMaxReqCount; //최대 대기시간(ms)
unsigned long hostQuotient; //재요청시간 계산용(최소:0~ 최대:hostPeriod/hostMaxReqCount )
unsigned long hostStartTime; //카메라 접속 시간
unsigned long hostTimeOffset;// 카메라 접속 후 응답수신 대기중일때 오버플로우 처리용
//////////////////////////////////////////////////////////////////////////////////////////

void GateProc()
{
    if(GATE_UP_F == true)
    {
      if( startGATEUPTime+GATE_UP_DELAYTIME < millis()) 
      {
          digitalWrite(Relay4, LOW);
          GATE_UP_F = false;
          startGATEUPTime=0;
      }
    }
    else if(GATE_DOWN_F == true)
    {
      if( startGATEDOWNTime+GATE_DOWN_DELAYTIME < millis()) 
      {
          digitalWrite(Relay5, LOW);
          GATE_DOWN_F = false;
          startGATEDOWNTime=0;
      }
    }
}

//호스트로 패킷전송
bool HostProc(char *packet)
{
#ifdef DEBUG
  Serial.print("Host:");
  Serial.print(HostIP);
  Serial.print(":");
  Serial.println(HostPort);
#endif


//    if (HostClient.connect(HostIP, HostPort))
//    {
//      Serial.println("Host connected");
//      Serial.print("=> Host : ");
//      Serial.println(packet);
//      HostClient.println(packet); //호스트로 전송
//
//      Serial.println("Host disconnect");
//      HostClient.stop();
//    }
//    else
//      Serial.println("Host connect failed..");


    bool resConn = false;
    int sendCount = 0;
    if (HostClient.connect(HostIP, HostPort))
    {
      Serial.println("Host connected");
      resConn = true; //접속성공

      hostStartTime = millis();
      hostTimeOffset =0;
      hostQuotient = 0; // 몫 구하기
      hostPeriod = 1000 * hostMaxReqCount; //최대 대기시간(ms)
      hostReqCount = 0;
      while(HostClient.available()==0) //ack 수신대기
      {
          if(millis() < hostStartTime) //overflow
          {
            hostTimeOffset = 4294967294 - hostStartTime;
            hostStartTime = 0;
          }
          if(millis() - hostStartTime > hostPeriod - hostTimeOffset)
          {
              Serial.println("<= Host Recv : TimeOut");
              goto close;
          }

          //카메라로부터 응답메세지 받지못한 경우 재요청
          else
          {
              if(hostMaxReqCount>0 && hostReqCount<hostMaxReqCount) {
                  if(hostQuotient != (millis()/(hostPeriod/hostMaxReqCount)))
                  {
                      hostQuotient = (millis()/(hostPeriod/hostMaxReqCount));
                      
                      Serial.print("=> Host Send : ");
                      Serial.println(packet);
                      HostClient.println(packet); //명령어 재전송
                      hostReqCount++;
                  }
              }
          }
      }


      int size;
      while((size = HostClient.available()) > 0)
      {
        uint8_t* msg = (uint8_t*)malloc(size);
        size = HostClient.read(msg,size);
        Serial.print("<= Host : ");
        Serial.write(msg,size); // OK
        Serial.println("");
        free(msg);
      }
      
close:
      Serial.println("Host disconnect");
      HostClient.stop();

    }
    else 
    {
      Serial.println("Host connect failed..");
    }

    return resConn;
}

//패킷처리 및 응답패킷 생성
void ProcPacket()
{
    //패킷형태 체크
    //패킷형태 : STX Command ETX
    if(checkRecvPacketComplete(recvTempBuff) == true) {

        //recvTempBuff패킷에서 Command만 추출 후 recvRealBuff저장
        getCommand(strlen(recvTempBuff), recvRealBuff); 

        //디바이스정보 가져오기
        if(strcmp(recvRealBuff,"GETDEVICEINFO")==0) { 
            makeReplyBroadcast(); //브로드케스트 응답패킷 생성
            nowReplyCount = 2;    //전송한 횟수세팅(총 3회중 1회만 전송하면 됨)

            //호스트로 응답패킷 전송위한 초기화
            ReplyInit();
        }

        //호스트로부터 ACK_R 수신 체크
        else if(check_Ack_R(recvRealBuff)== true) { 
            nowReplyCount = 0;
            ackR_F = true;
        }

        //명령처리(차단기제어, 디바이스정보 저장...)
        else { 
            commandProc(recvRealBuff);
            makeReplyBuffer(recvRealBuff);//명령응답패킷 생성(형태:STX ACK ETX)
            nowReplyCount = 0;  //전송한 횟수세팅

            //호스트로 응답패킷 요청하기 위하여 초기화함
            ReplyInit();
        }
    
        clearBuffer();
    }
}

//호스트명령 처리
void commandProc(char* buff){
    if(strncmp(buff, "NO", 2)==0) { // EEPROM Mac Address 저장
      saveDeviceNo(buff+2);   //커맨드 제외한 데이터만 전달
    } 
    else if(strncmp(buff, "MC", 2)==0) { // EEPROM Mac Address 저장
      saveMacAddress(buff+2); //커맨드 제외한 데이터만 전달
    } 
    else if(strncmp(buff, "LI", 2)==0) { // EEPROM IP Address 저장
      saveIPAddress(buff+2); //커맨드 제외한 데이터만 전달
    } 
    else if(strncmp(buff, "SM", 2)==0) { // EEPROM Subnetmask 저장
      saveSubnetmask(buff+2);
    } 
    else if(strncmp(buff, "GW", 2)==0) { // EEPROM Gateway 저장
      saveGateway(buff+2);
    } 
    else if(strncmp(buff, "DS", 2)==0) { // EEPROM DNS 저장
      saveDNS(buff+2);
    } 
    else if(strncmp(buff, "RH", 2)==0) { // EEPROM RemoteHost IP 저장
      saveRemoteHostIP(buff+2);
    } 
    else if(strncmp(buff, "RP", 2)==0) { // EEPROM RemoteHost Port 저장
      saveRemoteHostPort(buff+2);
    }
    else if(strncmp(buff, "CM", 2)==0) { // EEPROM Camera IP 저장
      saveCameraIP(buff+2);
    }
    else if(strcmp(recvRealBuff,"GATE UP")==0) {  //차단기 열기
        Serial.println("GATE UP");
        
        digitalWrite(Relay4, HIGH);
        startGATEUPTime = millis();
        
        //delay(500);
        //digitalWrite(Relay4, LOW);
    }
    else if(strcmp(recvRealBuff,"GATE DOWN")==0) { //차단기 닫기
        Serial.println("GATE DOWN");
        
        digitalWrite(Relay5, HIGH);
        startGATEDOWNTime = millis();
        
        //delay(500);
        //digitalWrite(Relay5, LOW);
    }
}

// STX, ETX 체크
bool checkRecvPacketComplete(char *buff)
{
  bool findSTX = false;
  bool findETX = false;
  for(int i=0; i<UDP_TX_PACKET_MAX_SIZE; i++)
  {
    if(buff[i] == 0x02)
      findSTX = true;

    if(findSTX = true) {
      if(buff[i] == 0x03){
        findETX = true;
        break;
      }
    }
  }

  if(findSTX == true && findETX == true)
    return true;
  else
    return false;
}

//recvTempBuff에서 실제 명령어만 추출하여 recvRealBuff에 저장
void getCommand(int packetSize, char* realBuff)
{
  int startIndex =0;
  int endIndex = 0;
#ifdef DEBUG
  Serial.print("Recv:");
  Serial.println(recvTempBuff);
#endif
  startIndex = getIndex(0x02,packetSize, recvTempBuff);
  endIndex = getIndex(0x03,packetSize, recvTempBuff);
  getData(recvTempBuff,startIndex+1,endIndex-1, realBuff);
}
int getIndex(char ch, int n, char word[]) {
  int result = -1;
    for (int i = 0; i < n; i++) {
        if (word[i] == ch) result = i;
    }
    return result;
}
void getData(char* packet, int s, int e, char* realBuff) {
  int idx=0;
  if( s>=0 && s<=e && e<UDP_TX_PACKET_MAX_SIZE ){
    for (int i = s; i <= e; i++) {
          //recvRealBuff[idx] = packet[i];
          realBuff[idx] = packet[i];
          idx++;
      }
  }
}

//호스트로부터 수신한 패킷에서 ACK_R 여부 확인
bool check_Ack_R(char* buff)
{
  if(strncmp(buff,  "ACK_R", 5) == 0) {
    return true;
  }
  else{
    return false;
  }
}

//UDP 클라이언트 IP 출력
void printUDPClientIP()
{
  Serial.print("Remote IP:");
  IPAddress remote = UdpServer.remoteIP();
  for (int i = 0; i < 4; i++) {
    Serial.print(remote[i], DEC);
    if (i < 3) {
      Serial.print(".");
    }
  }
#ifdef DEBUG
  Serial.print(", Port: ");
  Serial.println(UdpServer.remotePort());
#endif
}


//브로드케스트 응답 패킷
//응답패킷 생성(ACK)
void makeReplyBroadcast()
{
  EEPROM.get( eepAddress, myDeviceObject );    //EEPROM 읽기
  
  char tmpBuff[UDP_TX_PACKET_MAX_SIZE];

  memset(tmpBuff, 0, UDP_TX_PACKET_MAX_SIZE);
  memset(replyBuff, 0, 128);

  int headLen = 5;
  replyBuff[0] = 0x02;
  strcat(replyBuff, "ACK_");//ex)0x02 ACK_NO0,MAC00:00:00:00:00:00,LI192.168.100.200.....0x03

  //Device No
  sprintf(tmpBuff, "NO%d,", myDeviceObject.no);
  strcat(replyBuff, tmpBuff);

  //Mac Address
  uint8_t MAC_temp[6];
  Ethernet.MACAddress(MAC_temp);
  sprintf(tmpBuff, "MC%02x:%02x:%02x:%02x:%02x:%02x,", MAC_temp[0],MAC_temp[1],MAC_temp[2],MAC_temp[3],MAC_temp[4],MAC_temp[5]);
  strcat(replyBuff, tmpBuff);

  //IP Adderss
  IPAddress ip = Ethernet.localIP();
  sprintf(tmpBuff, "LI%d.%d.%d.%d,", ip[0],ip[1],ip [2],ip[3]);//local IP address
//  strcat(tmpBuff, 0x0D); //CR
//  strcat(tmpBuff, 0x0A); //LF
  strcat(replyBuff, tmpBuff);
  
  sprintf(tmpBuff, "SM%d.%d.%d.%d,", myDeviceObject.sm1,myDeviceObject.sm2,myDeviceObject.sm3,myDeviceObject.sm4);
  strcat(replyBuff, tmpBuff);

  sprintf(tmpBuff, "GW%d.%d.%d.%d,", myDeviceObject.gw1,myDeviceObject.gw2,myDeviceObject.gw3,myDeviceObject.gw4);
  strcat(replyBuff, tmpBuff);

  sprintf(tmpBuff, "DS%d.%d.%d.%d,", myDeviceObject.dns1,myDeviceObject.dns2,myDeviceObject.dns3,myDeviceObject.dns4);
  strcat(replyBuff, tmpBuff);

  sprintf(tmpBuff, "RH%d.%d.%d.%d,", myDeviceObject.rh1,myDeviceObject.rh2,myDeviceObject.rh3,myDeviceObject.rh4);
  strcat(replyBuff, tmpBuff);

  sprintf(tmpBuff, "RP%d,", myDeviceObject.rhport);
  strcat(replyBuff, tmpBuff);

  sprintf(tmpBuff, "CM%d.%d.%d.%d", myDeviceObject.cameraip1,myDeviceObject.cameraip2,myDeviceObject.cameraip3,myDeviceObject.cameraip4);
  strcat(replyBuff, tmpBuff);

  replyBuff[strlen(replyBuff)] = 0x03;
#ifdef DEBUG
  Serial.print("Send:");
  Serial.print(strlen(replyBuff));
  Serial.println(" bytes");
#endif
}

//응답패킷 생성(ACK)
void makeReplyBuffer(char* buff)
{
  memset(replyBuff, 0, UDP_TX_PACKET_MAX_SIZE);
  replyBuff[0] = 0x02;
  strcat(replyBuff, "ACK");//ex)0x02ACK_GATE UP0x03
  replyBuff[strlen(replyBuff)] = 0x03;
  
//  memset(replyBuff, 0, UDP_TX_PACKET_MAX_SIZE);
//  replyBuff[0] = 0x02;
//  strcat(replyBuff, "ACK_");//ex)0x02ACK_GATE UP0x03
//  strcat(replyBuff, buff);
//  replyBuff[1+4+strlen(buff)] = 0x03;
}

//버퍼 클리어
void clearBuffer()
{
  memset(recvTempBuff, 0, UDP_TX_PACKET_MAX_SIZE);
  memset(recvRealBuff, 0, UDP_TX_PACKET_MAX_SIZE);
}

//호스트로 응답패킷전송
void sendReply()
{
  //응답패킷 전송 ex : STX ACK ETX
  if(nowReplyCount < maxHostReplyCount) {
    nowReplyCount++;
    if(recvProtocol == ProtocolUDP ){ //UDP 응답
      UdpServer.beginPacket(UdpServer.remoteIP(), UdpServer.remotePort());
      UdpServer.write(replyBuff);
      UdpServer.endPacket();
    }
    else if(recvProtocol == ProtocolTCP ){ //TCP 응답
      client.println(replyBuff);
    }

    Serial.print("Send:");
    Serial.println(replyBuff);
  } 
  else
  {
    //횟수초과시 더이상 재전송 안함
    ackR_F = true;
    nowReplyCount=0;
  }
}

//호스트로 응답패킷 요청하기 위하여 초기화함
void ReplyInit()
{
    replyStartTime = millis(); //응답패킷에 대한 수신대기 시작시간 저장
    replyTimeOffset = 0;
    replyQuotient = 0; // 몫 구하기
    replyReqCount = 0;
    
    ackR_F = false;
}
