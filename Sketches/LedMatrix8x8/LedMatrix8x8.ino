#include <Arduino.h>
#include <LedControl.h>   //LedControl library: https://www.electronoobs.com/ledcontrol.php
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// WiFi access data
const char *ssid = "WIFI_NAME";
const char *password = "WIFI_PASSWORD";

// Private token of your Gitlab user
const char *private_token = "GITLAB_USER_TOKEN";

// Gitlab HTTPS certificate SHA-1 fingerprint
const char *https_fingerprint = "E3 D3 9A 20 01 71 75 3B 8F F3 5A 8B C1 A9 9B E9 65 52 E1 64";

// Pipelines to monitor
const String url_pipeline[] = {"353464363"};

int cols = 8;
int rows = 8;
int count = 10;

// How often to poll for updates
const int updateDelayMillis = 10000;

// How long to wait for response until switching to "error"
const int updateTimeoutMillis = 20000;

long lastUpdateMillis = 0;

enum PipelineState
{
  SUCCESS,
  RUNNING,
  FAILED,
  UNKNOWN,
  ERROR
};

PipelineState currentState[] = {UNKNOWN};
 
LedControl lc = LedControl(13,14,15,0); //DATA | CLK | CS/LOAD | number of matrices

void setup() {
  
  //..........Initializing matrix..............//
    lc.shutdown(0,false);
    /* Set the brightness to a medium values */
    lc.setIntensity(0,3);
    /* and clear the display */
    lc.clearDisplay(0);
  //...........................................//

  //..........Connecting to WiFi...............//
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  //...........................................//

  Serial.println("Starting.");

  for(int i = 0; i < count; i++) {
    currentState[i] = UNKNOWN;
  }
}

void loop()
{
  int counter = 0;
  for(int col = 0; col < cols; col++) {
    for(int row = 0; row < rows; row++) {
      switch (currentState[counter++])
      {
      case UNKNOWN:
        lc.setLed(0,row,col,false);
        break;
       
      case ERROR:
        lc.setLed(0,row,col,false);
        break;
    
      case SUCCESS:
        lc.setLed(0,row,col,true);
        break;
    
      case FAILED:
        lc.setLed(0,row,col,false);
        break;
    
      case RUNNING:
        lc.setLed(0,row,col,false);
        break;
      }
      if(counter > count) {
        lc.setLed(0,row,col,false);
      }
    }
  }
  delay(1000);
  update();
}

void update()
{
    
  if (millis() <= (lastUpdateMillis + updateDelayMillis))
    return;

  if (WiFi.status() == WL_CONNECTED)
  {
    for(int i = 0; i < count; i++) {

      Serial.print("Updating pipeline status... ");

      String url = "https://gitlab.com/api/v4/projects/" + url_pipeline[i] + "/pipelines?ref=master&per_page=1";
  
      HTTPClient http;
      http.setTimeout(updateTimeoutMillis);
      http.begin(url, https_fingerprint);
      http.addHeader("PRIVATE-TOKEN", private_token);

      int returnCode = http.GET();
      if (returnCode != 200)
      {
        Serial.print("ERROR: ");
        Serial.print(returnCode);
        Serial.print(" (");
        Serial.print(http.errorToString(returnCode));
        Serial.print("): ");
        Serial.print(http.getString());
  
        currentState[i] = ERROR;
      }else{

        Serial.print("update retrieved. ");
        
        StaticJsonDocument<300> doc;
        deserializeJson(doc, http.getString());
      
        const char *pipelineStatus = doc[0]["status"];
        Serial.print("Pipeline status for " + url_pipeline[i] + ": ");
        Serial.print(pipelineStatus);
  
        if (strcmp(pipelineStatus, "success") == 0)
        {
          currentState[i] = SUCCESS;
        }
        else if (strcmp(pipelineStatus, "failed") == 0)
        {
          currentState[i] = FAILED;
        }
        else if (strcmp(pipelineStatus, "running") == 0)
        {
          currentState[i] = RUNNING;
        }
        else
        {
          currentState[i] = UNKNOWN;
        }
      }
    }
      Serial.println("");
  }
  lastUpdateMillis = millis();
}
