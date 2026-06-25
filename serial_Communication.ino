void CommnunicationBeetle(){
  Serial.println("READ");
  while(toSubSerial.available() > 0){
    lastBeetleMs = millis();
    String command = toSubSerial.readStringUntil('\n');

    if (command.length() == 0) continue;

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
        continue;
      }

      Serial.println(command);
      tag1 = command.substring(3, 7);
      tag2 = command.substring(11, 15);
      tag3 = command.substring(19, 23);

      Serial.println("TAG1 = " + tag1);
      Serial.println("TAG2 = " + tag2);
      Serial.println("TAG3 = " + tag3);

      bool hasToggleCard = (tag1 == "MMMM" || tag2 == "MMMM" || tag3 == "MMMM");

      if (hasToggleCard) {
          PerformToggle();
          tag1 = ""; tag2 = ""; tag3 = "";
          tagState[0] = false; tagState[1] = false; tagState[2] = false;
          ResetBeetleErrorCounters();
          return;
      }

      tagState[0] = PlayerDetector(tag1);
      tagState[1] = PlayerDetector(tag2);
      tagState[2] = PlayerDetector(tag3);

      ResetBeetleErrorCounters();
    }
    else if(cmd == 'E'){
      // side effect from MMMM card, ignore
    }
    else if(cmd == 'M'){
      PerformToggle();
      return;
    }
    else if(cmd == 'B'){
      Serial.println(command);
    }
    else {
      invalidCmdCount++;
      Serial.println("[UART] WARN unknown command '" + String(cmd) + "'");
    }
  }
}

void PerformToggle() {
  static unsigned long lastToggleMs = 0;
  const unsigned long TOGGLE_COOLDOWN_MS = 2000;
  if (lastToggleMs != 0 && (millis() - lastToggleMs) < TOGGLE_COOLDOWN_MS) {
    while (toSubSerial.available()) toSubSerial.read();
    return;
  }

  String deviceState = (String)(const char*)my["device_state"];
  if (deviceState == "ready") {
    my["device_state"] = "activate";
    ActivateFunc();
    has2wifi.Send((String)(const char*)my["device_name"], "device_state", "activate");
  } else if (deviceState == "activate") {
    my["device_state"] = "ready";
    ReadyFunc();
    has2wifi.Send((String)(const char*)my["device_name"], "device_state", "ready");
  }
  lastToggleMs = millis();
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

  if (!playerNum.startsWith("G9P")) {
    tagParseErrorCount++;
    Serial.println("[UART] WARN unknown tag prefix: '" + playerNum + "'");
    return false;
  }

  // 역할 구분: G9P1=술래, G9P2=유령, G9P3~G9P9=생존자(탈출 카운트 대상)
  char roleNumber = playerNum[3];
  if (roleNumber == '1') return false;                     // 술래
  if (roleNumber == '2') return false;                     // 유령
  if (roleNumber >= '3' && roleNumber <= '9') return true;  // 생존자

  tagParseErrorCount++;
  Serial.println("[UART] WARN unsupported G9P tag: '" + playerNum + "'");
  return false;
}
