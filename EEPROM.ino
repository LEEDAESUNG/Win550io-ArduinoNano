
//EEPROM 설정
void EEPROM_Init(uint16_t &no, byte *mac, IPAddress &ip, IPAddress &sm, IPAddress &gw, IPAddress &dns, IPAddress &hostIP, uint16_t &hostPort){

  EEPROM.get( eepAddress, myDeviceObject );    //EEPROM 읽기
  
  if(myDeviceObject.ip1== 255) { //공장초기화 상태:초기값 세팅
    myDeviceObject.no = no;
    myDeviceObject.ip1 = ip[0];
    myDeviceObject.ip2 = ip[1];
    myDeviceObject.ip3 = ip[2];
    myDeviceObject.ip4 = ip[3];
    myDeviceObject.sm1 = sm[0];
    myDeviceObject.sm2 = sm[1];
    myDeviceObject.sm3 = sm[2];
    myDeviceObject.sm4 = sm[3];
    myDeviceObject.gw1 = gw[0];
    myDeviceObject.gw2 = gw[1];
    myDeviceObject.gw3 = gw[2];
    myDeviceObject.gw4 = gw[3];
    myDeviceObject.dns1 = dns[0];
    myDeviceObject.dns2 = dns[1];
    myDeviceObject.dns3 = dns[2];
    myDeviceObject.dns4 = dns[3];
    myDeviceObject.mac1 = mac[0];
    myDeviceObject.mac2 = mac[1];
    myDeviceObject.mac3 = mac[2];
    myDeviceObject.mac4 = mac[3];
    myDeviceObject.mac5 = mac[4];
    myDeviceObject.mac6 = mac[5];
    myDeviceObject.rh1 = hostIP[0]; //remote Host
    myDeviceObject.rh2 = hostIP[1]; //remote Host
    myDeviceObject.rh3 = hostIP[2]; //remote Host
    myDeviceObject.rh4 = hostIP[3]; //remote Host
    myDeviceObject.rhport = hostPort; //remote Host Port

    EEPROM.put(eepAddress, myDeviceObject);   //EEPROM 쓰기
    EEPROM.get(eepAddress, myDeviceObject );  //EEPROM 읽기
  }

  no = myDeviceObject.no;
  mac[0] = myDeviceObject.mac1;  mac[1] = myDeviceObject.mac2;  mac[2] = myDeviceObject.mac3;
  mac[3] = myDeviceObject.mac4;  mac[4] = myDeviceObject.mac5;  mac[5] = myDeviceObject.mac6;
  ip[0] = myDeviceObject.ip1; ip[1] = myDeviceObject.ip2; ip[2] = myDeviceObject.ip3; ip[3] = myDeviceObject.ip4;
  sm[0] = myDeviceObject.sm1; sm[1] = myDeviceObject.sm2; sm[2] = myDeviceObject.sm3; sm[3] = myDeviceObject.sm4;
  gw[0] = myDeviceObject.gw1; gw[1] = myDeviceObject.gw2; gw[2] = myDeviceObject.gw3; gw[3] = myDeviceObject.gw4;
  dns[0] = myDeviceObject.dns1; dns[1] = myDeviceObject.dns2; 
  dns[2] = myDeviceObject.dns3; dns[3] = myDeviceObject.dns4;
  hostIP[0] = myDeviceObject.rh1; hostIP[1] = myDeviceObject.rh2;
  hostIP[2] = myDeviceObject.rh3; hostIP[3] = myDeviceObject.rh4;
  hostPort = myDeviceObject.rhport;
}


//디바이스 고유코드 저장(0~)
void saveDeviceNo(char *buff)
{
  uint16_t no = atoi(buff);
  myDeviceObject.no = no;
  EEPROM.put(eepAddress, myDeviceObject); //EEPROM 쓰기
  EEPROM.get(eepAddress, myDeviceObject );  //EEPROM 읽기

  NO = myDeviceObject.no;
}

//맥어드레스 저장 + 이더넷 재설정
void saveMacAddress(char *buff)
{
  byte mac[6];
  int count = 0;
  char *ptr = strtok(buff, ":");    // . 기준으로 문자열을 자르고, 포인터 반환
  while (ptr != NULL)               // 자른 문자열이 나오지 않을 때까지 반복
  {
      mac[count++] = strtol(ptr, NULL, 16);
      ptr = strtok(NULL, ":");      // 다음 문자열을 잘라서 포인터 반환
  }

  if(count == 6 && mac[0]>=0 && mac[0]<=255 && mac[1]>=0 && mac[1]<=255 && mac[2]>=0 && mac[2]<=255 && mac[3]>=0 && mac[3]<=255 && mac[4]>=0 && mac[4]<=255 && mac[5]>=0 && mac[5]<=255){
    myDeviceObject.mac1 = mac[0];
    myDeviceObject.mac2 = mac[1];
    myDeviceObject.mac3 = mac[2];
    myDeviceObject.mac4 = mac[3];
    myDeviceObject.mac5 = mac[4];
    myDeviceObject.mac6 = mac[5];
    EEPROM.put(eepAddress, myDeviceObject);   //EEPROM 쓰기
    EEPROM.get(eepAddress, myDeviceObject );  //EEPROM 읽기
    
    //변경한 맥어드레스 저장
    MAC[0]=myDeviceObject.mac1;MAC[1]=myDeviceObject.mac2;MAC[2]=myDeviceObject.mac3;
    MAC[3]=myDeviceObject.mac4;MAC[4]=myDeviceObject.mac5;MAC[5]=myDeviceObject.mac6;

    //이더넷 재설정
    Ethernet.begin(MAC, IP, DNS, GW, SM);
  }
  
  else{
#ifdef DEBUG
    Serial.println("Mac Address 형식 오류:");
#endif
  }
}

//IP어드레스 저장 + 이더넷 재설정
void saveIPAddress(char *buff)
{
  char sIP[8];
  int ip[4];
  int count = 0;
  char *ptr = strtok(buff, ".");  // . 기준으로 문자열을 자르고, 포인터 반환
  while (ptr != NULL)               // 자른 문자열이 나오지 않을 때까지 반복
  {
      sprintf(sIP, "%s", ptr);
      ip[count++] = atoi(sIP);
      ptr = strtok(NULL, ".");      // 다음 문자열을 잘라서 포인터를 반환
  }
  if(count == 4 && ip[0]>0 && ip[0]<255 && ip[1]>0 && ip[1]<255 && ip[2]>0 && ip[2]<255 && ip[3]>0 && ip[3]<255){
    myDeviceObject.ip1 = ip[0];
    myDeviceObject.ip2 = ip[1];
    myDeviceObject.ip3 = ip[2];
    myDeviceObject.ip4 = ip[3];
    EEPROM.put(eepAddress, myDeviceObject);   //EEPROM 쓰기
    EEPROM.get(eepAddress, myDeviceObject );  //EEPROM 읽기

    IP = {myDeviceObject.ip1, myDeviceObject.ip2, myDeviceObject.ip3, myDeviceObject.ip4};

    //이더넷 재설정
    Ethernet.begin(MAC, IP, DNS, GW, SM);
  }
  else{
#ifdef DEBUG
    Serial.println("IP형식 오류:");
#endif
  }
}

//Subnetmask 저장 + 이더넷 재설정
void saveSubnetmask(char *buff)
{
  char sSM[8];
  int sm[4];
  int count = 0;
  char *ptr = strtok(buff, ".");  // . 기준으로 문자열을 자르고, 포인터 반환
  while (ptr != NULL)               // 자른 문자열이 나오지 않을 때까지 반복
  {
      sprintf(sSM, "%s", ptr);
      sm[count++] = atoi(sSM);
      ptr = strtok(NULL, ".");      // 다음 문자열을 잘라서 포인터를 반환
  }
  if(count == 4 && sm[0]>0 && sm[0]<=255 && sm[1]>=0 && sm[1]<=255 && sm[2]>=0 && sm[2]<=255 && sm[3]>=0 && sm[3]<=255){
    myDeviceObject.sm1 = sm[0];
    myDeviceObject.sm2 = sm[1];
    myDeviceObject.sm3 = sm[2];
    myDeviceObject.sm4 = sm[3];
    EEPROM.put(eepAddress, myDeviceObject);   //EEPROM 쓰기
    EEPROM.get(eepAddress, myDeviceObject );  //EEPROM 읽기

    SM = {myDeviceObject.sm1, myDeviceObject.sm2, myDeviceObject.sm3, myDeviceObject.sm4};
  
    //이더넷 재설정
    Ethernet.begin(MAC, IP, DNS, GW, SM);
  }
  else{
#ifdef DEBUG
    Serial.println("SM형식 오류:");
#endif
  }
}

//게이트웨이 저장 + 이더넷 재설정
void saveGateway(char *buff)
{
  char sGW[8];
  int gw[4];
  int count = 0;
  char *ptr = strtok(buff, ".");  // . 기준으로 문자열을 자르고, 포인터 반환
  while (ptr != NULL)               // 자른 문자열이 나오지 않을 때까지 반복
  {
      sprintf(sGW, "%s", ptr);
      gw[count++] = atoi(sGW);
      ptr = strtok(NULL, ".");      // 다음 문자열을 잘라서 포인터를 반환
  }
  if(count == 4 && gw[0]>0 && gw[0]<255 && gw[1]>0 && gw[1]<255 && gw[2]>0 && gw[2]<255 && gw[3]>0 && gw[3]<255){
    myDeviceObject.gw1 = gw[0];
    myDeviceObject.gw2 = gw[1];
    myDeviceObject.gw3 = gw[2];
    myDeviceObject.gw4 = gw[3];
    EEPROM.put(eepAddress, myDeviceObject);   //EEPROM 쓰기
    EEPROM.get(eepAddress, myDeviceObject );  //EEPROM 읽기

    GW = {myDeviceObject.gw1, myDeviceObject.gw2, myDeviceObject.gw3, myDeviceObject.gw4};

    //이더넷 재설정
    Ethernet.begin(MAC, IP, DNS, GW, SM);
  }
  else{
#ifdef DEBUG
    Serial.println("SM형식 오류:");
#endif
  }
}

//DNS 저장 + 이더넷 재설정
void saveDNS(char *buff)
{
  char sDNS[8];
  int dns[4];
  int count = 0;
  char *ptr = strtok(buff, ".");  // . 기준으로 문자열을 자르고, 포인터 반환
  while (ptr != NULL)               // 자른 문자열이 나오지 않을 때까지 반복
  {
      sprintf(sDNS, "%s", ptr);
      dns[count++] = atoi(sDNS);
      ptr = strtok(NULL, ".");      // 다음 문자열을 잘라서 포인터를 반환
  }
  if(count == 4 && dns[0]>0 && dns[0]<255 && dns[1]>0 && dns[1]<255 && dns[2]>0 && dns[2]<255 && dns[3]>0 && dns[3]<255){
    myDeviceObject.dns1 = dns[0];
    myDeviceObject.dns2 = dns[1];
    myDeviceObject.dns3 = dns[2];
    myDeviceObject.dns4 = dns[3];
    EEPROM.put(eepAddress, myDeviceObject); //EEPROM 쓰기
    EEPROM.get(eepAddress, myDeviceObject );  //EEPROM 읽기

    DNS = {myDeviceObject.dns1, myDeviceObject.dns2, myDeviceObject.dns3, myDeviceObject.dns4};

    //이더넷 재설정
    Ethernet.begin(MAC, IP, DNS, GW, SM);
  }
  else{
#ifdef DEBUG
    Serial.println("DNS형식 오류:");
#endif
  }
}

//리모트호스트 IP저장
void saveRemoteHostIP(char *buff)
{
  char sIP[8];
  int ip[4];
  int count = 0;
  char *ptr = strtok(buff, ".");  // . 기준으로 문자열을 자르고, 포인터 반환
  while (ptr != NULL)               // 자른 문자열이 나오지 않을 때까지 반복
  {
      sprintf(sIP, "%s", ptr);
      ip[count++] = atoi(sIP);
      ptr = strtok(NULL, ".");      // 다음 문자열을 잘라서 포인터를 반환
  }
  if(count == 4 && ip[0]>0 && ip[0]<255 && ip[1]>0 && ip[1]<255 && ip[2]>0 && ip[2]<255 && ip[3]>0 && ip[3]<255){
    myDeviceObject.rh1 = ip[0];
    myDeviceObject.rh2 = ip[1];
    myDeviceObject.rh3 = ip[2];
    myDeviceObject.rh4 = ip[3];
    EEPROM.put(eepAddress, myDeviceObject);   //EEPROM 쓰기
    EEPROM.get(eepAddress, myDeviceObject );  //EEPROM 읽기

    HostIP = {myDeviceObject.ip1, myDeviceObject.ip2, myDeviceObject.ip3, myDeviceObject.ip4};
  }
  else{
#ifdef DEBUG
    Serial.println("Remote HostIP 형식 오류:");
#endif
  }
}

//리모트호스트 IP포트정보 저장(미사용)
void saveRemoteHostPort(char *buff)
{
  uint16_t port = atoi(buff);
  myDeviceObject.rhport = port;
  EEPROM.put(eepAddress, myDeviceObject); //EEPROM 쓰기
  EEPROM.get(eepAddress, myDeviceObject );  //EEPROM 읽기

  HostPort = myDeviceObject.rhport;
}
