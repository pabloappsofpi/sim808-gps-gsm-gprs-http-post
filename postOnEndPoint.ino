#include <SoftwareSerial.h>
SoftwareSerial sim808(10, 11); // RX, TX

enum _parseState {
  PS_DETECT_MSG_TYPE,

  PS_IGNORING_COMMAND_ECHO,

  PS_HTTPACTION_TYPE,
  PS_HTTPACTION_RESULT,
  PS_HTTPACTION_LENGTH,

  PS_HTTPREAD_LENGTH,
  PS_HTTPREAD_CONTENT
};


unsigned long lastActionTime = 0;

byte parseState = PS_DETECT_MSG_TYPE;
char buffer[80];
byte pos = 0;

int contentLength = 0;

void resetBuffer() {
  memset(buffer, 0, sizeof(buffer));
  pos = 0;
}

void sendsim808(const char* msg, int waitMs = 500) {
  sim808.println(msg);
  while(sim808.available()) {
    parseATText(sim808.read());
  }
  delay(waitMs);
}

void setup()
{
  sim808.begin(9600);
  Serial.begin(9600);
  
 

}
void postOnEndpoint(){
  String latitude ="10";
  String longitude = "-7.0012";
  sendsim808("AT+SAPBR=3,1,\"APN\",\"vodafone\"");  
  sendsim808("AT+SAPBR=1,1",3000);
  sendsim808("AT+HTTPINIT");  
  sendsim808("AT+HTTPPARA=\"CID\",1");
  String url = "AT+HTTPPARA=\"URL\",\"us-central1-electronics-academy.cloudfunctions.net/getGPS?longitude="+longitude+"&latitude="+latitude+"\"";
    const char* urlChar = url.c_str();

  sendsim808(urlChar);
  //=0 get post=1
  sendsim808("AT+HTTPACTION=1");
delay(5000);
  sim808.flush();
}
void loop()
{ 
    
   postOnEndpoint();
  
  
  while(sim808.available()) {
    parseATText(sim808.read());
  }

  delay(3000);
}

void parseATText(byte b) {

  buffer[pos++] = b;

  if ( pos >= sizeof(buffer) )
    resetBuffer(); // just to be safe

    //Serial.println(buffer);*/

  switch (parseState) {
  case PS_DETECT_MSG_TYPE: 
    {
      if ( b == '\n' )
        resetBuffer();
      else {        
        if ( pos == 3 && strcmp(buffer, "AT+") == 0 ) {
          parseState = PS_IGNORING_COMMAND_ECHO;
        }
        else if ( b == ':' ) {
          //Serial.print("Checking message type: ");
          //Serial.println(buffer);

          if ( strcmp(buffer, "+HTTPACTION:") == 0 ) {
            Serial.println("Received HTTPACTION");
            parseState = PS_HTTPACTION_TYPE;
          }
          else if ( strcmp(buffer, "+HTTPREAD:") == 0 ) {
            Serial.println("Received HTTPREAD");            
            parseState = PS_HTTPREAD_LENGTH;
          }
          resetBuffer();
        }
      }
    }
    break;

  case PS_IGNORING_COMMAND_ECHO:
    {
      if ( b == '\n' ) {
        Serial.print("Ignoring echo: ");
        Serial.println(buffer);
        parseState = PS_DETECT_MSG_TYPE;
        resetBuffer();
      }
    }
    break;

  case PS_HTTPACTION_TYPE:
    {
      if ( b == ',' ) {
        Serial.print("HTTPACTION type is ");
        Serial.println(buffer);
        parseState = PS_HTTPACTION_RESULT;
        resetBuffer();
      }
    }
    break;

  case PS_HTTPACTION_RESULT:
    {
      if ( b == ',' ) {
        Serial.print("HTTPACTION result is ");
        Serial.println(buffer);
        parseState = PS_HTTPACTION_LENGTH;
        resetBuffer();
      }
    }
    break;

  case PS_HTTPACTION_LENGTH:
    {
      if ( b == '\n' ) {
        Serial.print("HTTPACTION length is ");
        Serial.println(buffer);
        
        // now request content
        sim808.print("AT+HTTPREAD=0,");
        sim808.println(buffer);
        
        parseState = PS_DETECT_MSG_TYPE;
        resetBuffer();
      }
    }
    break;

  case PS_HTTPREAD_LENGTH:
    {
      if ( b == '\n' ) {
        contentLength = atoi(buffer);
        Serial.print("HTTPREAD length is ");
        Serial.println(contentLength);
        
        Serial.print("HTTPREAD content: ");
        
        parseState = PS_HTTPREAD_CONTENT;
        resetBuffer();
      }
    }
    break;

  case PS_HTTPREAD_CONTENT:
    {
      // for this demo I'm just showing the content bytes in the serial monitor
      Serial.write(b);
      
      contentLength--;
      
      if ( contentLength <= 0 ) {

        // all content bytes have now been read

        parseState = PS_DETECT_MSG_TYPE;
        resetBuffer();
        
        Serial.print("\n\n\n");
        
      }
    }
    break;
  }
}
