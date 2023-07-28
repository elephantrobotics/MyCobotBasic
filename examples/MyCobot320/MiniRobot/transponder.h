#ifndef transponder_h
#define transponder_h

#include <MyCobotBasic.h>
#include "config.h"
#include "ServerBase.h"
#include <WiFi.h>
#include <vector>
#include <string>
#if defined MyCobot || defined MyCobot_Pro_350 || defined MechArm
  #include <BLEDevice.h>
  #include <BLEServer.h>
  #include <BLEUtils.h>
  #include <BLE2902.h>
  #include <BluetoothSerial.h>
#else
  #include <BluetoothSerial.h>
#endif
#include "TOF.h"

using namespace std;

#if defined MyCobot || defined MyCobot_Pro_350 || defined MechArm
  #define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"//"4fafc201-1fb5-459e-0001-c5c9c331914b"// // UART service UUID
  #define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"//"beb5483e-36e1-4688-0001-ea07361b26a8"//
  #define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"//"beb5483e-36e1-4688-0001-ea07361b26a8"//
#endif
/*#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif*/
#define ARDUINOOSC_DEBUGLOG_ENABLE
//#include <ArduinoOSC.h>

#define MAX_SRV_CLIENTS 1
#define REMOVE_BONDED_DEVICES 0
#define PAIR_MAX_DEVICES 20

class Transponder: public ServerBase
{
private:
    bool checkHeader(vector<unsigned char> &v_data);
    bool HandleStickyPackets(vector<unsigned char> &temp,
                             vector<unsigned char> &v_data);
    bool HandleAtomData(vector<unsigned char> &v_data);
    void EventResponse(MyCobotBasic &myCobot);
    static void UITemplate(vector<unsigned short> color, vector<unsigned char> size,
                           vector<short> x, vector<short> y, vector<string> msg, vector<int> line_feed);
    void rFlushSerial();
    void connect_ATOM(MyCobotBasic &myCobot);
    void info();
    void CreateWlanServer();
    void WlanTransponder();
    void ConnectingInfo();
    void ConnectedInfo();
    void SetBaud();
    string GetWlanInfo();
    void BTConnectedInfo();
    void BTWaitInfo();
    void CreateBTServer();

    void WriteData(int mode, vector<unsigned char> v_data);
    void GetUserData(string &data);
    void GetUserData(vector<unsigned char> &data);
    void SendDataToUser(vector<unsigned char> &v_data);
    void SendDataToUser(const string str_data);
    void GetAtomData(vector<unsigned char> &data);
    void SendDataToAtom(vector<unsigned char> &v_data);
    bool HandleOtherMsg(vector<unsigned char> &v_data);

    void ConnectFailedInfo(bool flag);
    void TimeOutInfo();
    void GetTOFDistance();

public:
    void run(MyCobotBasic &myCobot);
    void init();
    static ServerBase *createInstance()
    {
        return new Transponder();
    }
    static void BTConnectingInfo(uint32_t numVal);

    //task call
    void readData();

    //task
    static void TaskReadData(void *p);

    //Control the data read and write between basic and user
    bool data_power = false;

private:
    bool EXIT = false;
    bool state_on{false};
    int pos_y[4] = {70, 100, 130, 160};
    int rect1[4] = {0, 70, 26, 120};
    int rect2[4] = {26, 70, 200, 120};
    DisplayTemplate distep;
#if defined MyCobot || defined MyCobot_Pro_350
    string ssid = "MyCobotWiFi2.4G";
    string password = "mycobot123";
#elif defined MechArm 
    string ssid = "MechArmWiFi2.4G";
    string password = "mecharm123";
#endif
    string bak_ssid;
    string bak_password;
    string i_ssid = "ssid: ";
    string i_password = "pssword: ";
    //Whether ssid and pwd are not entered
    bool is_first{true};
    uint16_t server_port = 9000;
    IPAddress ip;
    bool wifi_state = false;
    WiFiClient serverClients[MAX_SRV_CLIENTS];
    bool wlan_uart{true};
    bool is_timeout{false};
#if defined MyCobot || defined MyCobot_Pro_350 || defined MechArm
    //ble
#if defined MyCobot
    string Bt_name = "mycobot280m5ble";
#elif defined MechArm
    string Bt_name = "mecharm270m5ble";
#elif defined MyCobot_Pro_350
    string Bt_name = "mycobot320m5ble";
#endif
    BLEServer *pServer = NULL;
    BLEService *pService = NULL;
    BLECharacteristic * pTxCharacteristic;
//    BLECharacteristic * pCharacteristic;
    bool oldDeviceConnected = false;
    uint8_t txValue = 0;
#else
    //bt
    BluetoothSerial SerialBT;
#if (!defined MyCobot_Pro_350)
    String Bt_name = "mycobot280-m5";
#else
    String Bt_name = "mycobot320-m5";
#endif
    uint8_t pairedDeviceBtAddr[PAIR_MAX_DEVICES][6];
    //Define Mac address storage space
//    uint8_t mac_addr[6] = {0};
    char bda_str[18];
    bool is_paired{false};
#endif
    uint8_t mac_addr[6] = {0};
    bool loop_on{false};
    
    TOF tof;
    enum MODE : int {Uart = 0, Wlan = 1, Bt = 2, Exit = 3} transponder_mode;
    bool is_transparent_mode = false; //默认为非透传（比如指令特有帧头帧尾） ture--透传(接收指令后不处理)
};

#endif
