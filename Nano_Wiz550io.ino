/*
 Communication with LabVIEW by UDP Protocol:
 This sketch send udp signal to LabVIEW (PC)
 created 24 Feb 2016
 by JelicleLim 
 http://www.winduino.com
 */

//배포시에는 #define 문장을 주석처리할 것
//#define DEBUG

//#define WIZ550io_WITH_MACADDRESS
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <EEPROM.h>

//extern volatile unsigned long timer0_millis; //타이머제어

////////////////////////////////////////////////////
//핀맵 정의
bool interrupt_F = false;
const byte interruptPin = 2;

const int Relay1 = 16; //Power Reset DC5V
const int Relay2 = 3;  //Power Reset DC12V
const int Relay3 = 4;  //Trigger
const int Relay4 = 5;  //Gate Up
const int Relay5 = 6;  //Gate Dn
const int Relay6 = 7;  //Buzzer
const int Relay7 = 8;  //S.P1
const int Relay8 = 9;  //S.P2
const int CDSPin = A3;// CDS


//////////////////////////////////////////////////////////////////////////////////////////
//부저
bool Buzzer_F = false;
unsigned long BuzzerStartTime;
unsigned long BuzzerTimeOffset;
const long BuzzerPeriod = 6000; // Buzzer 유지시간(ms)

//////////////////////////////////////////////////////////////////////////////////////////
//호스트
const uint8_t hostMaxReqCount = 3; //재요청횟수:최소 1~ 변경가능
uint8_t hostReqCount = 0; //카메라로부터 응답패킷 수신대기상태에서 수신안되는 경우 재요청횟수
int hostPeriod = 1000 * hostMaxReqCount; //최대 대기시간(ms)
unsigned long hostQuotient; //재요청시간 계산용(최소:0~ 최대:hostPeriod/hostMaxReqCount )
unsigned long hostStartTime; //카메라 접속 시간
unsigned long hostTimeOffset;// 카메라 접속 후 응답수신 대기중일때 오버플로우 처리용

//unsigned long currentAckRMillis = 0;
//unsigned long previousAckRMillis = 0;
//const long AckRInterval = 100; // AckR 체크 간격(ms)

const uint8_t replyMaxReqCount = 3; //응답 요청횟수:최소 1~ 변경가능
const int replyPeriod = 1000 * replyMaxReqCount;
unsigned long replyStartTime;
unsigned long replyTimeOffset;
uint8_t replyReqCount = 0; //카메라로부터 응답패킷 수신대기상태에서 수신안되는 경우 재요청횟수
unsigned long replyQuotient; //재요청시간 계산용(최소:0~ 최대:hostPeriod/hostMaxReqCount )

//////////////////////////////////////////////////////////////////////////////////////////
// CDS테이블
// 키값:cds, 데이터:exposure
// 예1) cds=0 ==> exposure=2100
// 예2) cds=1 ==> exposure=1900
//int exposureTable[11] = {2100,1900,1700,1500,1300,1100,900,700,500,300,100};
//int exposureTable[15] = {1500,1400,1300,1200,1100,1000,900,800,700,600,500,400,300,200,100};
//int exposureTable[10] = {1000,900,800,700,600,500,400,300,200,50};
//int exposureTable[14] = {1300,1200,1100,1000,900,800,700,600,500,400,300,200,100,50};
int exposureTable[16] = {2000,1900,1800,1700,1600,1500,1400,1300,1200,1100,1000,900,800,700,600,500};
int exposureIndex = 0;
int nowExposure = 0;
int saveExposure = 0;
int CDSValue = 0;
unsigned long CDSPreviousTime=0;
const long CDSPeriod = 6000; // CDS 체크 간격(ms) - 60초
unsigned long CDSTimeOffset=0;
//unsigned long currentCDSMillis = 0;
//unsigned long previousCDSMillis = 0;
//const long CDSInterval = 6000; // CDS 체크 간격(ms) - 60초

//////////////////////////////////////////////////////////////////////////////////////////
//카메라
const uint8_t cameraMaxReqCount = 3; //재요청횟수:최소 1~ 변경가능
uint8_t cameraReqCount = 0; //카메라로부터 응답패킷 수신대기상태에서 재요청횟수
int cameraPeriod = 1000 * cameraMaxReqCount; //최대 수신대기시간(ms)
unsigned long cameraQuotient; //재요청시간 계산용(최소:0~ 최대:cameraPeriod/cameraMaxReqCount )
unsigned long cameraStartTime; //카메라 접속 시간
unsigned long cameraTimeOffset;// 카메라 접속 후 응답수신 대기중일때 오버플로우 처리용

///////////////////////////////////////////////////
//차단기처리 플래그
bool GATE_UP_F = false;
bool GATE_DOWN_F = false;
uint16_t GATE_UP_DELAYTIME = 500; //(ms)
uint16_t GATE_DOWN_DELAYTIME = 500; //(ms)
unsigned long startGATEUPTime;
unsigned long startGATEDOWNTime;

///////////////////////////////////////////////////
//프로토콜 종류
#define ProtocolUDP 1
#define ProtocolTCP 2

///////////////////////////////////////////////////
//Wiz500io 공장출고 제품 초기값 세팅 또는 현재설정 상태 저장
uint16_t NO = 0;                        // device no
byte MAC[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
IPAddress IP(192, 168, 100, 211);
IPAddress SM(255, 255, 255, 0);
IPAddress GW(192, 168, 100, 1);
IPAddress DNS(8, 8, 8, 8);
IPAddress HostIP(192, 168, 100, 200);   // default host ip(tcp)
uint16_t HostPort = 50001;              // host port(tcp)
IPAddress CAMERAIP(192, 168, 100, 201); // default camera ip
uint16_t CAMERAPORT = 1335;             //camera command port

///////////////////////////////////////////////////
//서버 포트 정의
//IPAddress localIP;
unsigned int localPort = 50001;      // tcp, udp server local port
unsigned int localUDPPort = 50001;      // tcp, udp server local port
//
//수신버퍼 정의
char recvRealBuff[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char recvTempBuff[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char replyBuff[256];  // a string to send back
//
//호스트로부터 수신한 패킷에 대하여 응답패킷(ACK) 송신횟수 / 최대 송신횟수 정의
int maxACKSendCount = 3;
int nowACKSendCount = 0;
bool ackR_F = true; // 응답패킷(ACK)을 수신한 호스트에서 그에대한 응답패킷(ACK_R) 수신여부

//패킷을 수신한 프로토콜 종류
int svrProtocol = 0; //1:UDP, 2:TCP, 3:UDP BroadCast

//서버소켓 정의
EthernetUDP UdpServer;                                //UDP 서버
EthernetServer TcpServer = EthernetServer(localPort); //TCP 서버
EthernetClient client;                                //TCP 클라이언트
EthernetClient CameraClient;                          //TCP 카메라 클라이언트
EthernetClient HostClient;                            //TCP 호스트 클라이언트
////////////////////////////////////////////////////////////////
//EEPROM 데이터 정의
int eepAddress = 0;
struct DeviceObject {
  uint16_t no; //디바이스 고유넘버(0~99)
  
  uint8_t ip1; //local ip
  uint8_t ip2; //local ip
  uint8_t ip3; //local ip
  uint8_t ip4; //local ip
  
  uint8_t sm1; //subnetmask
  uint8_t sm2; //subnetmask
  uint8_t sm3; //subnetmask
  uint8_t sm4; //subnetmask
  
  uint8_t gw1; //gateway
  uint8_t gw2; //gateway
  uint8_t gw3; //gateway
  uint8_t gw4; //gateway
  
  uint8_t dns1; //gateway
  uint8_t dns2; //gateway
  uint8_t dns3; //gateway
  uint8_t dns4; //gateway
  
  byte mac1; //mac address
  byte mac2; //mac address
  byte mac3; //mac address
  byte mac4; //mac address
  byte mac5; //mac address
  byte mac6; //mac address
  
  uint8_t rh1; //remote Host
  uint8_t rh2; //remote Host
  uint8_t rh3; //remote Host
  uint8_t rh4; //remote Host
  uint16_t rhport; //remote Host Port

  uint8_t cameraip1;  //camera ip
  uint8_t cameraip2;  //camera ip
  uint8_t cameraip3;  //camera ip
  uint8_t cameraip4;  //camera ip
  
//  int TFmini_Distance; //거리
//  int TFmini_Count;    //체크 횟수
};
DeviceObject myDeviceObject;


//핀 설정
void SetPinMode()
{
  pinMode(Relay1, OUTPUT);
  pinMode(Relay2, OUTPUT);
  pinMode(Relay3, OUTPUT);
  pinMode(Relay4, OUTPUT);
  pinMode(Relay5, OUTPUT);
  pinMode(Relay6, OUTPUT);
  pinMode(Relay7, OUTPUT);
  pinMode(Relay8, OUTPUT);

  pinMode(interruptPin, INPUT_PULLUP);
  delay(100);                       // wait for a second
  attachInterrupt(digitalPinToInterrupt(interruptPin), blink, RISING);
}

//인터럽트 처리:호스트로 상태 전송
//const long previousInterruptMillis = 5000;//인터벌
void blink() {
  digitalWrite(Relay2, HIGH);

//  unsigned long currentInterruptMillis = millis();
//  if(interrupt_F == false) {
//    if (currentInterruptMillis - previousInterruptMillis >= HostSendInterval) {
//        previousInterruptMillis = currentInterruptMillis;
//
//        interrupt_F = true;
//    }
//  }

  if(interrupt_F == false) {
    interrupt_F = true;
  }

  if(Buzzer_F == false) {
    Buzzer_F = true;
  }

  digitalWrite(Relay2, LOW);
}


void setup() {

  Serial.begin(115200);

  //핀 모드 세팅
  SetPinMode();
  
  //EEPROM 초기상태일 경우 파라미터 값으로 초기화 한다.
  //EEPROM 이미 설정된 값이 있을 경우, EEPROM의 값을 파라미터로 재설정한다
  EEPROM_Init(NO, MAC, IP, SM, GW, DNS, HostIP, HostPort, CAMERAIP);

  //Ethernet.begin(localIP);            // Wiz550io IP세팅
  Ethernet.begin(MAC, IP, DNS, GW, SM); // Wiz550io 


  char tmpBuff[UDP_TX_PACKET_MAX_SIZE];
  uint8_t MAC_temp[6];
  Ethernet.MACAddress(MAC_temp); //Wiz550io 맥어드레스 가져오기
  IPAddress ip = Ethernet.localIP(); //Wiz550io IP주소 가져오기
  TcpServer.begin();                      // TCP서버(command)
  UdpServer.begin(localUDPPort);             // UDP서버(command)
  
#ifdef DEBUG
  sprintf(tmpBuff, "[MAC]%02X:%02X:%02X:%02X:%02X:%02X", MAC_temp[0],MAC_temp[1],MAC_temp[2],MAC_temp[3],MAC_temp[4],MAC_temp[5]);
  Serial.println(tmpBuff);
  
  sprintf(tmpBuff, "[IP]%d.%d.%d.%d", ip[0],ip[1],ip [2],ip[3]);//local IP address
  Serial.println(tmpBuff);

  Serial.print("[UDP Server] ");
  Serial.print(Ethernet.localIP());
  Serial.print(":");
  Serial.println(localUDPPort);

  Serial.print("[TCP Server] ");
  Serial.print(Ethernet.localIP());
  Serial.print(":");
  Serial.println(localPort);

  sprintf(tmpBuff, "[Device No] %d", myDeviceObject.no);
  Serial.println(tmpBuff);

  sprintf(tmpBuff, "[CAMERA IP]%d.%d.%d.%d", myDeviceObject.cameraip1,myDeviceObject.cameraip2,myDeviceObject.cameraip3,myDeviceObject.cameraip4);
  Serial.println(tmpBuff);

  sprintf(tmpBuff, "[HOST IP]%d.%d.%d.%d", myDeviceObject.rh1,myDeviceObject.rh2,myDeviceObject.rh3,myDeviceObject.rh4);
  Serial.println(tmpBuff);
#endif

  Serial.print("Jawootek LPR Board");
  Serial.println(" Ver 1.0");
}

void loop() {
  
  //UDP Server Receive(command:50001)
  int packetSize = UdpServer.parsePacket();
  if (packetSize) {
      svrProtocol = ProtocolUDP;
      UdpServer.read(recvTempBuff, UDP_TX_PACKET_MAX_SIZE);
#ifdef DEBUG
      Serial.println("UDP Server => "); //test
#endif
      ProcPacket();
  }

  //TCP Server Receive(command:50001)
  client = TcpServer.available();
  if (client) {
      if (client.available() > 0) {
          svrProtocol = ProtocolTCP; //TCP
          client.read(recvTempBuff, UDP_TX_PACKET_MAX_SIZE);
#ifdef DEBUG
          Serial.println("TCP Server => "); //test
#endif
          ProcPacket();
      }
  }


//  //소켓수신패킷에 대하여 응답패킷 전송
//  //ProcPacket() 이후 호스트로부터 ACK_R 수신할때까지 최대 3회 응답패킷 재전송
//  if(ackR_F == false) {
//      currentAckRMillis = millis();
//      if (currentAckRMillis - previousAckRMillis >= AckRInterval) {
//          previousAckRMillis = currentAckRMillis;
//          
//          sendReply(); //응답패킷 전송
//    }
//  }

    //소켓수신패킷에 대하여 응답패킷 전송
    //ProcPacket() 이후 호스트로부터 ACK_R 수신할때까지 최대 3회 응답패킷 재전송
    if(ackR_F == false)
    {
        if(millis() < replyStartTime) { //overflow
          replyTimeOffset = 4294967294 - replyStartTime;
          replyStartTime = 0;
        }
        if(millis() - replyStartTime > replyPeriod - replyTimeOffset)
        {
            if(replyMaxReqCount>0 && replyReqCount<replyMaxReqCount) {
                if(replyQuotient != (millis()/(replyPeriod/replyMaxReqCount))) //replyPeriod시간만큼 지연후 전송
                {
                    replyQuotient = (millis()/(replyPeriod/replyMaxReqCount));
                    
                    sendReply(); //응답패킷 전송
                    replyReqCount++;
                }
            }
        }
    }


  //차단기 처리
  if(GATE_UP_F == true || GATE_DOWN_F == true) {
      GateProc();
  }


//  //CDS처리
//  currentCDSMillis = millis();
//  if (currentCDSMillis - previousCDSMillis >= CDSInterval) {
//    previousCDSMillis = currentCDSMillis;
//    
//    CDSProc();
//  }

    //CDS처리 : CDSPeriod 시간마다 처리함
    if(millis() < CDSPreviousTime) { //overflow
          CDSTimeOffset = 4294967294 - CDSPreviousTime;
          CDSPreviousTime = 0;
    }
    if(millis() - CDSPreviousTime > CDSPeriod - CDSTimeOffset)    {
        CDSTimeOffset=0;
        CDSPreviousTime = millis();
        CDSProc();
    }


  //인터럽트 처리(충격센서)
  if( interrupt_F == true) {
      Serial.println("<= Shock");
      
      char shockPacket[UDP_TX_PACKET_MAX_SIZE];
      memset(shockPacket, 0, UDP_TX_PACKET_MAX_SIZE);
      shockPacket[0] = 0x02;
      sprintf(&shockPacket[1], "Shock_%d", myDeviceObject.no);
      shockPacket[strlen(shockPacket)] = 0x03;
      
      ShockProc(shockPacket);

      interrupt_F = false;
  }

  //부저 처리
  if(Buzzer_F == true) {
      BuzzerProc();
  }
}
