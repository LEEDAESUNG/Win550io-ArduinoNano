
void BuzzerProc() 
{
    digitalWrite(Relay6, HIGH);

    if(millis() < BuzzerStartTime) //overflow
    {
      BuzzerTimeOffset = 4294967294 - BuzzerStartTime;
      BuzzerStartTime = 0;
    }
    if(millis() - BuzzerStartTime > BuzzerPeriod - BuzzerTimeOffset)
    {
        BuzzerTimeOffset = 0;
        Buzzer_F = false;
        digitalWrite(Relay6, LOW);
    }
}
