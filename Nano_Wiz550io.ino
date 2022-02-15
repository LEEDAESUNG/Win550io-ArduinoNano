/*
 Communication with LabVIEW by UDP Protocol:
 This sketch send udp signal to LabVIEW (PC)
 created 24 Feb 2016
 by JelicleLim 
 http://www.winduino.com
 */


#include <SoftwareSerial.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <EEPROM.h>

#define DEBUG

SoftwareSerial mySerial(A5, A4); // RX, TX


//핀맵 정의
bool interrupt_F = false;
const byte interruptShockPin = 2;

const int DC5V_Relay = A2; //A2; Power Reset DC5V
const int DC12V_Relay = A1;  //Power Reset DC12V
const int Trigger_Relay = 4;  //Trigger
const int GateUp_Relay = 5;  //
const int GateDn_Relay = 6;  //
const int Buzzer_Relay = 7;  //
const int SP1_Relay = 8;  //S.P1
const int SP2_Relay = 9;  //S.P2
//const int sensorPin = A0;    // select the input pin for the potentiometer
const int sensorPin = A3;    // select the input pin for the potentiometer



//프로토콜 종류
#define ProtocolUDP 1
#define ProtocolTCP 2


//Wiz500io 공장출고 제품 초기값 세팅 또는 현재설정 상태 저장
uint16_t NO = 0;                                   // default Device no
byte MAC[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // default MAC
IPAddress IP(192, 168, 100, 211);                  // default Device IP
IPAddress SM(255, 255, 255, 0);                    // default Device Subnetmask
IPAddress GW(192, 168, 100, 1);                    // default Device Gateway
IPAddress DNS(8, 8, 8, 8);                         // default Device DNS
IPAddress HostIP(192, 168, 100, 200);              // default Remote Host IP(tcp)
uint16_t HostPort = 50001;                         // default Remote Host Port(tcp)


//서버 포트 정의
//IPAddress localIP;
unsigned int localPort = 50001;      // tcp server local port
unsigned int localUDPPort = 50001;   // udp server local port


//서버소켓 정의
EthernetUDP UdpServer;                                //UDP 서버
EthernetServer TcpServer = EthernetServer(localPort); //TCP 서버
EthernetClient client;                                //TCP 클라이언트
EthernetClient HostClient;                            //TCP 호스트 클라이언트


//수신 프로토콜 구분
int recvProtocol = 0; //1:UDP, 2:TCP, 3:UDP BroadCast


//수신버퍼 정의
char recvRealBuff[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char replyBuff[256];  // a string to send back


//호스트로로 응답패킷(ACK) 송신횟수 / 최대 송신횟수 정의
int maxACKSendCount = 3;
int nowACKSendCount = 0;
bool ackR_F = true; // 응답패킷(ACK)을 수신한 호스트에서 그에대한 응답패킷(ACK_R) 수신여부


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
};
DeviceObject myDeviceObject;


//부저
bool Buzzer_F = false;
unsigned long Buzzer_StartTime;
unsigned long Buzzer_OffsetTime;
const long Buzzer_DELAYTIME = 6000; // Buzzer 유지시간(ms)


//const uint8_t replyMaxReqCount = 3; //응답 요청횟수:최소 1~ 변경가능
////const int replyPeriod = 1000 * replyMaxReqCount;
//const int replyPeriod = 1000;
//unsigned long replyStartTime;
//unsigned long replyTimeOffset;
//uint8_t replyReqCount = 0; //카메라로부터 응답패킷 수신대기상태에서 수신안되는 경우 재요청횟수
//unsigned long replyQuotient; //재요청시간 계산용(최소:0~ 최대:hostPeriod/hostMaxReqCount )


const uint8_t ReplyMaxReqCount = 3;
uint8_t ReplyReqCount = 0;
unsigned long ReplyPreviousTime;
unsigned long Reply_OffsetTime;
const int ReplyPeriod = 1000;


//CDS
int CDSValue = 0;
unsigned long CDSPreviousTime=0;
const long CDSPeriod = 10000; // CDS 체크 간격(ms) : 10초
unsigned long CDS_OffsetTime=0;


//차단기 플래그
bool GATE_UP_F = false;
unsigned long GATEUP_StartTime;
unsigned long GATEUP_OffsetTime;
uint16_t GATE_UP_DELAYTIME = 500; //처리유지시간(ms)


bool GATE_DOWN_F = false;
unsigned long GATEDOWN_StartTime;
unsigned long GATEDOWN_OffsetTime;
uint16_t GATE_DOWN_DELAYTIME = 500; //처리유지시간(ms)


bool TRIGGER_F = false;
unsigned long TRIGGER_StartTime;
unsigned long TRIGGER_OffsetTime;
uint16_t TRIGGER_DELAYTIME = 500; //처리유지시간(ms)


//핀 설정
void SetPinMode()
{
  pinMode(DC5V_Relay, OUTPUT);
  pinMode(DC12V_Relay, OUTPUT);
  pinMode(Trigger_Relay, OUTPUT);
  pinMode(GateUp_Relay, OUTPUT);
  pinMode(GateDn_Relay, OUTPUT);
  pinMode(Buzzer_Relay, OUTPUT);
  pinMode(SP1_Relay, OUTPUT);
  pinMode(SP2_Relay, OUTPUT);

  digitalWrite(DC5V_Relay, HIGH);
  digitalWrite(DC12V_Relay, HIGH);
  digitalWrite(GateUp_Relay, HIGH);
  digitalWrite(GateDn_Relay, HIGH);
  digitalWrite(Buzzer_Relay, HIGH);
  digitalWrite(SP1_Relay, HIGH);
  digitalWrite(SP2_Relay, HIGH);
  

  pinMode(interruptShockPin, INPUT_PULLUP);
  delay(100);
  attachInterrupt(digitalPinToInterrupt(interruptShockPin), interrupt, RISING);
}

//인터럽트 처리
void interrupt() 
{
    //충격센서
    if(interrupt_F == false) {
      interrupt_F = true;
    }

    //부저
    Buzzer_F = true;
    Buzzer_StartTime = millis();
    digitalWrite(Buzzer_Relay, LOW); //부저켜기
}


void setup() 
{
    Serial.begin(115200);
  
    SetPinMode();
    
    //EEPROM 공장초기화 상태일 경우 파라미터 값으로 세팅하고,
    //이미 세팅된 값이 있을 경우, 세팅된 EEPROM의 값을 가져온다.
    EEPROM_Init(NO, MAC, IP, SM, GW, DNS, HostIP, HostPort);
  
    Ethernet.begin(MAC, IP, DNS, GW, SM); // Wiz550io 세팅
  
  
    char tmpBuff[UDP_TX_PACKET_MAX_SIZE];
    uint8_t MAC_temp[6];
    Ethernet.MACAddress(MAC_temp);      //Wiz550io 맥어드레스 가져오기
    IPAddress ip = Ethernet.localIP();  //Wiz550io IP주소 가져오기
    TcpServer.begin();                  // TCP서버(command)
    UdpServer.begin(localUDPPort);      // UDP서버(command)
    
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
  
    sprintf(tmpBuff, "[HOST IP]%d.%d.%d.%d", myDeviceObject.rh1,myDeviceObject.rh2,myDeviceObject.rh3,myDeviceObject.rh4);
    Serial.print(tmpBuff);
    Serial.print(":");
    Serial.println(HostPort);
#endif
  
    Serial.print("Jawootek LPR Board");
    Serial.println(" Ver 1.0");
}


void loop() {
  
    //UDP Server(50001)
    int packetSize = UdpServer.parsePacket();
    if (packetSize) {
        char recvTempBuff[UDP_TX_PACKET_MAX_SIZE];
        memset(recvTempBuff, 0, UDP_TX_PACKET_MAX_SIZE);
        
        recvProtocol = ProtocolUDP;
        UdpServer.read(recvTempBuff, UDP_TX_PACKET_MAX_SIZE);
        ProcPacket(recvTempBuff);
    }
  
    //TCP Server(50001)
    client = TcpServer.available();
    if (client) {
        if (client.available() > 0) {
            char recvTempBuff[UDP_TX_PACKET_MAX_SIZE];
            memset(recvTempBuff, 0, UDP_TX_PACKET_MAX_SIZE);
            
            recvProtocol = ProtocolTCP; //TCP
            client.read(recvTempBuff, UDP_TX_PACKET_MAX_SIZE);
            ProcPacket(recvTempBuff);
        }
    }


    //호스트로부터 수신한 패킷에 대하여 응답패킷 전송
    //위 ProcPacket() 패킷처리 -> 응답패킷 호스트로 전송 -> 호스트로부터 ACK_R 수신(미수신시 최대 3회 응답패킷 재전송)
    if(ackR_F == false)
    {
        if( ReplyReqCount < ReplyMaxReqCount-1 ) {
            if(millis() < ReplyPreviousTime) { //overflow
                  Reply_OffsetTime = 4294967294 - ReplyPreviousTime;
                  ReplyPreviousTime = 0;
            }
            if(millis() - ReplyPreviousTime > ReplyPeriod - Reply_OffsetTime)    {
                Reply_OffsetTime=0;
                ReplyPreviousTime = millis();
                
                SendReply(); //응답패킷 전송
                ReplyReqCount++;
            }
        }

        
    }


    //릴레이 처리
    if(GATE_UP_F == true || GATE_DOWN_F == true || TRIGGER_F == true ) {
        RelayProc();
    }


    //CDS처리 : CDSPeriod 시간마다 처리함
    if(millis() < CDSPreviousTime) { //overflow
          CDS_OffsetTime = 4294967294 - CDSPreviousTime;
          CDSPreviousTime = 0;
    }
    if(millis() - CDSPreviousTime > CDSPeriod - CDS_OffsetTime)    {
        CDS_OffsetTime=0;
        CDSPreviousTime = millis();
        
        CDSProc();
    }


    //인터럽트 처리(충격센서)
    if( interrupt_F == true) {
        char shockPacket[UDP_TX_PACKET_MAX_SIZE];
        memset(shockPacket, 0, UDP_TX_PACKET_MAX_SIZE);
        shockPacket[0] = 0x02;
        sprintf(&shockPacket[1], "%d_Shock", myDeviceObject.no);
        shockPacket[strlen(shockPacket)] = 0x03;
        
        ShockProc(shockPacket);
  
        interrupt_F = false;
    }

  
    //부저 처리
    if(Buzzer_F == true) {
        BuzzerProc();
    }
}
