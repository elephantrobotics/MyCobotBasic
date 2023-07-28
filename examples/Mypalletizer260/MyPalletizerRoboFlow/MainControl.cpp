#include "MainControl.h"

typedef struct {
    int joint_angle[4];
} joint_MyPalletizerAngles_enc;

int data_len_max = 1000;
MyPalletizerAngles jae[1000];

int girrep_data[1000];

byte control_pin = 26;   // PIN 26 high -> loop play data from Flash

int rec_data_len = 0;
byte MyPal_mode = 0;

int error_display_time = 3000;
static int EXIT = false;


void MainControl::run(MyPalletizerBasic &MyPal)
{
    pinMode(control_pin, INPUT);
    MyPal.setLEDRGB(0, 255, 0);
    MainControl::updateMode(MyPal, MyPal_mode);
    MainControl::displayInfo(MyPal, 0);
    while (1) {
        M5.update();
        byte btn_pressed = 0;
        if (M5.BtnA.wasReleased()) {
            btn_pressed = 1;
            MainControl::updateMode(MyPal, btn_pressed);
        }
        if (M5.BtnB.wasReleased()) {
            btn_pressed = 2;
            MainControl::updateMode(MyPal, btn_pressed);
        }
        if (M5.BtnC.wasReleased()) {
            btn_pressed = 3;
            MainControl::updateMode(MyPal, btn_pressed);
        }
        if (EXIT) {
            EXIT = false;
            break;
        }
        MainControl::IO(MyPal);
    }
}

void MainControl::updateMode(MyPalletizerBasic &MyPal, byte btn_pressed)
{
    btn = (enum BTN)btn_pressed;
    if (ui == Menu) {
        switch (btn) {
            case A:
                ui = PlayMenu;
                MainControl::displayInfo(MyPal, ui);
                break;
            case B:
                ui = RecordMenu;
                MainControl::displayInfo(MyPal, ui);
                MyPal.releaseAllServos();
                break;
            case C:
                // MyPal.setFreeMoveMode(0);
                delay(20);
                // MyPal.releaseServo(7);
                EXIT = true;
                break;
        }

    } else if (ui == PlayMenu) {
        switch (btn_pressed) {
            case A:
                ui = PlayRam;
                MainControl::displayInfo(MyPal, ui);
                // play loop from ram
                MainControl::play(MyPal); 

                break;
            case B:
                ui = PlayFlash;
                // play loop from flash
                MainControl::playFromFlash(MyPal);

                break;
            case C:
                ui = Menu;  
                // get back
                MainControl::displayInfo(MyPal, ui);
                break;
        }

    }

    else if (ui == RecordMenu) {

        switch (btn_pressed) {
            case A:
                ui = RecordRam;
                MainControl::displayInfo(MyPal, ui);
                MainControl::record(MyPal);

                // finish record
                MainControl::displayInfo(MyPal, RecordSave);
                delay(2000);

                // recover to original
                ui = Menu;
                MainControl::displayInfo(MyPal, ui);
                break;

            case B:
                // record into ram as well
                ui = RecordFlash;  
                MainControl::displayInfo(MyPal, ui);
                MainControl::recordIntoFlash(MyPal);
                break;
            case C:
                ui = Menu;
                MainControl::displayInfo(MyPal, ui);
                break;
        }

    }

    else if ((ui == PlayRam) || (ui == PlayFlash)) {
        switch (btn_pressed) {
            case A:
                Serial.println("Continue Play");

                break;
            case B:
                Serial.println("Pause");

                break;
            case C:
                Serial.println("Stop");
                ui = Menu;
                break;
        }
    }

    else if ((ui == RecordRam) || (ui == RecordFlash)) {
        switch (btn_pressed) {
            case B:
                Serial.println("Save and Stop");
                break;
            case C:
                Serial.println("stop record");
                break;
        }

        ui = Menu;
        MainControl::displayInfo(MyPal, ui);
    }
}

void MainControl::displayInfo(MyPalletizerBasic &MyPal, byte ui_mode)
{
    M5.Lcd.clear(BLACK);
    delay(50);
    M5.Lcd.setTextSize(2);
    int buttom_y = 190;
    int buttom_1y = 210;
    int buttom_2y = 210;
    ui = (enum UI)ui_mode;

    switch (ui) {
        case Menu: {
            M5.Lcd.fillScreen(0);
            if (language == Chinese) {
                M5.Lcd.drawString("  MyPal-拖动示教", 20, 40, 1);
                M5.Lcd.drawString("播放", 60, buttom_y, 1);
                M5.Lcd.drawString("录制", 160, buttom_y, 1);
                M5.Lcd.drawString("退出", 260, buttom_y, 1);
                M5.update();
            } else if (language == English) {
                M5.Lcd.clear(BLACK);
                M5.Lcd.setTextColor(BLACK);
                M5.Lcd.setTextColor(RED);
                M5.Lcd.setTextSize(3);
                M5.Lcd.setCursor(0, 10);
                M5.Lcd.printf("MyPal");
                M5.Lcd.setCursor(0, 40);
                M5.Lcd.drawFastHLine(0, 70, 320, GREY);
                M5.Lcd.setTextSize(3);
                M5.Lcd.println("MainControl Menu");
                M5.Lcd.setTextSize(2);
                M5.Lcd.setTextColor(WHITE);
                M5.Lcd.setCursor(30, 210);
                M5.Lcd.print("Play");
                M5.Lcd.setCursor(120, 210);
                M5.Lcd.print("Record");
                M5.Lcd.setCursor(230, 210);
                M5.Lcd.print("Exit");
                M5.update();
            }
            break;
        }

        case PlayMenu: {
            M5.Lcd.fillScreen(0);
            if (language == Chinese) {
                M5.Lcd.drawString(" 请选择示教路径的播放位置", 10, 40, 1);
                M5.Lcd.drawString("缓存", 60, buttom_y, 1);
                M5.Lcd.drawString("储存卡", 160, buttom_y, 1);
                M5.Lcd.drawString("返回", 260, buttom_y, 1);
            } else if (language == English) {
                M5.Lcd.setCursor(20, 40);
                M5.Lcd.print("Playing for Ram/Flash?");
                M5.Lcd.setCursor(40, buttom_1y);
                M5.Lcd.print("Ram");
                M5.Lcd.setCursor(136, buttom_1y);
                M5.Lcd.print("Flash");
                M5.Lcd.setCursor(240, buttom_1y);
                M5.Lcd.print("Back");
            }
            break;
        }

        case RecordMenu: {
            M5.Lcd.fillScreen(0);
            if (language == Chinese) {
                M5.Lcd.drawString(" 请选择示教视频储存路径", 20, 40, 1);
                M5.Lcd.drawString("缓存", 60, buttom_y, 1);
                M5.Lcd.drawString("储存卡", 160, buttom_y, 1);
                M5.Lcd.drawString("返回", 260, buttom_y, 1);
            } else if (language == English) {
                M5.Lcd.setCursor(20, 40);
                M5.Lcd.print("Recording to Ram/Flash?");
                M5.Lcd.setCursor(40, buttom_1y);
                M5.Lcd.print("Ram");
                M5.Lcd.setCursor(136, buttom_1y);
                M5.Lcd.print("Flash");
                M5.Lcd.setCursor(240, buttom_1y);
                M5.Lcd.print("Back");
            }
            break;
        }

        case PlayRam: {
            M5.Lcd.fillScreen(0);
            if (language == Chinese) {
                M5.Lcd.drawString(" 正在执行缓存中的路径", 20, 40, 1);
                M5.Lcd.drawString(" 播放中...", 20, 70, 1);
                M5.Lcd.drawString("开始", 60, buttom_y, 1);
                M5.Lcd.drawString("暂停", 160, buttom_y, 1);
                M5.Lcd.drawString("结束", 260, buttom_y, 1);
            } else if (language == English) {
                M5.Lcd.setCursor(20, 40);
                M5.Lcd.print("Play from Ram\n  Playing...");
                M5.Lcd.setCursor(40, buttom_2y);
                M5.Lcd.print("Play");
                M5.Lcd.setCursor(130, buttom_2y);
                M5.Lcd.print("Pause");
                M5.Lcd.setCursor(230, buttom_2y);
                M5.Lcd.print("Stop");
            }
            break;
        }

        case PlayFlash: {
            M5.Lcd.fillScreen(0);
            if (language == Chinese) {
                M5.Lcd.drawString(" 正在执行储存卡中的路径", 20, 40, 1);
                M5.Lcd.drawString(" 播放中...", 20, 70, 1);
                M5.Lcd.drawString("开始", 60, buttom_y, 1);
                M5.Lcd.drawString("暂停", 160, buttom_y, 1);
                M5.Lcd.drawString("结束", 260, buttom_y, 1);
            } else if (language == English) {
                M5.Lcd.setCursor(0, 40);
                M5.Lcd.print(" Play from Flash/nPlaying");
                M5.Lcd.setCursor(40, buttom_2y);
                M5.Lcd.print("Play");
                M5.Lcd.setCursor(130, buttom_2y);
                M5.Lcd.print("Pause");
                M5.Lcd.setCursor(230, buttom_2y);
                M5.Lcd.print("Stop");
            }
            break;
        }

        case RecordRam: {
            M5.Lcd.fillScreen(0);
            if (language == Chinese) {
                M5.Lcd.drawString(" 录制并存入缓存", 20, 40, 1);
                M5.Lcd.drawString(" 录制中...", 20, 70, 1);
                M5.Lcd.drawString(" 停止录制并保存", 5, buttom_y, 1);
            } else if (language == English) {
                M5.Lcd.setCursor(0, 40);
                M5.Lcd.print("Record into Ram\nRecording...");
                M5.Lcd.setCursor(5, buttom_1y);
                M5.Lcd.print("Stop Recording and Save");
            }
            break;
        }

        case RecordFlash: {
            M5.Lcd.fillScreen(0);
            if (language == Chinese) {
                M5.Lcd.drawString(" 录制并存入储存卡", 20, 40, 1);
                M5.Lcd.drawString(" 录制中...", 20, 70, 1);
                M5.Lcd.drawString(" 停止录制并保存", 5, buttom_y, 1);
            }
            if (language == English) {
                M5.Lcd.setCursor(0, 40);
                M5.Lcd.print("Record into Flash\nRecording...");
                M5.Lcd.setCursor(5, buttom_y);
                M5.Lcd.print("Stop Recording and Save");
            }

            break;
        }

        case pause: { // Stop recording
            M5.Lcd.fillScreen(0);
            if (language == Chinese) {
                M5.Lcd.drawString(" 暂停中", 20, 40, 1);
                M5.Lcd.drawString("开始", 60, buttom_y, 1);
                M5.Lcd.drawString("暂停", 160, buttom_y, 1);
                M5.Lcd.drawString("结束", 260, buttom_y, 1);
            } else if (language == English) {
                M5.Lcd.setCursor(0, 40);
                M5.Lcd.print(" Puase Now");
                M5.Lcd.setCursor(40, buttom_2y);
                M5.Lcd.print("Play");
                M5.Lcd.setCursor(130, buttom_2y);
                M5.Lcd.print("Pause");
                M5.Lcd.setCursor(230, buttom_2y);
                M5.Lcd.print("Stop");
            }
            break;
        }

        case RecordSave: { // Stop recording
            M5.Lcd.fillScreen(0);
            if (language == Chinese) {
                M5.Lcd.drawString(" 保存录制", 20, 40, 1);
            } else if (language == English) {
                M5.Lcd.setCursor(0, 40);
                M5.Lcd.print("  Saved Recording");
            }
            break;
        }

        case GDataFlash: { // Play from Flash
            M5.Lcd.fillScreen(0);
            if (language == Chinese) {
                M5.Lcd.drawString("从储存卡中获取数据", 20, 40, 1);
            } else if (language == English) {
                M5.Lcd.setCursor(0, 40);
                M5.Lcd.print("Getting data from Flash");
            }
            break;
        }

        case SDataFlash: { // Record from Flash
            M5.Lcd.fillScreen(0);
            if (language == Chinese) {
                M5.Lcd.drawString("将数据保存到储存卡", 20, 40, 1);
            } else if (language == English) {
                M5.Lcd.setCursor(0, 40);
                M5.Lcd.print("Saving Data into Flash");
            }
            break;
        }

        case IoState: { // loop play from sram
            M5.Lcd.fillScreen(0);
            if (language == Chinese) {
                M5.Lcd.drawString("IO口状态", 20, 40, 1);
            } else if (language == English) {
                M5.Lcd.setCursor(0, 40);
                M5.Lcd.print("IO Pin Active!");
            }
            break;
        }

        case NotPlay: { // loop play from sram
            M5.Lcd.fillScreen(0);
            if (language == Chinese) {
                M5.Lcd.drawString("数据不足，无法播放", 20, 40, 1);
            } else if (language == English) {
                M5.Lcd.setCursor(0, 40);
                M5.Lcd.print("Data too short, not playing");
            }
            break;
        }

        case NoData: { // loop play from sram
            M5.Lcd.fillScreen(0);
            if (language == Chinese) {
                M5.Lcd.drawString("无数据", 20, 40, 1);
            } else if (language == English) {
                M5.Lcd.setCursor(0, 40);
                M5.Lcd.print("Empty data!");
            }
            break;
        }
        default:
            ;
            break;
    }
}

void MainControl::record(MyPalletizerBasic &MyPal)  // is stop
{
    MyPal.setLEDRGB(255, 255, 0);
    // record mode : 1- record to ram;  2- record to flash
    rec_data_len = 0;
    MyPalletizerAngles _data; 
    delay(20);
    int _encoder = MyPal.getEncoder(7);
    delay(35);
    for (int data_index = 0; data_index < data_len_max ; data_index ++) {
        M5.update();
        for (int i = 0; i < 4; i++) {
            jae[data_index][i] = MyPal.getEncoder(i + 1);
            delay(REC_TIME_DELAY - SEND_DATA_GAP);

        }
        Serial.print(String(jae[data_index][3]) + ", ");
        if (_encoder > 0) {
            girrep_data[data_index] = MyPal.getEncoder(7);
            delay(REC_TIME_DELAY - SEND_DATA_GAP);
        } else {
            girrep_data[data_index] = 2048;
            delay(20);
        }

        Serial.println(" ");
        rec_data_len++;
        if (M5.BtnA.wasReleased() || M5.BtnB.wasReleased()
                || M5.BtnC.wasReleased()) break;
    }
    /*for (int i = 0; i < rec_data_len; i++) {
       Serial.print("encoder == ");
       for (int j = 0; j < 4; j++){
          Serial.print(jae[i][j]);
          Serial.print(" ");
       }  
      Serial.println();
    }*/
}


void MainControl::play(MyPalletizerBasic &MyPal)  // is stop  is pause
{
    MyPal.setLEDRGB(0, 255, 0);

    bool is_stop = 0;
    bool is_pause = 0;

    while (1) {
        // Serial.print("123");
        // play once
        for (int index = 0 ; index < rec_data_len; index++) {
            M5.update();
            MyPal.setEncoder(7, girrep_data[index]);
            delay(20);
            MyPal.setEncoders(jae[index], 100);
            Serial.print(String(jae[index][3]) + ", ");
            // check pause button
            if (M5.BtnB.wasReleased()) {
                MainControl::displayInfo(MyPal, 32);
                while (1) {
                    M5.update();
                    if (M5.BtnA.wasReleased()) {
                        MainControl::displayInfo(MyPal, 11);
                        break;
                    }
                    if (M5.BtnC.wasReleased()) {
                        is_stop = 1;
                        break;
                    }
                }
            }

            // check stop button
            if (M5.BtnC.wasReleased()) {
                M5.update();
                is_stop = 1;
                break;
            }

            // check stop button
            if (is_stop == 1) break;
            delay(WRITE_TIME_GAP * 6);
        }

        // stop button will also end loop
        if (is_stop == 1) {
            break;
        }

        // data too short
        if (rec_data_len < 10) break;

        // quick loop
        if (M5.BtnC.wasReleased())break;
    }

    // recover to play page
    M5.update();
    MyPal_mode = 1;
    MainControl::displayInfo(MyPal, MyPal_mode);
}

void MainControl::playFromFlash(MyPalletizerBasic &MyPal)
{
    M5.update();
    MainControl::displayInfo(MyPal, 41);

    // initialization first
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    Serial.printf("Reading file: %s\r\n", FILENAME);
    File file = SPIFFS.open(FILENAME);
    if (!file || file.isDirectory()) {
        Serial.println("- failed to open file for reading");
        return;
    }
    String this_line = "";
    int index = 0;

    int t1 = millis();

    while (file.available()) {
        char this_char = char(file.read());
        this_line += this_char;
        if (this_char == '\n') {
            MyCobotSaver::saver_MyPalletizer_angles_enc sae_this;
            sae_this = MyPal.saver.MyPalletizerprocessStringIntoInts(this_line);

            for (int jn = 0; jn < 4; jn++) {
                jae[index][jn] = sae_this.joint_angle[jn];
            }
            girrep_data[index] = sae_this.joint_angle[3];
            index ++;
            this_line = "";
        }
        if (index > data_len_max) break;
    }

    // update the len
    rec_data_len = index - 1;

    // play from flash
    M5.update();
    MainControl::displayInfo(MyPal, MyPal_mode);
    MainControl::play(MyPal);
}

void MainControl::recordIntoFlash(MyPalletizerBasic &MyPal)
{
    // recording data
    MainControl::record(MyPal);
    M5.update();
    // show saving to recording
    MainControl::displayInfo(MyPal, 33);

    // initialize flash
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    // list exisiting files
    MyPal.saver.listDir(SPIFFS, "/", 0);

    // clean exsiting file
    MyPal.saver.writeFile(SPIFFS, FILENAME, " ");

    // check time
    File file = SPIFFS.open(FILENAME, FILE_APPEND);

    for (int data_index = 0; data_index < rec_data_len; data_index ++) {
        String data_output = "";
        for (int l = 0; l < 4 ; l ++) {
            data_output +=  jae[data_index][l];
            data_output += ",";
        }
        data_output += girrep_data[data_index];
        data_output += "\n";
        file.print(data_output);
    }

    // recover to original
    MyPal_mode = 0;
    MainControl::displayInfo(MyPal, MyPal_mode);
}

void MainControl::IO(MyPalletizerBasic &MyPal)
{
    int pin_data = digitalRead(control_pin);

    return;
    if (pin_data == 1) {
        MainControl::displayInfo(MyPal, 51);
        delay(error_display_time);

        MyPal_mode = 12;
        MainControl::play(MyPal);
    }

}



/*
bool MainControl::checkDataLen(MyPalletizerBasic &MyPal)
{

  if (rec_data_len == 0){
    MainControl::displayInfo(MyPal, 53);
    delay(error_display_time);
    return 0;
  }

  if (rec_data_len < 10){
    MainControl::displayInfo(MyPal, 52);
    delay(error_display_time);
    return 0;
  }
  return 1;
}
*/
