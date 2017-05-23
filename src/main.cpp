
#include "Arduino.h"
#include <SoftwareSerial.h>
#define _baudrate   115200
//*-- IoT Information
#define SSID "ADLINKTECH"
#define PASS "adlink6166"
#define IP "184.106.153.149" // ThingSpeak IP Address: 184.106.153.149
#define _SCK 4
#define _CS 2
#define _SO 6
#define LED 13

// 使用 GET 傳送資料的格式
// GET /update?key=[THINGSPEAK_KEY]&field1=[data 1]&filed2=[data 2]...;
String GET = "GET /update?key=CYMS8PAN6RKBN70V";
static volatile uint8_t is_tc_open;
void sendDebug(String cmd);
void Loding();
void SentOnCloud(String T, String H);
float max6675_getCelsius();
void software_Reset();

void setup()
{
    pinMode(LED, OUTPUT);
    Serial.begin( _baudrate );
    Serial1.begin( _baudrate );

    sendDebug("AT");
    Loding();

    sendDebug("AT+CWMODE=1");
    Loding();

    String cmd="AT+CWJAP=\"";
    cmd+=SSID;
    cmd+="\",\"";
    cmd+=PASS;
    cmd+="\"";
    sendDebug(cmd);
    Loding();

    pinMode( _SCK, OUTPUT );
    pinMode( _SO, INPUT );
    pinMode( _CS, OUTPUT );
    digitalWrite( _CS, HIGH );
}
void loop()
{
  if( is_tc_open )
  {
    static uint8_t tc_check;
    Serial.println( "TC is Open !");

    // 每 5 秒鐘重新確認 TC 是否已重新連接上
    if( tc_check ++ > 3 )
    {
      max6675_getCelsius();
      tc_check = 0;
    }
  }
  else
  {
    float C = max6675_getCelsius();
//    float F = max6675_getFahrenheit();
    delay(1000);   // 60 second
    SentOnCloud( String(5), String(C) );
    Serial.print( C );
    Serial.println(" C");
  }

  delay(1000);
}

void SentOnCloud( String T, String H )
{
    // 設定 ESP8266 作為 Client 端
    String cmd = "AT+CIPSTART=\"TCP\",\"";
    cmd += IP;
    cmd += "\",80";
    sendDebug(cmd);
    if( Serial1.find( "Error" ) )
    {
        Serial.print( "RECEIVED: Error\nExit1" );
        return;
    }
    cmd = GET + "&field1=" + T + "&field2=" + H +"\r\n";
    Serial1.print( "AT+CIPSEND=" );
    Serial1.println( cmd.length() );
    if(Serial1.find( ">" ) )
    {
        Serial.print(">");
        Serial.print(cmd);
        Serial1.println(cmd);
    }
    else
    {
        //debug.println( "AT+CIPCLOSE" );
    }
    byte count = 0;
    while(1)
    {
      if( Serial1.find("OK") )
      {
          Serial.println( "RECEIVED: OK" );
          digitalWrite(LED, HIGH);
          delay(400);
          digitalWrite(LED, LOW);
          count = 0;
          break;
      }
      if(count >= 3)
      {
        Serial.println("Auto Reboot");
        software_Reset();
      }
      delay(300);
      Serial.println("Waiting for rececived ");
      count++;
    }


}

void Loding(){
    for (int timeout=0 ; timeout<5 ; timeout++)
    {
      if(Serial1.find("OK"))
      {
          Serial.println("RECEIVED: OK");
          digitalWrite(LED, HIGH);
          delay(400);
          digitalWrite(LED, LOW);
          break;
      }
        Serial.println("Wifi Loading...");
        delay(400);
    }
}

void sendDebug(String cmd)
{
    Serial.print("SEND: ");
    Serial.println(cmd);
    Serial1.println(cmd);
}

float max6675_getCelsius()
{
  uint16_t t_c = 0;

   //初始化溫度轉換
  digitalWrite( _CS, LOW );
  delay(2);
  digitalWrite( _CS, HIGH );
  delay(220);

  // 開始溫度轉換
  digitalWrite( _CS, LOW );

  // 15th-bit, dummy bit
  digitalWrite( _SCK, HIGH );
  delay(1);
  digitalWrite( _SCK, LOW );

     // 14th - 4th bits, temperature valuw
  for( int i = 11; i >= 0; i-- )
  {
      digitalWrite( _SCK, HIGH );
      t_c += digitalRead( _SO ) << i;
      digitalWrite( _SCK, LOW );
  }

  // 3th bit, 此位元可以用來判斷 TC 是否損壞或是開路
  // Bit D2 is normally low and goes high when the therometer input is open.
  digitalWrite( _SCK , HIGH );
  is_tc_open = digitalRead( _SO );
  digitalWrite( _SCK, LOW );

  // 2nd - 1st bits,
  // D1 is low to provide a device ID for the MAX6675 and bit D0 is three-state.
  for( int i = 1; i >= 0; i-- )
  {
      digitalWrite( _SCK, HIGH );
      delay(1);
      digitalWrite( _SCK, LOW );
  }

  // 關閉 MAX6675
  digitalWrite( _CS, HIGH );

  return (float)(t_c * 0.25);
}

void software_Reset()
{
  asm volatile ("  jmp 0");
}
