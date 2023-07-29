#include "Transponder.h"

typedef  unsigned char u8;
WiFiServer server(9000);
bool confirmRequestPending{false};
xSemaphoreHandle xSemap;
bool deviceConnected = false;
string ble_data;
    
void Transponder::init()
{
    pinMode(15, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(35, INPUT);
    pinMode(36, INPUT);
    delay(100);
    digitalWrite(15, 1);
    digitalWrite(5, 1);

    distep.y_pos = pos_y;
    distep.rect_pos1 = rect1;
    distep.rect_pos2 = rect2;

    if (EEPROM.read(state_addr) == 0) {
        distep.state = 0;
        state_on = true;
    } else if (EEPROM.read(state_addr) == 1) {
        distep.state = 1;
        state_on = true;
    }
    info();
}

void Transponder::run(MyCobotBasic &myCobot)
{
    init();
    EXIT = false;
    xSemap = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(TaskReadData, "TaskReadData", 10000, this, 1, NULL, 1);
    EventResponse(myCobot);
}


void Transponder::TaskReadData(void *p)
{
    Transponder *trans = reinterpret_cast<Transponder *>(p);
    trans->readData();
}

/*
 * Function: Check whether the header of the serial port wifi or bluetooth return message is fe fe
 */
bool Transponder::checkHeader(vector<unsigned char> &v_data)
{
    if (v_data[0] == HEADER && v_data[1] == HEADER)
        return true;
    else {
        v_data.erase(v_data.begin(), v_data.begin() + 1);
    }
    return false;
}

/*
 * Function:Handle data segmentation sticky packet problem
 */
bool Transponder::HandleStickyPackets(vector<unsigned char> &temp,
                                      vector<unsigned char> &v_data)
{
    //Get the length of the protocol to determine the terminator fa
    vector<unsigned char>::iterator it = temp.begin() + 2 + temp[2];
    vector<unsigned char>::iterator it_vdata = v_data.begin() + 2 + v_data[2];
    int len = temp.size();
    //now protocol,latest len is 26,like fe fe 88(88 is error)-->delete three, meanwhile need len-->latest is fa
    if (temp[2] < 30 && *it == END) {
        temp.erase(it + 1, temp.end());
        v_data.erase(v_data.begin(), it_vdata + 1);
        return true;
    } else {
         v_data.erase(v_data.begin() + 1, v_data.begin() + 2);
    }
    return false;
}

/*
 * Function:Process the data returned by Atom, no need for FF FF
 */
bool Transponder::HandleAtomData(vector<unsigned char> &v_data)
{
    vector<unsigned char>::iterator it = v_data.begin();
    for (it; it < v_data.end() - 1; it++) {
        if (*it == HEADER && *(it + 1) == HEADER) {
            v_data.erase(v_data.begin(), it);
            return true;
        }
    }
    return false;
}

/*
 * Function:Response to button events
 */
void Transponder::EventResponse(MyCobotBasic &myCobot)
{
    while (!EXIT) {
        M5.update();
        if (M5.BtnA.wasReleased()) {
            distep.state += 1;
            distep.MenuChoice();
            M5.update();
        } else if (M5.BtnB.wasReleased()) {
            distep.state -= 1;
            distep.MenuChoice();
            M5.update();
        } else if (M5.BtnC.wasReleased() || state_on) {
            //write data
            EEPROM.write(state_addr, byte(distep.state));
            //save changed data
            EEPROM.commit();
            state_on = false;
            M5.update();
            switch ((transponder_mode = (enum MODE)distep.state)) {
                case Uart: {
                    rFlushSerial();
                    data_power = true;
                    connect_ATOM(myCobot);
                    while (true) {
                        M5.update();
                        if (M5.BtnC.wasReleased()) {
                            myCobot.jogStop();
                            data_power = false;
                            info();
                            break;
                        }
                        //Sleep for 1ms per cycle to reduce the chip heating rate as much as possible
                        delay(1);
                    }
                }
                break;
                case Wlan: {
                    rFlushSerial();
                    data_power = true;
                    ConnectingInfo();
                    CreateWlanServer();
                    while (true) {
                        M5.update();
                        if (!wlan_uart) {
                            WlanTransponder();
                        }
                        if (is_timeout) {
                            TimeOutInfo();
                            is_timeout = false;
                            delay(500);
                            if (!wlan_uart)
                                ConnectedInfo();
                            else
                                ConnectFailedInfo(false);
                        }
                        //No click event response on timeout
                        if (M5.BtnC.wasReleased() && !is_timeout) {
                            myCobot.jogStop();
                            data_power = false;
                            info();
                            break;
                        } else if (M5.BtnA.wasReleased() && !is_timeout) {
                            if (!wlan_uart) {
                                WiFi.disconnect();
                                server.stop();
                            }
                            ConnectingInfo();
                            CreateWlanServer();
                        }
                        delay(1);
                    }
                }
                WiFi.disconnect();
                server.stop();
                break;
                case Bt: {
                    rFlushSerial();
                    data_power = true;
                    CreateBTServer();
                    BTWaitInfo();
                    while (true) {
                        m5.update();
#if defined MyCobot || defined MyCobot_Pro_350 || defined MechArm
                        if (deviceConnected && !loop_on) {
                            BTConnectedInfo();
                            loop_on = true;
                        } else if (!deviceConnected && loop_on){
                            BTWaitInfo();
                            loop_on = false;
                        }
                        if (!deviceConnected && oldDeviceConnected) {
                            delay(500); // give the bluetooth stack the chance to get things ready
                            pServer->startAdvertising(); // restart advertising
                            // Serial.println("start advertising");
                            oldDeviceConnected = deviceConnected;
                        }
                        // connecting
                        if (deviceConnected && !oldDeviceConnected) {
                            oldDeviceConnected = deviceConnected;
                        }
#else
                        if (SerialBT.hasClient() && !loop_on) {
                            BTConnectedInfo();
                            //If already connected, neither discoverable nor connectable
                            esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE,
                                                     ESP_BT_NON_DISCOVERABLE);
                            loop_on = true;
                        } else if (!SerialBT.hasClient() && loop_on) {
                            BTWaitInfo();
                            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
                            loop_on = false;
                        }
                        if (confirmRequestPending) {
                            if (M5.BtnA.wasReleased()) {
                                SerialBT.confirmReply(true);
                            }
                        }

#endif
                        if (M5.BtnC.wasReleased()) {
                            myCobot.jogStop();
                            data_power = false;
                            info();
                            break;
                        }
                        delay(1);
                    }
                }
#if defined MyCobot || defined MyCobot_Pro_350 || defined MechArm
                pServer->getAdvertising()->stop();
                pService->stop();
                //pServer = NULL;
#else
                SerialBT.end();
#endif
                break;
                case Exit:
                    BLEDevice::deinit(false);
                    EXIT = true;
                    break;
                default:
                    break;
            }
        }
    }
}
/*
 * Function:Receive client request message
 */
void Transponder::GetUserData(vector<unsigned char> &data)
{
    if (transponder_mode == Uart) {
        while (Serial.available() > 0) {
            data.push_back((char)Serial.read());
        }
    } else if (transponder_mode == Wlan) {
        while (serverClients[0].available() > 0) {
            data.push_back((char)serverClients[0].read());
        }
    } else if (transponder_mode == Bt) {
#if defined MyCobot || defined MyCobot_Pro_350 || defined MechArm
        if (deviceConnected && ble_data.length() > 0) {
            for (int i = 0; i < ble_data.length(); ++i) {
                data.push_back((char)ble_data[i]);
                // Serial.printf("--%d--", data[i]);
            }
            // Serial.println();
            ble_data = "";
        }
#else
        while (SerialBT.available() > 0) {
            data.push_back((char)SerialBT.read());
        }
#endif
    }
    return;
}

/*
 * Function:Overloading because the format of setting the account is not fe fe
 */
void Transponder::GetUserData(string &data)
{
    if (transponder_mode == Uart) {
        while (Serial.available() > 0) {
            data += (char)Serial.read();
        }
    } else if (transponder_mode == Wlan) {
        while (serverClients[0].available() > 0) {
            data += (char)serverClients[0].read();
        }
    } else if (transponder_mode == Bt) {
#if defined MyCobot || defined MyCobot_Pro_350  || defined MechArm
        if (deviceConnected && ble_data.length() > 0)
            data = ble_data;
#else
        while (SerialBT.available() > 0) {
            data += (char)SerialBT.read();
        }
#endif
    }
    return;
}

/*
 * Function:Get Atom return message
 */
void Transponder::GetAtomData(vector<unsigned char> &data)
{
    //Controlling container size
    if (data.size() > 200) {
        data.clear();
    }

    while (Serial2.available() > 0) {
        data.push_back(Serial2.read());
    }
    return;
}

/*
 * Function:Handle some client messages that Atom doesn't handle
 */
bool Transponder::HandleOtherMsg(vector<unsigned char> &v_data)
{
    bool is_atom = true;
    switch (v_data[2]) {
        //b0-set ssid pwd  b1 get s+p b2-port
        case 2: {
            switch (v_data[3]) {
                case GET_BASIC_VERSION: {
                    v_data[2] = 0x03;
                    v_data.insert(v_data.end() - 1, SYSTEM_VERSION);
                }
                break;
                case GET_TOF_DISTANCE: {
                    GetTOFDistance();
                    v_data[2] = 0x04;
                    v_data.insert(v_data.end() - 1, tof.gbuf[10]);
                    v_data.insert(v_data.end() - 1, tof.gbuf[11]);
                }
                break;
                case SET_SSID_PWD: {
                    unsigned long t_begin = millis();
                    string info;
                    while (true) {
                        if ((millis() - t_begin < 200)) {
                            GetUserData(info);
                            int i = info.find(")");
                            if (i != string::npos) {
                                is_timeout = false;
                                is_first = false;
                                string bak_info = info;
                                int f_index = info.find("'");
                                int l_index = info.find_last_of("'");
                                i_ssid = "ssid: ";
                                i_password = "password: ";
                                ssid = info.substr(f_index + 1, l_index - f_index - 1);
                                i_ssid += ssid;
                                f_index = bak_info.find("(");
                                l_index = bak_info.find_last_of(")");
                                password = bak_info.substr(f_index + 1, l_index - f_index - 1);
                                i_password += password;
                                break;
                            }
                        } else if (info == "" && (millis() - t_begin > 200)) {
                            is_timeout = true;
                            break;
                        }
                    }
                }
                break;
                case GET_SSID_PWD: {
                    string str_data = "";
                    if (is_first)
                        str_data = "ssid: " + ssid + " " + "password: " + password;
                    else
                        str_data = i_ssid + " " + i_password;
                    v_data.clear();//clear fe fe 02 b1 fa
                    SendDataToUser(str_data);
                }
                break;
                default:
                    is_atom = false;
                    break;
            }
        }
        break;
        case 4:
            switch (v_data[3]) {
                case SET_PORT: {
                    uint8_t high_port = v_data[4];//fe fe 04 b2 port fa
                    uint8_t low_port = v_data[5];
                    server_port = (high_port << 8) | low_port;
                }
                break;
#if (!defined MyCobot_Pro_350)
                case SET_BASIC_OUT: {
                    //0xfe 0xfe 0x04 0xa0 pin_no pin_data 0xfa
                    byte pin_no = v_data[4];
                    pinMode(pin_no, OUTPUT);
                    delay(5);
                    bool pin_data = v_data[5];
                    digitalWrite(pin_no, pin_data);
                }
                break;
#endif
                default:
                    is_atom = false;
                    break;
            }
            break;
        case 3:
            switch (v_data[3]) {
                case SET_COMMUNICATE_MODE: {
                    is_transparent_mode = v_data[4];
                }
                break;
#if (!defined MyCobot_Pro_350)
                case GET_BASIC_IN: {
                    byte pin_no = v_data[4];
                    pinMode(pin_no, INPUT);
                    delay(5);
                    bool pin_state = digitalRead(pin_no);
                    delay(5);
                    v_data[2] = 0x04;
                    v_data.insert(v_data.end() - 1, pin_state);
                }
                break;
#endif
                default:
                    is_atom = false;
                    break;
            }
            break;
        default:
            is_atom = false;
            break;
    }
    return is_atom;
}

/*
 * Function：send and receive messages
 */
void Transponder::readData()
{
    vector<unsigned char> client_data;
    vector<unsigned char> atom_data;
    while (!EXIT) {
        xSemaphoreTake(xSemap, portMAX_DELAY);

        //Receive user messages and send them to Atom
        if (data_power) {
            GetUserData(client_data);
            SendDataToAtom(client_data);

            //Receive Atom messages and send them to user
            GetAtomData(atom_data);
            SendDataToUser(atom_data);
        }

        xSemaphoreGive(xSemap);
        vTaskDelay(1);
    }
    vTaskDelete(NULL);
    return;
}

void Transponder::rFlushSerial()
{
    while (Serial.read() != -1)
        ;
}

/*
 * Function: Send the message returned by Atom to the user
 */
void Transponder::SendDataToUser(vector<unsigned char> &v_data)
{
    vector<unsigned char> temp;
    vector<unsigned char>::iterator it;
    /* Judging whether the data is empty, not empty-->Start processing data-->Delete ffff..
     * then,the beginning of fefe exists in the data
     * yes-->Judging whether there is fa(At the end of the fa) in the data-->Send data
     */
    if (!v_data.empty()) {
        if (is_transparent_mode) {
            WriteData(USER, v_data);
            v_data.clear();
        } else {
            if (HandleAtomData(v_data)) {
                temp = v_data;
                if (HandleStickyPackets(temp, v_data)) {
                    vector<unsigned char>::iterator it_send = temp.begin();
                    WriteData(USER, temp);
                        /*if (transponder_mode == Uart) {
                            Serial.write(temp.data(), (uint8_t)temp.size());
                        } else if (transponder_mode == Wlan) {
                            serverClients[0].write(temp.data(), (uint8_t)temp.size());
                        } else if (transponder_mode == Bt) {
    #if defined MyCobot || defined MyCobot_Pro_350  || defined MechArm
                            if (deviceConnected) {
                                pTxCharacteristic->setValue(temp.data(), (uint8_t)temp.size());
                                pTxCharacteristic->notify();
                            }
    #else
                            SerialBT.write(temp.data(), (uint8_t)temp.size());
    #endif
                        }*/
                }
            }
        }
    }
    return;
}

/*
 * Function:Overload the wifi account format, which is inconsistent with the communication protocol
 */
void Transponder::SendDataToUser(const string str_data)
{
    if (transponder_mode == Uart) {
        Serial.write(str_data.c_str(), str_data.size());
    } else if (transponder_mode == Wlan) {
        serverClients[0].write(str_data.c_str(), str_data.size());
    } else if (transponder_mode == Bt) {
#if defined MyCobot || defined MyCobot_Pro_350  || defined MechArm
        if (deviceConnected) {
    //      SerialBT.write((uint8_t *)str_data.c_str(), str_data.size());
    //      pCharacteristic->setValue((uint8_t *)str_data.c_str(), str_data.size());
    //      pCharacteristic->notify();
            pTxCharacteristic->setValue((uint8_t *)str_data.c_str(), str_data.size());
            pTxCharacteristic->notify();
        }
#else
        SerialBT.write((uint8_t *)str_data.c_str(), str_data.size());
#endif
    }
}

/*
 *send data
 *mode 1--send data to atom serial2 2--send data to user serial
 *v_data the data
*/
void Transponder::WriteData(int mode, vector<unsigned char> v_data)
{
    switch (mode)
    {
    case 1: {
        Serial2.write(v_data.data(), (uint8_t)v_data.size());
        break;
    }
    case 2: {
        if (transponder_mode == Uart) {
            Serial.write(v_data.data(), (uint8_t)v_data.size());
        } else if (transponder_mode == Wlan) {
            serverClients[0].write(v_data.data(), (uint8_t)v_data.size());
        } else if (transponder_mode == Bt) {
    #if defined MyCobot || defined MyCobot_Pro_350  || defined MechArm
                if (deviceConnected) {
                    pTxCharacteristic->setValue(v_data.data(), (uint8_t)v_data.size());
                    pTxCharacteristic->notify();
                }
    #else
                SerialBT.write(v_data.data(), (uint8_t)v_data.size());
    #endif
                    }
        break;
    }
    default:
        break;
    }
}

/*
 * Function:Send messages from users to Atom
 */
void Transponder::SendDataToAtom(vector<unsigned char> &v_data)
{
    vector<unsigned char> temp;
    temp = v_data;
    vector<unsigned char>::iterator it;
    int state = 1;
    /* Judging whether the data is empty, not empty--" Judging whether the header is fefe, whether there is an end bit fa in the data,
      *yes-->Judging whether the message is processed by Atom, no--" Direct processing, Yes--" Send to Atom*/
    if (!v_data.empty()) {
        if (is_transparent_mode) {
            if (HandleOtherMsg(v_data))
                state = 2;
            WriteData(state, v_data); //透传下，指令不处理直接下发
            v_data.clear();
        } else {
            if (checkHeader(v_data)) {
                if (HandleStickyPackets(temp, v_data)) {
                    if (HandleOtherMsg(temp)) {
                        SendDataToUser(temp);
                    } else {
                        vector<unsigned char>::iterator it_send = temp.begin();
                        for (it_send; it_send < temp.end(); it_send++) {
                            // WriteData(ATOM, /**it_send*/temp);
                            Serial2.write(*it_send);
                        }
                    }
                }
            }
        }
    }
    return;
}

/*
 * Function:interface display template
 */
void Transponder::UITemplate(vector<unsigned short> color,
                             vector<unsigned char> size, vector<short> x, vector<short> y,
                             vector<string> msg, vector<int> line_feed)
{
    for (int i = 0; i < color.size(); i++) {
        M5.Lcd.setTextColor(color[i]);
        M5.Lcd.setTextSize(size[i]);
        M5.Lcd.setCursor(x[i], y[i]);
        if (line_feed[i] == 1)
            M5.Lcd.println(msg[i].c_str());
        else if (line_feed[i] == 0)
            M5.Lcd.print(msg[i].c_str());
        else if (line_feed[i] == 2)
            M5.Lcd.printf(msg[i].c_str());
    }
}

/*
 * Function: Get display Atom connection status
 */
void Transponder::connect_ATOM(MyCobotBasic &myCobot)
{
    M5.Lcd.clear(BLACK);
    delay(50);
    UITemplate({RED, WHITE, WHITE}, {2, 2, 2}, {55, 10, 230}, {20, 120, 220}, {"Connect test", "Atom: ", "Exit"}, {1, 0, 1});
    M5.Lcd.drawFastHLine(0, 70, 320, GREY);
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.setCursor(80, 120);
    xSemaphoreTake(xSemap, portMAX_DELAY);
    int state = myCobot.isPoweredOn();//when tranponder,should mutex,otherwise,readdata will read
    xSemaphoreGive(xSemap);
    if (state == 1) {
        M5.Lcd.println("ok");
    } else {
        M5.Lcd.println("no");
    }
    M5.update(); 
}

void Transponder::info()
{
    string menu[4] = {"USB UART", "WLAN Server", "Bluetooth", "EXIT"};
    int num[4] = {0, 1, 2, 3};

    M5.Lcd.clear(BLACK);
    UITemplate({RED, RED}, {3, 2}, {10, 10}, {10, 40}, {TYPE, "Basic Transponder"}, {2, 1});

    distep.MenuInit(num, menu, 4);
    distep.MenuChoice();

    distep.Start();
    M5.update();
    delay(10);
}

/*
 * Function: Create WLAN service
 */
void Transponder::CreateWlanServer()
{
    WiFi.begin(ssid.c_str(), password.c_str());
    for (int loops = 10; loops > 0; loops--) {
        if (WiFi.status() == WL_CONNECTED) {
            ip = WiFi.localIP();
            bak_ssid = ssid;
            bak_password = password;
//            transponder_mode = Wlan;
            wlan_uart = false;
            ConnectedInfo();
            break;
        } else {
            //Serial.println(loops);
            delay(1000);
        }
    }
    if (WiFi.status() != WL_CONNECTED) {
        if (!wlan_uart) {
            ssid = bak_ssid;
            password = bak_password;
            ConnectFailedInfo(true);
            delay(500);
            //Reconnect to the last successful network
            CreateWlanServer();
        } else {
            transponder_mode = Uart;
            wlan_uart = true;
            ConnectFailedInfo(false);
            //delay(1000);
        }
    }

    server.begin(server_port);
    server.setNoDelay(true);
    return;
}

/*
 * Function: wifi connection status, number of clients, currently the service only supports one client connection
 */
void Transponder::WlanTransponder()
{
    uint8_t i;
    if (WiFi.status() == WL_CONNECTED) {
        //check if there are any new clients
        if (server.hasClient()) {
            for (i = 0; i < MAX_SRV_CLIENTS; i++) {
                //find free/disconnected spot
                if (!serverClients[i] || !serverClients[i].connected()) {
                    if (serverClients[i]) serverClients[i].stop();
                    serverClients[i] = server.available();
                    //if (!serverClients[i]) //Serial.println("available broken");
                    //Serial.print("New client: ");
                    //Serial.print(i);
                    //Serial.print(' ');
                    //Serial.println(serverClients[i].remoteIP());
                    break;
                }
            }
            if (i >= MAX_SRV_CLIENTS) {
                //no free/disconnected spot so reject
                server.available().stop();
            }
        }
    } else {
        //Serial.println("WiFi not connected!");
        for (i = 0; i < MAX_SRV_CLIENTS; i++) {
            if (serverClients[i]) serverClients[i].stop();
        }
        delay(1000);
    }
}

/*
 * Function: interface display during wifi connection
 */
void Transponder::ConnectingInfo()
{
    M5.Lcd.clear(BLACK);
    M5.Lcd.fillRect(0, 0, 320, 30, GREY);
    M5.Lcd.setBitmapColor(GREY, GREY);
    UITemplate({WHITE, WHITE}, {2, 2}, {10, 10}, {10, 80}, {ssid, "WIFI Connecting"}, {1, 1});
}

/*
 * Function:After the wifi connection is successful, the interface displays: wifi account wlan ip, port
 */
void Transponder::ConnectedInfo()
{
    M5.Lcd.clear(BLACK);
    M5.Lcd.fillRect(0, 0, 320, 30, GREY);
    M5.Lcd.setBitmapColor(GREY, GREY);
    UITemplate({GREEN, WHITE, WHITE, WHITE, WHITE, WHITE}, {2, 2, 2, 2, 2, 2}, {10, 10, 10, 10, 20, 230},
    {10, 80, 110, 140, 220, 220}, {ssid, "WIFI Connected", "IP: ", "Port: ", "ReConnect", "Exit"},
    {1, 1, 0, 0, 1, 1});
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setCursor(50, 110);
    M5.Lcd.println(ip);
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setCursor(80, 140);
    M5.Lcd.println(server_port);
}

/*
 * Function: wifi connection failure interface display
 */
void Transponder::ConnectFailedInfo(bool flag)
{
    M5.Lcd.clear(BLACK);
    M5.Lcd.fillRect(0, 0, 320, 30, GREY);
    M5.Lcd.setBitmapColor(GREY, GREY);
    if (flag) {
        UITemplate({RED, WHITE, WHITE, WHITE, WHITE, WHITE}, {2, 2, 2, 2, 2, 2}, {10, 10, 10, 10, 20, 230},
        {10, 80, 110, 140, 220, 220}, {ssid, "WIFI Connect Failed", "ssid or password error", "Connecting last wifi", "ReConnect", "Exit"},
        {1, 0, 0, 0, 1, 1});
    } else {
        UITemplate({RED, WHITE, WHITE, WHITE, WHITE}, {2, 2, 2, 2, 2}, {10, 10, 110, 20, 230},
        {10, 80, 110, 220, 220}, {ssid, "WIFI Connect Failed", "ssid or password error", "ReConnect", "Exit"},
        {1, 0, 0, 1, 1});
    }
}

void Transponder::TimeOutInfo()
{
    M5.Lcd.clear(BLACK);
    M5.Lcd.fillRect(0, 0, 320, 30, GREY);
    M5.Lcd.setBitmapColor(GREY, GREY);
    UITemplate({GREEN, WHITE}, {2, 2}, {10, 10}, {10, 80}, {ssid, "Input timeout(200ms)"}, {1, 0});
}

void Transponder::SetBaud()
{

}

#if (!defined MyCobot) && (!defined MyCobot_Pro_350) && (!defined MechArm)
void BTConfirmRequestCallback(uint32_t numVal)
{
    confirmRequestPending = true;
    Transponder::BTConnectingInfo(numVal);
}

void BTAuthCompleteCallback(boolean success)
{
    confirmRequestPending = false;
    if (success) {
        M5.Lcd.fillRect(10, 110, 260, 20, BLACK);
        M5.Lcd.fillRect(10, 220, 60, 30, BLACK);
        //Serial.println("Pairing success!!");
    } else {
        //Serial.println("Pairing failed, rejected by user!!");
    }
}
#else
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
    //   Serial.println("onWrite");
      std::string rxValue = pCharacteristic->getValue();
      ble_data = rxValue;
    }
};
#endif
/*
 * Function: create a ble service
 */
void Transponder::CreateBTServer()
{
#if defined MyCobot || defined MyCobot_Pro_350 || defined MechArm
    // Create the BLE Device
    BLEDevice::init(Bt_name);
    // Create the BLE Server
    if (pServer == NULL)
    {
        pServer = BLEDevice::createServer();
    
        pServer->setCallbacks(new MyServerCallbacks());
        // Create the BLE Service
        pService = pServer->createService(SERVICE_UUID);
      
        // Create a BLE Characteristic
        pTxCharacteristic = pService->createCharacteristic(
                          CHARACTERISTIC_UUID_TX,
                          BLECharacteristic::PROPERTY_NOTIFY
                        );
                            
        pTxCharacteristic->addDescriptor(new BLE2902());
      
        BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                             CHARACTERISTIC_UUID_RX,
                            BLECharacteristic::PROPERTY_WRITE
                          );
      
        pRxCharacteristic->setCallbacks(new MyCallbacks());
    }
      
    // Start the service
    pService->start();
    
    // Start advertising
    pServer->getAdvertising()->start();
    Serial.println("Waiting a client connection to notify...");
#else
    SerialBT.enableSSP();
    SerialBT.onConfirmRequest(&BTConfirmRequestCallback);
    SerialBT.onAuthComplete(&BTAuthCompleteCallback);
    //Bluetooth device name
    SerialBT.begin(Bt_name);
    //Serial.println("The device started, now you can pair it with bluetooth!");
#endif
    //get mac address
    esp_efuse_mac_get_default(mac_addr);
    mac_addr[5] += 2; //last addr need +2
    return;
}

/*
 * Function:After the Bluetooth is turned on, the interface is displayed
 */
void Transponder::BTWaitInfo()
{
    M5.Lcd.clear(BLACK);
    M5.Lcd.fillRect(0, 0, 320, 30, GREY);
    M5.Lcd.setBitmapColor(GREY, GREY);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println(Bt_name.c_str());
#if defined MyCobot || defined MyCobot_Pro_350 || defined MechArm
    UITemplate({WHITE, WHITE, WHITE, WHITE}, {2, 2, 2, 2}, {10, 10, 10, 230}, {80, 130, 170, 220},
    {"Bluetooth enabled", "BT Mac: ", "UUID: ", "Exit"}, {0, 0, 0, 1});
#else
    UITemplate({WHITE, WHITE, WHITE}, {2, 2, 2}, {10, 10, 230}, {80, 130, 220},
    {"Bluetooth enabled", "BT Mac: ", "Exit"}, {0, 0, 1});
#endif
    M5.Lcd.setCursor(100, 130);
    M5.Lcd.setTextSize(2);
    for (int i = 0; i < 6; i++) {
        M5.Lcd.printf("%02x", mac_addr[i]);
        if (i != 5)
            M5.Lcd.print(":");
    }
#if defined MyCobot || defined MyCobot_Pro_350 || defined MechArm
    M5.Lcd.setCursor(75, 170);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println(SERVICE_UUID);
#endif
}

/*
 * Function: Interface display during client Bluetooth connection: pairing code display
 */
void Transponder::BTConnectingInfo(uint32_t numVal)
{
#if (!defined MyCobot) && (!defined MyCobot_Pro_350) && (!defined MechArm)
     M5.Lcd.fillRect(10, 110, 260, 20, BLACK);
     M5.Lcd.fillRect(10, 220, 60, 30, BLACK);
     UITemplate({WHITE, WHITE}, {2, 2}, {20, 10}, {220, 110}, {"Pair", "Pairing code: "}, {1, 0});
     M5.Lcd.setCursor(200, 110);
     M5.Lcd.setTextSize(2);
     M5.Lcd.print(numVal);
#endif
}

/*
 * Function: The client bluetooth connection is successful, and the interface is displayed
 */
void Transponder::BTConnectedInfo()
{
    M5.Lcd.clear(BLACK);
    M5.Lcd.fillRect(0, 0, 320, 30, GREY);
    M5.Lcd.setBitmapColor(GREY, GREY);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println(Bt_name.c_str());
#if defined MyCobot || defined MyCobot_Pro_350 || defined MechArm
    UITemplate({WHITE, WHITE, WHITE, WHITE}, {2, 2, 2, 2}, {10, 10, 10, 230}, {80, 130, 170, 220}, {"A client connected", "BT Mac: ", "UUID: ", "Exit"}, {0, 0, 0, 1});
#else
    UITemplate({WHITE, WHITE, WHITE}, {2, 2, 2}, {10, 10, 230}, {80, 130, 220}, {"A client connected", "BT Mac: ", "Exit"}, {0, 0, 1});
#endif
    M5.Lcd.setCursor(100, 130);
    M5.Lcd.setTextSize(2);
    for (int i = 0; i < 6; i++) {
        M5.Lcd.printf("%02x", mac_addr[i]);
        if (i != 5)
            M5.Lcd.print(":");
    }
#if defined MyCobot || defined MyCobot_Pro_350 || defined MechArm
    M5.Lcd.setCursor(75, 170);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println(SERVICE_UUID);
#endif
}

/*
 * Function: Get tof distance value
 */
void Transponder::GetTOFDistance()
{
    Wire.begin();
    //uint16_t dist = 0;
    tof.write_byte_data_at(VL53L0X_REG_SYSRANGE_START, 0x01);
    byte val = 0;
    int cnt = 0;
    // 1 second waiting time max
    while (cnt < 100) {
        delay(10);
        val = tof.read_byte_data_at(VL53L0X_REG_RESULT_RANGE_STATUS);
        if (val & 0x01) break;
        if (cnt++ == 99) {
            return;
        }
    }
    tof.read_block_data_at(0x14, 12);
}
