

//부저처리
void BuzzerProc()
{
    if(millis() < Buzzer_StartTime) //overflow
    {
      Buzzer_OffsetTime = 4294967294 - Buzzer_StartTime;
      Buzzer_StartTime = 0;
    }
    if(millis() - Buzzer_StartTime > Buzzer_DELAYTIME - Buzzer_OffsetTime) // Buzzer_DELAYTIME 시간이후 부저끄기
    {
        Buzzer_OffsetTime = 0;
        Buzzer_F = false;
        digitalWrite(Buzzer_Relay, HIGH);
    }
}
