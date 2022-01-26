
void ShockProc(char *msg)
{
    int retryHostConnectCount = 3; //호스트접속 실패시 3회 재접속시도
    while(retryHostConnectCount > 0 && HostProc(msg) == false){
        retryHostConnectCount--;
    }
}
