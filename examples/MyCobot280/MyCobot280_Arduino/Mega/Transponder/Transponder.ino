#include <MyCobotBasic.h>
#include <vector>
#include <EEPROM.h>
// #define POWER_DEBUG
#define VERSION 0x0B
#define SET_BASIC_OUT 0xa0
#define GET_BASIC_IN 0xa1
#define GET_VERSION 0xF6
#define GET_REBOOT_COUNT 0xD8

byte HEADER = 0xFE;
byte FOOTER = 0xFA;
#define USER 2
#define REBOOT_COUNT_ADDR 20
#define EEPROM_UINT32_DEFAULT_VALUE 4294967295

using namespace std;
vector<unsigned char> client_data;
vector<unsigned char> atom_data;
vector<unsigned char> temp_data;
vector<unsigned char> buffer_data;
int count = -1, data = -1;
long long t_begin;

void GetUserData(vector<unsigned char> &data);

void setup()
{
  Serial.begin(1000000);
  /* If you use other boards, please check whether the serial port on the official website supports Serial1.
     For example, m5's Serial is Serial1 (so replace the following Serial1 with Serial1)*/
  Serial1.begin(1000000);
  delay(100);

  MyCobotBasic myCobot;
  delay(500);

  myCobot.powerOn();
  delay(500);

//  pinMode(15, OUTPUT); // 1
//  pinMode(5, OUTPUT); // 2
//  pinMode(2, OUTPUT); // 2

#ifdef POWER_DEBUG 
  Serial.println("------------------------------------ele system reboot--------------------------------------");
#endif
    RecordRebootCount();
}

void loop()
{
//   while(Serial1.available()>0){Serial.write(Serial1.read());}//atom返回数据,然后打印至电脑
//   while(Serial.available()>0){Serial1.write(Serial.read());}//电脑发送数据,然后basic发给atom

//  ClientDataToAtom();
  /*SendDataToAtom();
  GetAtomData();
  AtomDataToClient();
  OtherMsgToClient();
  ClearData();*/
        //Receive user messages and send them to Atom
  GetUserData(client_data);
  SendDataToAtom(client_data);

  //Receive Atom messages and send them to user
  GetAtomData(atom_data);
  SendDataToUser(atom_data);
  delay(1);
}


/*
   Function: Check whether the header of the serial port wifi or bluetooth return message is fe fe
*/
bool checkHeader(vector<unsigned char> &v_data)
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
bool HandleStickyPackets(vector<unsigned char> &temp,
                                      vector<unsigned char> &v_data)
{
    //Get the length of the protocol to determine the terminator fa
    vector<unsigned char>::iterator it = temp.begin() + 2 + temp[2];
    vector<unsigned char>::iterator it_vdata = v_data.begin() + 2 + v_data[2];
    int len = temp.size();
    //now protocol,latest len is 26,like fe fe 88(88 is error)-->delete three, meanwhile need len-->latest is fa
    if (temp[2] < 30 && *it == FOOTER) {
        temp.erase(it + 1, temp.end());
        v_data.erase(v_data.begin(), it_vdata + 1);
        return true;
    } else {
         v_data.erase(v_data.begin() + 1, v_data.begin() + 2);
    }
    return false;
}

/*
   Function:Handle some client messages that Atom doesn't handle
*/
bool HandleOtherMsg(vector<unsigned char> &v_data)
{
  bool flag = false;
  switch (v_data[2]) {
    case 4:
      switch (v_data[3]) {
        case SET_BASIC_OUT: {
            //0xfe 0xfe 0x04 0xa0 pin_no pin_data 0xfa
            byte pin_no = v_data[4];
            pinMode(pin_no, OUTPUT);
            delay(5);
            bool pin_data = v_data[5];
            digitalWrite(pin_no, pin_data);
          }
          break;
      }
      break;
    case 3:
      switch (v_data[3]) {
        case GET_BASIC_IN: {
            //0xfe 0xfe 0x03 0xa1 pin_no 0xfa
            byte pin_no = v_data[4];
            pinMode(pin_no, INPUT);
            delay(5);
            bool pin_state = digitalRead(pin_no);
            delay(5);
            v_data[2] = 0x04;
            v_data.insert(v_data.end() - 1, pin_state);
            flag = true;
          }
          break;
      }
    case 2:
      switch (v_data[3]) {
        case GET_VERSION: {
            //0xfe 0xfe 0x02 0x03 0xfa
            byte version = VERSION;
            v_data[2] = 0x03;
            v_data.insert(v_data.end() - 1, version);
            flag = true;
            break;
          }
        case GET_REBOOT_COUNT: {
          int reboot_count = 
            GetIntEEPROM(REBOOT_COUNT_ADDR);
          uint8_t byte_data[4] = {0,0,0,0};
          byte_data[0] = reboot_count >> 24;
          byte_data[1] = reboot_count >> 16;
          byte_data[2] = reboot_count >> 8;
          byte_data[3] = reboot_count & 0xff;
          v_data[2] = 0x06;
          for (int j = 0; j < sizeof(byte_data) / sizeof(byte_data[0]); ++j)
          {
              v_data.insert(v_data.end() - 1, byte_data[j]);
          }
          break;
        }
      }
      break;
  }
  return flag;
}

/*
   Function: Send the message returned by Atom to the user
*/
void SendDataToUser(vector<unsigned char> &v_data)
{
  vector<unsigned char> temp;
    vector<unsigned char>::iterator it;
    /* Judging whether the data is empty, not empty-->Start processing data-->Delete ffff..
     * then,the beginning of fefe exists in the data
     * yes-->Judging whether there is fa(At the end of the fa) in the data-->Send data
     */
    if (!v_data.empty()) {
        if (HandleAtomData(v_data)) {
            temp = v_data;
            if (HandleStickyPackets(temp, v_data)) {
                vector<unsigned char>::iterator it_send = temp.begin();
                WriteData(USER, temp);
            }
        }
    }
  return;
}

/*void ClientDataToAtom()
{
  if (Serial.available() > 0) {
    count = 1;
    while (Serial.available() > 0) {
      data = Serial.read();
      Serial1.write(data);
      client_data.push_back(data);
    }
    t_begin = millis();
  }
}*/

/*
 * Function:Send messages from users to Atom
 */
void SendDataToAtom(vector<unsigned char> &v_data)
{
    vector<unsigned char> temp;
    temp = v_data;
    vector<unsigned char>::iterator it;
    int state = 1;
    /* Judging whether the data is empty, not empty--" Judging whether the header is fefe, whether there is an end bit fa in the data,
      *yes-->Judging whether the message is processed by Atom, no--" Direct processing, Yes--" Send to Atom*/
    if (!v_data.empty()) {
        if (checkHeader(v_data)) {
            if (HandleStickyPackets(temp, v_data)) {
                if (HandleOtherMsg(temp)) {
                    SendDataToUser(temp);
                } else {
                    vector<unsigned char>::iterator it_send = temp.begin();
                    for (it_send; it_send < temp.end(); it_send++) {
                        // WriteData(ATOM, temp);
                        Serial1.write(*it_send);
                    }
                }
            }
        }
    }
    return;
}

void GetAtomData(vector<unsigned char> &data)//保证atom返回的数据都可以获取到
{
   if (data.size() > 200) {
        data.clear();
    }
    while (Serial1.available() > 0) {
        data.push_back(Serial1.read());
        delayMicroseconds(50);
    }

}

void AtomDataToClient()
{
    static int flag=0;
    static int First_HEADER=0;
    if (!buffer_data.empty()) {//如果缓存有数据
        static int num = 0;
        count = 0;
        for( int i = 0; i < buffer_data.size(); i++) {
//          Serial.print(buffer_data[i],HEX);
//          Serial.print(" ");
           if ( flag == 0 &&buffer_data[i] == HEADER ){
            if( i + 1 == buffer_data.size())//此为分开返回的情况
            {First_HEADER = HEADER; flag = 2;break;}
            else if (buffer_data[i+1] == HEADER)//此为一起返回的情况
            {flag = 1;data = buffer_data[1];}
           }
              
           if (flag == 0){First_HEADER = 0;break;}
           
           if (flag == 2){
            if (buffer_data[0] == HEADER){flag = 1;data = buffer_data[1];Serial.write(First_HEADER);First_HEADER=0;}
           }

           if (flag == 1){
            Serial.write(buffer_data[i]);
            num++;
            }
           if(buffer_data[i] == FOOTER && num == data){
                flag = 0;
                num = 0;
           }
       }
//           if(abs(millis()-t_begin)>20){
//            Serial.print(t_begin);
//            Serial.print(" ");
//          }
 }
}


void OtherMsgToClient()
{
  if (count == 1 && ((millis() - t_begin) > 10)) {
    count = 0;
    for (int i = 0; i < client_data.size(); i++) {
      temp_data.push_back(client_data[i]);
    }
    SendDataToUser(temp_data);
  }
}

void ClearData()
{
    buffer_data.clear();
  if (count == 0) {
    client_data.clear();
  }
}

/*
 * Function:Receive client request message
 */
void GetUserData(vector<unsigned char> &data)
{
      while (Serial.available() > 0) {
          data.push_back((char)Serial.read());
          delayMicroseconds(50);
      }
    return;
}

/*
 * Function:Process the data returned by Atom, no need for FF FF
 */
bool HandleAtomData(vector<unsigned char> &v_data)
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
 *send data
 *mode 1--send data to atom serial2 2--send data to user serial
 *v_data the data
*/
void WriteData(int mode, vector<unsigned char> v_data)
{
    switch (mode)
    {
    case 1: {
        Serial1.write(v_data.data(), (uint8_t)v_data.size());
        break;
    }
    case 2: {
        
        Serial.write(v_data.data(), (uint8_t)v_data.size());
        break;
    }
    default:
        break;
    }
}

uint32_t GetIntEEPROM(byte addr)
{
    uint32_t boot_count = 0;
    EEPROM.get(addr, boot_count); 
// #ifdef POWER_DEBUG 
    // Serial.println("Boot count: ");
    // Serial.println(boot_count);
// #endif
    return boot_count;
}

void SetIntEEPROM(byte addr, uint32_t boot_count)
{
// #ifdef POWER_DEBUG 
    /*Serial.println("SetIntEEPROM: ");
    Serial.println(addr);
    Serial.println(" , ");
    Serial.println(boot_count);*/
// #endif
    EEPROM.put(addr, boot_count);
    return;
}

void RecordRebootCount()
{
    uint32_t reboot_count = 0;
    if ((reboot_count = GetIntEEPROM(REBOOT_COUNT_ADDR)) == 
        EEPROM_UINT32_DEFAULT_VALUE) {
        reboot_count = 1;
    } else {
        ++reboot_count;
    }
    // Serial.println(reboot_count);
    SetIntEEPROM(REBOOT_COUNT_ADDR, reboot_count);
}



//待用勿删
//  static int temp = 0;
//  static int flag = 0;
//  delay(0.5);
//  while (Serial1.available() > 0) {
//    data = Serial1.read();
//
//    if (data == HEADER && flag == 0 && (temp = Serial1.read()) == HEADER) {
//      flag = 2;
//      Serial.write(temp);
//      Serial.write(data);
//    }
//
//    else if (flag == 0 && data == HEADER || flag == 1 )
//      if ((temp = Serial1.read()) != HEADER)
//        flag = 1;
//      else if (data == HEADER) {
//        flag = 2;
//        Serial.write(data);
//      }
//
//    if (flag == 2) {
//      Serial.write(data);
//      if (data == FOOTER)
//        flag = 0;
//    }
//
//
//  }


// void AtomDataToClient()
// {   
//     static int flag=0;
//     static int First_HEADER=0;
//     if (!buffer_data.empty()) {//如果缓存有数据
//         count = 0;
//         delay(2);
//         //Serial.print(buffer_data.size());
//         for( int i = 0; i < buffer_data.size(); i++) {
//             //Serial.print(buffer_data.size());

//             //考虑到如果FE FE是分开返回的情况
//             if (buffer_data.size() == 1 && buffer_data[i] == HEADER){
//                First_HEADER = buffer_data[i];//获取本条唯一的数据
//               break;//跳出该条返回的数据,进入下一条
//             }
//             if (First_HEADER == HEADER && buffer_data[i]== HEADER ){
//               flag = 1;
//               Serial.write(First_HEADER);
//               First_HEADER = 0;
//             }
            
//             //此情况为FE FE一起返回的情况
//             if ((buffer_data[i+1]) == HEADER && (buffer_data[i]) == HEADER ) {//Serial.print(1000,DEC);//检测解析方法是否正确,正确
//                 flag =1;
//                 Serial.write(buffer_data[i]); //FE
//                 Serial.write(buffer_data[i+1]); //FE
//                 i=i+1;//指向数据,若只返回了FE FE,continue后也会直接跳出循环
//                 continue;
//             }
//             if (flag ==1) {
//                 Serial.write(buffer_data[i]);
//             if (buffer_data[i] == FOOTER) flag=0;
//             }
//          }
//     }
// }