

//호스트처리
//msg:전송할 데이터
//maxSendCount : 전송실패할 경우, 최대전송횟수
bool HostProc(char *msg, int maxSendCount)
{
    bool resConn = false;
    unsigned long hostStartTime;
    unsigned long hostTimeOffset;
    unsigned long hostQuotient; // 몫 구하기
    int hostPeriod; //호스트응답 수신대기 최대시간(ms);
    uint8_t hostReqCount;
    
    if (HostClient.connect(HostIP, HostPort))
    {
#ifdef DEBUG
      Serial.println("Host connected");
#endif

      resConn = true; //접속성공
      hostStartTime = millis();
      hostTimeOffset = 0;
      hostQuotient = 0; // 몫 구하기
      hostPeriod =  1000 * maxSendCount; //호스트응답 수신대기 최대시간(ms);
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
#ifdef DEBUG
              Serial.println("<= Host Recv : TimeOut");
#endif
              goto close;
          }

          //호스트로부터 응답메세지 받지못한 경우 재요청
          else
          {
              if(maxSendCount>0 && hostReqCount<maxSendCount) {
                  if(hostQuotient != (millis()/(hostPeriod/maxSendCount)))
                  {
                      hostQuotient = (millis()/(hostPeriod/maxSendCount));
#ifdef DEBUG
                      Serial.print("=> Host Send : ");
                      Serial.println(msg);
#endif
                      HostClient.println(msg); //명령어 재전송
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
#ifdef DEBUG
          Serial.print("<= Host Recv : ");
          Serial.write(msg,size); // OK
          Serial.println("");
#endif
          free(msg);
      }

close:
#ifdef DEBUG
        Serial.println("Host disconnect");
#endif
        HostClient.stop();

    }
    else 
    {
#ifdef DEBUG
        Serial.println("Host connect failed..");
#endif
    }

    return resConn;
}

//수신패킷처리 및 응답패킷 생성
void ProcPacket(char *buff)
{
    //패킷포맷 : STX Command ETX
    if(checkRecvPacketComplete(buff) == true) {

        //packet에서 실제Command만 추출 후 recvRealBuff에 저장
        getCommand(buff, recvRealBuff); 

        //디바이스정보 가져오기
        if(strcmp(recvRealBuff,"GETDEVICEINFO")==0) { 
            makeReplyBroadcast();   //응답패킷 생성
            nowACKSendCount = 2;    //전송한 횟수세팅(총 3회중 1회만 전송하기위해 2회전송한 것으로 세팅)

            //호스트로 응답패킷을 전송하기 위해 초기화
            ReplyInit();
        }

        //호스트로부터 ACK_R 수신 체크
        else if(check_AckR(recvRealBuff)== true) { 
            nowACKSendCount = 0;
            ackR_F = true;
        }

        //명령처리(차단기제어, 디바이스정보 저장...)
        else { 
            commandProc(recvRealBuff);
            makeReplyBuffer();    //응답패킷 생성
            nowACKSendCount = 0;  //전송한 횟수세팅

            //호스트로 응답패킷 전송하기 위해 초기화
            ReplyInit();
        }
    
        clearBuffer(buff);
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
//    else if(strncmp(buff, "CM", 2)==0) { // EEPROM Camera IP 저장
//      saveCameraIP(buff+2);
//    }
    else if(strcmp(recvRealBuff,"GATE UP")==0) {  //차단기 열기
#ifdef DEBUG
        Serial.println("GATE UP");
#endif
        digitalWrite(GateUp_Relay, LOW);
        GATEUP_StartTime = millis();
        GATE_UP_F = true;
    }
    else if(strcmp(recvRealBuff,"GATE DOWN")==0) { //차단기 닫기
#ifdef DEBUG
        Serial.println("GATE DOWN");
#endif
        digitalWrite(GateDn_Relay, LOW);
        GATEDOWN_StartTime = millis();
        GATE_DOWN_F = true;
        
        //delay(500);
        //digitalWrite(GateDn_Relay, LOW);
    }
    else if(strcmp(recvRealBuff,"GETFRAME")==0) { //카레라캡쳐
#ifdef DEBUG
        Serial.println("GATEFRAME");
#endif
        digitalWrite(Trigger_Relay, LOW);
        TRIGGER_StartTime = millis();
        TRIGGER_F = true;
    }
    else if(strcmp(recvRealBuff,"POWER ON")==0) { //파워ON
#ifdef DEBUG
        Serial.println("POWER ON");
#endif
        digitalWrite(DC5V_Relay, LOW);
        digitalWrite(DC12V_Relay, LOW);
    }
    else if(strcmp(recvRealBuff,"POWER OFF")==0) { //파워OFF
#ifdef DEBUG
        Serial.println("POWER OFF");
#endif
        digitalWrite(DC5V_Relay, HIGH);
        digitalWrite(DC12V_Relay, HIGH);
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
//void getCommand(int packetSize, char* realBuff)
void getCommand(char* tempBuff, char* realBuff)
{
  int startIndex =0;
  int endIndex = 0;
  int packetSize = strlen(tempBuff);

//  startIndex = getIndex(0x02,packetSize, recvTempBuff);
//  endIndex = getIndex(0x03,packetSize, recvTempBuff);
  //getData(recvTempBuff,startIndex+1,endIndex-1, realBuff);
  startIndex = getIndex(0x02,packetSize, tempBuff);
  endIndex = getIndex(0x03,packetSize, tempBuff);
  getData(tempBuff,startIndex+1,endIndex-1, realBuff);
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
bool check_AckR(char* buff)
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
#ifdef DEBUG
  Serial.print("Remote IP:");
  IPAddress remote = UdpServer.remoteIP();
  for (int i = 0; i < 4; i++) {
    Serial.print(remote[i], DEC);
    if (i < 3) {
      Serial.print(".");
    }
  }
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

  replyBuff[strlen(replyBuff)] = 0x03;

}

//응답패킷 생성(ACK)
void makeReplyBuffer()
{
//  memset(replyBuff, 0, UDP_TX_PACKET_MAX_SIZE);
//  replyBuff[0] = 0x02;
//  strcat(replyBuff, "ACK");//ex)0x02ACK_GATE UP0x03
//  replyBuff[strlen(replyBuff)] = 0x03;
    memset(replyBuff, 0, UDP_TX_PACKET_MAX_SIZE);
    replyBuff[0] = 0x02;
    sprintf(&replyBuff[1],"%d_ACK",myDeviceObject.no);
    replyBuff[strlen(replyBuff)] = 0x03;
}

//버퍼 클리어
void clearBuffer(char* buff)
{
  //memset(recvTempBuff, 0, UDP_TX_PACKET_MAX_SIZE);
  memset(buff, 0, UDP_TX_PACKET_MAX_SIZE);
  memset(recvRealBuff, 0, UDP_TX_PACKET_MAX_SIZE);
}

//호스트로 응답패킷전송
void SendReply()
{
  //응답패킷 전송 : stx ACK etx
  if(nowACKSendCount < maxACKSendCount) {
    nowACKSendCount++;
    if(recvProtocol == ProtocolUDP ){ //UDP 응답
      UdpServer.beginPacket(UdpServer.remoteIP(), UdpServer.remotePort());
      UdpServer.write(replyBuff);
      UdpServer.endPacket();
    }
    else if(recvProtocol == ProtocolTCP ){ //TCP 응답
      client.println(replyBuff);
    }
#ifdef DEBUG
    Serial.print("Send:");
    Serial.println(replyBuff);
#endif
  } 
  else
  {
    //횟수초과시 더이상 재전송 안함
    ackR_F = true;
    nowACKSendCount=0;
  }
}

//호스트로 응답패킷 요청하기 위하여 초기화함
void ReplyInit()
{
    ReplyPreviousTime = millis()-ReplyPeriod; //reply 즉시 실행위해 -ReplyPeriod 했다
    ReplyReqCount = 0;
    Reply_OffsetTime = 0;
    
    ackR_F = false;
}

//릴레이 처리
void RelayProc()
{
    if(GATE_UP_F == true)
    {
      if(millis() < GATEUP_StartTime) //overflow
      {
        GATEUP_OffsetTime = 4294967294 - GATEUP_StartTime;
        GATEUP_StartTime = 0;
      }
      if(millis() - GATEUP_StartTime > GATE_UP_DELAYTIME - GATEUP_OffsetTime) //GATE_UP_DELAYTIME 시간 후 처리
      {
          digitalWrite(GateUp_Relay, HIGH);
          GATE_UP_F = false;
          GATEUP_StartTime=0;
          GATEUP_OffsetTime=0;
      }
    }
    else if(GATE_DOWN_F == true)
    {
      if(millis() < GATEDOWN_StartTime) //overflow
      {
        GATEDOWN_OffsetTime = 4294967294 - GATEDOWN_StartTime;
        GATEDOWN_StartTime = 0;
      }
      if(millis() - GATEDOWN_StartTime > GATE_DOWN_DELAYTIME - GATEDOWN_OffsetTime) //GATE_DOWN_DELAYTIME 시간 후 처리
      {
          digitalWrite(GateDn_Relay, HIGH);
          GATE_DOWN_F = false;
          GATEDOWN_StartTime=0;
          GATEDOWN_OffsetTime=0;
      }
    }
    else if(TRIGGER_F == true)
    {
      if(millis() < TRIGGER_StartTime) //overflow
      {
        TRIGGER_OffsetTime = 4294967294 - TRIGGER_StartTime;
        TRIGGER_StartTime = 0;
      }
      if(millis() - TRIGGER_StartTime > TRIGGER_DELAYTIME - TRIGGER_OffsetTime) //TRIGGER_DELAYTIME 시간 후 처리
      {
          digitalWrite(Trigger_Relay, HIGH);
          TRIGGER_F = false;
          TRIGGER_StartTime=0;
          TRIGGER_OffsetTime=0;
      }
    }
    
}
