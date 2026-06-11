void DataChanged()
{
  String myJson;
  serializeJson(my, myJson);
  Serial.println(myJson);
  static StaticJsonDocument<1000> cur;
  if((String)(const char*)my["game_state"] != (String)(const char*)cur["game_state"]){
    if((String)(const char*)my["game_state"] == "setting"){
        SettingFunc();
    }
    else if((String)(const char*)my["game_state"] == "ready"){
        ReadyFunc();
    }
    else if((String)(const char*)my["game_state"] == "activate"){
        if((String)(const char*)my["device_state"] != "fake" && ptrCurrentMode != TagCount){
            ActivateFunc();
        }
    }
    else if((String)(const char*)my["game_state"] == "escape"){
        EscapeClose();
        GameTimer.disable(gameTimerId);
        ptrCurrentMode = WaitFunc;
    }
  }
  if (my["brightness"].as<int>() != cur["brightness"].as<int>())
    UpdateBrightness();
  if((String)(const char*)my["device_state"] != (String)(const char*)cur["device_state"]){
    if((String)(const char*)my["device_state"] == "player_win"){
        AllNeoOn(BLUE);
        EscapeClose();
    }
    else if((String)(const char*)my["device_state"] == "fake"){
        AllNeoOn(PURPLE);
        EscapeClose();
    }
    else if((String)(const char*)my["device_state"] == "activate"){
        if((String)(const char*)my["game_state"] == "activate" && ptrCurrentMode != TagCount){
            ActivateFunc();
        }
    }
    else if((String)(const char*)my["device_state"] == "github"){
        Serial.println("[OTA] OTA 업데이트 요청 수신");
        ota.check();
    }
  }
  cur = my;
}

void WaitFunc(){
}

void SettingFunc(void)
{
    Serial.println("SETTING");
    digitalWrite(RELAY_PIN, HIGH);
    AllNeoOn(WHITE);
    EscapeClose();
    GameTimer.disable(gameTimerId);
    has2wifi.Send((String)(const char*)my["device_name"], "game_state", "ready");
    has2wifi.Send((String)(const char*)my["device_name"], "device_state", "ready");
    ReadyFunc();
}

void ActivateFunc(void){
    Serial.println("ACTIVATE");
    Mp3PlayLargeFolder(1, VE1);
    AllNeoOn(YELLOW);
    EscapeOpen();
    GameTimer.enable(gameTimerId);
    while (toSubSerial.available()) toSubSerial.read(); // EscapeOpen 블로킹 중 쌓인 RX 버퍼 비우기
    ptrCurrentMode = TagCount;
}

void ReadyFunc(void){
    Serial.println("READY");
    digitalWrite(RELAY_PIN, HIGH);
    AllNeoOn(RED);
    EscapeClose();
    while (toSubSerial.available()) toSubSerial.read();
    ptrCurrentMode = WaitFunc;
    GameTimer.enable(gameTimerId); // ready 상태에서도 500ms로 카드 폴링 (MMMM 토글 빠른 인식)
}
