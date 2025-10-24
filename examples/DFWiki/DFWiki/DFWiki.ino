#include <M5Unified.h>

char buff[4]={0x80,0x06,0x03,0x77};
unsigned char data[11]={0};

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(115200);
  Serial2.begin(9600,SERIAL_8N1,32,33);
}

void loop() {
  Serial2.print(buff);
  while(1) {
    if(Serial2.available()>0)//Determine whether there is data to read on the serial 
    {
      delay(50);
      for(int i=0;i<11;i++)
      {
        data[i]=Serial2.read();
      }
      unsigned char Check=0;
      for(int i=0;i<10;i++)
      {
        Check=Check+data[i];
      }
      Check=~Check+1;
      if(data[10]==Check)
      {
        if(data[3]=='E'&&data[4]=='R'&&data[5]=='R')
        {
          Serial.println("Out of range");
        }
        else
        {
          float distance=0;
          distance=(data[3]-0x30)*100+(data[4]-0x30)*10+(data[5]-0x30)*1+(data[7]-0x30)*0.1+(data[8]-0x30)*0.01+(data[9]-0x30)*0.001;
          Serial.print("Distance = ");
          Serial.print(distance,3);
          Serial.println(" M");
        }
      }
      else
      {
        Serial.println("Invalid Data!");
      }
    }
    delay(20);
  }
}
