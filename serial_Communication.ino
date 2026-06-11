void CommnunicationBeetle(){
  Serial.println("READ");
  if(toSubSerial.available() > 0){
    lastBeetleMs = millis();
    String command = toSubSerial.readStringUntil('\n');

    if (command.length() == 0) return;

    char cmd = command[0];

    if(cmd == 'W'){
      Serial.println("Beetle Init Success");
      toSubSerial.println("W");
    }
    else if(cmd == 'R'){
      Serial.println("Beetle Reset Success");
    }
    else if(cmd == 'T'){
      lastBeetleRawPacket = command;

      bool fmtOk = (command.length() >= 23 &&
                    command[1] == '1' && command[2] == ':' &&
                    command[7] == '_' &&
                    command[8] == 'T' && command[9] == '2' && command[10] == ':' &&
                    command[15] == '_' &&
                    command[16] == 'T' && command[17] == '3' && command[18] == ':');

      if (!fmtOk) {
        packetFormatErrorCount++;
        Serial.println("[UART] WARN malformed T packet: " + command);
        return;
      }

      Serial.println(command);
      tag1 = command.substring(3, 7);
      tag2 = command.substring(11, 15);
      tag3 = command.substring(19, 23);

      Serial.println("TAG1 = " + tag1);
      Serial.println("TAG2 = " + tag2);
      Serial.println("TAG3 = " + tag3);

      // [임시/테스트] G2P2 카드를 MMMM 토글 카드처럼 취급. 운영 배포 전 반드시 제거할 것.
      bool hasToggleCard = (tag1 == "MMMM" || tag2 == "MMMM" || tag3 == "MMMM" ||
                            tag1 == "G2P2" || tag2 == "G2P2" || tag3 == "G2P2");

      if (hasToggleCard) {
          // 토글 디바운스: 직전 토글(모터 동작 완료) 후 일정 시간은 무시.
          // 카드를 계속 올려두거나 버퍼에 쌓인 중복 MMMM 패킷으로 인한 activate<->ready
          // 연속 뒤집힘을 방지한다. 쿨다운은 ActivateFunc/ReadyFunc 완료 시점부터 측정.
          static unsigned long lastToggleMs = 0;
          const unsigned long TOGGLE_COOLDOWN_MS = 2000;
          if (lastToggleMs != 0 && (millis() - lastToggleMs) < TOGGLE_COOLDOWN_MS) {
              tag1 = ""; tag2 = ""; tag3 = "";
              tagState[0] = false; tagState[1] = false; tagState[2] = false;
              while (toSubSerial.available()) toSubSerial.read();
              return;
          }

          String deviceState = (String)(const char*)my["device_state"];
          // 로컬 동작을 먼저 수행해 네트워크가 느려도 장치가 즉시 반응하게 하고,
          // 서버 전송(블로킹 HTTP)은 그 다음에 한다.
          if (deviceState == "ready") {
              my["device_state"] = "activate";
              ActivateFunc();
              has2wifi.Send((String)(const char*)my["device_name"], "device_state", "activate");
          } else if (deviceState == "activate") {
              my["device_state"] = "ready";
              ReadyFunc();
              has2wifi.Send((String)(const char*)my["device_name"], "device_state", "ready");
          }
          lastToggleMs = millis(); // 동작 완료 후 시점 기준으로 쿨다운 시작
          tag1 = "";
          tag2 = "";
          tag3 = "";
          tagState[0] = false;
          tagState[1] = false;
          tagState[2] = false;
          ResetBeetleErrorCounters();
          while (toSubSerial.available()) toSubSerial.read(); // 동작 중 쌓인 중복 MMMM 패킷 폐기
          return;
      }

      tagState[0] = PlayerDetector(tag1);
      tagState[1] = PlayerDetector(tag2);
      tagState[2] = PlayerDetector(tag3);

      ResetBeetleErrorCounters();
    }
    else if(cmd == 'B'){
      Serial.println(command);
    }
    else if(cmd == 'M'){
      ESP.restart();
    }
    else {
      invalidCmdCount++;
      Serial.println("[UART] WARN unknown command '" + String(cmd) + "'");
    }
  }
  while(toSubSerial.available()){
    toSubSerial.read();
  }
}

bool PlayerDetector(String playerNum)
{
  if (playerNum.length() < 4) {
    tagParseErrorCount++;
    Serial.println("[UART] WARN tag length < 4: '" + playerNum + "'");
    return false;
  }

  if (playerNum[3] == '0')
    return false;

  if (playerNum == "G2P1") return true;

  if (!playerNum.startsWith("G9P")) {
    tagParseErrorCount++;
    Serial.println("[UART] WARN unknown tag prefix: '" + playerNum + "'");
    return false;
  }

  char roleNumber = playerNum[3];
  if (roleNumber == '1') return false;
  if (roleNumber == '2') return false;
  if (roleNumber >= '3' && roleNumber <= '8') return true;

  tagParseErrorCount++;
  Serial.println("[UART] WARN unsupported G9P tag: '" + playerNum + "'");
  return false;
}
