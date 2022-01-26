//////////////////////////////////////////////////////////////////////////////////////////
// CDS테이블
// 키값:cds, 데이터:exposure
// 예1) cds=0 ==> exposure=2100
// 예2) cds=1 ==> exposure=1900
// 예3) cds=10 ==> exposure=100
//int exposureTable[11] = {2100,1900,1700,1500,1300,1100,900,700,500,300,100};
//int exposureTable[15] = {1500,1400,1300,1200,1100,1000,900,800,700,600,500,400,300,200,100};
int exposureTable[10] = {1000,900,800,700,600,500,400,300,200,100};
int nowCDS = 0;
int nowExposure = 0;
int saveExposure = 0;


//////////////////////////////////////////////////////////////////////////////////////////
const uint8_t cameraMaxReqCount = 3; //재요청횟수:최소 1~ 변경가능
//////////////////////////////////////////////////////////////////////////////////////////

uint8_t cameraReqCount = 0; //카메라로부터 응답패킷 수신대기상태에서 수신안되는 경우 재요청횟수
int cameraPeriod = 1000 * cameraMaxReqCount; //최대 대기시간(ms)
unsigned long cameraQuotient; //재요청시간 계산용(최소:0~ 최대:cameraPeriod/cameraMaxReqCount )
unsigned long cameraStartTime; //카메라 접속 시간
unsigned long cameraTimeOffset;// 카메라 접속 후 응답수신 대기중일때 오버플로우 처리용
//////////////////////////////////////////////////////////////////////////////////////////


//CDS처리
void CDSProc(){

  CDSValue = analogRead(CDSPin);
  //int val1 = map(CDSValue, 100, 800, 0, 10);//CDS 아날로그 데이터 100~800값을 0~10으로 맵핑
  //int val1 = map(CDSValue, 0, 1023, 0, 14);
  int val1 = map(CDSValue, 0, 1023, 0, 9);
  nowCDS = constrain(val1, 0, 9);             //값의 범위를 0~10으로 제한

  nowExposure = exposureTable[nowCDS];
  if(saveExposure != nowExposure) {

    Serial.print("CDS row data:");
    Serial.println(CDSValue);

    char cameraCommand[32];
    saveExposure = nowExposure;
    sprintf(cameraCommand,"SetExposure %d%2x%2x", CDSValue,13,10);//임시 raw데이터 함께 전송

    int retryConnectCount = 3; //카메라접속 실패시 3회 재접속시도
    while(retryConnectCount > 0 && CameraProc(cameraCommand) == false){ //카메라 Exposure 처리(최대 3회 시도)
        retryConnectCount--;
    }
  }
}

//카메라 Exposure 세팅
bool CameraProc(char* command)
{
#ifdef DEBUG
  Serial.print("Camera IP:");
  Serial.print(CAMERAIP);
  Serial.print(":");
  Serial.println(CAMERAPORT);
#endif

    bool resConn = false;
  
    if (CameraClient.connect(CAMERAIP,CAMERAPORT))
    {
        Serial.println("Camera connected");
        resConn = true;//카메라 접속성공
        
        //CameraClient.println(command); //명령어 전송
        
        cameraStartTime = millis();
        cameraTimeOffset =0;
        cameraQuotient = 0; // 몫 구하기
        cameraPeriod = 1000 * cameraMaxReqCount; //최대 대기시간(ms)
        cameraReqCount = 0;
        while(CameraClient.available()==0) //ack 수신대기
        {
            if(millis() < cameraStartTime) //overflow
            {
              cameraTimeOffset = 4294967294 - cameraStartTime;
              cameraStartTime = 0;
            }
            if(millis() - cameraStartTime > cameraPeriod - cameraTimeOffset)
            {
                Serial.println("<= Camera Recv : TimeOut");
                goto close;
            }

            //카메라로부터 응답메세지 받지못한 경우 재요청
            else
            {
                if(cameraMaxReqCount>0 && cameraReqCount<cameraMaxReqCount) {
                    if(cameraQuotient != (millis()/(cameraPeriod/cameraMaxReqCount)))
                    {
                        cameraQuotient = (millis()/(cameraPeriod/cameraMaxReqCount));
                        
                        Serial.print("=> Camera Send : ");
                        Serial.println(command);
                        CameraClient.println(command); //명령어 재전송
                        cameraReqCount++;
                    }
                }
            }
        }

        
        int size;
        while((size = CameraClient.available()) > 0)
        {
          uint8_t* msg = (uint8_t*)malloc(size);
          size = CameraClient.read(msg,size);
          Serial.print("<= Camera : ");
          Serial.write(msg,size); // OK
          free(msg);
        }

close:
        CameraClient.stop();
        Serial.println("Camera disconnect");

        //호스트로 전송
        if(resConn == true) {
            //HostProc(command);

            char CDSRePackage[UDP_TX_PACKET_MAX_SIZE];
            memset(CDSRePackage, 0, UDP_TX_PACKET_MAX_SIZE);
            CDSRePackage[0] = 0x02;
            //sprintf(&CDSRePackage[1], "%s", command); //원본
            sprintf(&CDSRePackage[1],"%s_%d_%d", command,nowExposure,myDeviceObject.no); //임시
            CDSRePackage[strlen(CDSRePackage)] = 0x03;

            int retryHostToConnectCount = 3; //호스트접속 실패시 3회 재접속시도
            while(retryHostToConnectCount > 0 && HostProc(CDSRePackage) == false)
            {
                retryHostToConnectCount--;
            }
        }
    }
    else{
      Serial.println("Camera connect failed..");
    }
    
    return resConn;
}
