

//CDS처리
void CDSProc(){

    CDSValue = analogRead(sensorPin);

    char hostCommand[UDP_TX_PACKET_MAX_SIZE];
    memset(hostCommand, 0, UDP_TX_PACKET_MAX_SIZE);
    hostCommand[0] = 0x02;
    sprintf(&hostCommand[1],"%d_CDS_%d",myDeviceObject.no, CDSValue);//CDS raw 데이터
    hostCommand[strlen(hostCommand)] = 0x03;
  
    HostProc(hostCommand, 1);
}
