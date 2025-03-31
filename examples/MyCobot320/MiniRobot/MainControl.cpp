#include "MainControl.h"


typedef struct {
    int joint_angle[6];
} joint_angles_enc;

int data_len_max = 2000;
std::vector<Angles> jae;
std::vector<Angles> speeds;
std::vector<int> __delay__time;

std::vector<int> girrep_data;                             //int girrep_data[1400];

// PIN 26 high -> loop play data from Flash
byte control_pin = 26;   

int rec_data_len = 0;

int error_display_time = 3000;
static int EXIT = false;

void MainControl::getPDI(MyCobotBasic &myCobot)
{
//    Serial.print("getPDI");
    for (int i = 0; i + 1 < 7; ++i) {
        for (int j = 0; j < 3; ++j) {
            first_pdi[i][j] = myCobot.getServoData(i+1, cmd[j]);
            delay(10);
//            Serial.printf("f pdi %d i = %d", first_pdi[i][j], i);
            if (i == 0 && j == 0) {
                first_pdi[i][j] = myCobot.getServoData(i+1, cmd[j]);
                delay(10);
//                Serial.printf("f pdi %d i = %d", first_pdi[i][j], i);
            }
//            Serial.printf("%d %d", i+1, cmd[j]);
        }
    }
    return;
}

/*
 * change pdi,change pdi 1-->change current pdi,0-->change to before pdi
 */
void MainControl::changePD(MyCobotBasic &myCobot, bool change_pdi)
{
//    Serial.print("changepd");
    for (int i = 0; i  + 1 < 7; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (change_pdi)
                myCobot.setServoData(i+1, cmd[j], pdi[j]);
            else
                myCobot.setServoData(i+1, cmd[j], first_pdi[i][j]);
//            Serial.printf("%d %d\n", i+1, cmd[j]);
            delay(50);
        }
    }
    return;
}

void MainControl::run(MyCobotBasic &myCobot)
{
    pinMode(control_pin, INPUT);
    myCobot.setLEDRGB(0, 255, 0);
    MainControl::updateMode(myCobot, ui);
    MainControl::displayInfo(myCobot, ui);


    //进入拖动示教时，先获取pdi并保持，然后将pdi改为10 0 1
#ifdef MyCobot_Pro_350
    //getPDI(myCobot);
    delay(50);
    //changePD(myCobot, 1);
#endif
    //judge aysn or sysn
    sync = myCobot.asynOrSync();
    
    while (1) {
        M5.update();
        byte btn_pressed = 0;
        if (M5.BtnA.wasReleased()) {
            btn_pressed = 1;
            MainControl::updateMode(myCobot, btn_pressed);
        }
        if (M5.BtnB.wasReleased()) {
            btn_pressed = 2;
            MainControl::updateMode(myCobot, btn_pressed);
        }
        if (M5.BtnC.wasReleased()) {
            btn_pressed = 3;
            MainControl::updateMode(myCobot, btn_pressed);
        }
        if (EXIT) {
            EXIT = false;
            #ifdef MyCobot_Pro_350
                delay(50);
              //  changePD(myCobot, 0);
            #endif
            break;
        }
        MainControl::IO(myCobot);
    }
}

/*
 * Function: replace ui interface
 * Parameter description: The second parameter is that a function button on the interface is clicked
 */
void MainControl::updateMode(MyCobotBasic &myCobot, byte btn_pressed)
{
    btn = (enum BTN)btn_pressed;
    if (ui == Menu) {
        switch (btn) {
            case A:
                ui = PlayMenu;
                MainControl::displayInfo(myCobot, ui);
                break;
            case B:
                ui = RecordMenu;
                MainControl::displayInfo(myCobot, ui);
                //release servo 2-6 no dampe 1 need
                myCobot.releaseAllServos(1);
                if (sync)
                    delay(50);
                else
                    delay(100);
#if defined MyCobot_Pro_350
                //myCobot.releaseServo(1);
                //myCobot.focusServo(1);
                //delay(100);
#endif
                break;
            case C:
//                myCobot.setFreeMove();
//                delay(100);
                EXIT = true;
                break;
        }

    } else if (ui == PlayMenu) {
        switch (btn_pressed) {
            case A:
                ui = PlayRam;
                MainControl::displayInfo(myCobot, ui);
                // play loop from ram
                MainControl::play(myCobot); 

                break;
            //case B:
            //    ui = PlayFlash;
                // play loop from flash
            //    MainControl::playFromFlash(myCobot);
            //    break;

            case C:
                ui = Menu;  
                // get back
                MainControl::displayInfo(myCobot, ui);
                break;
        }

    }

    else if (ui == RecordMenu) {

        switch (btn_pressed) {
            case A:
                ui = RecordRam;
                MainControl::displayInfo(myCobot, ui);
                MainControl::record(myCobot);

                // finish record
                MainControl::displayInfo(myCobot, RecordSave);
                delay(2000);

                // recover to original
                ui = Menu;
                MainControl::displayInfo(myCobot, ui);
                break;

            // case B:
            //     // record into ram as well
            //     ui = RecordFlash;  
            //     MainControl::displayInfo(myCobot, ui);
            //     MainControl::recordIntoFlash(myCobot);
            //     break;
            case C:
                ui = Menu;
                for (int i = 1; i < 7; ++i) {
                    myCobot.focusServo(i);
                    delay(100);
                }
                MainControl::displayInfo(myCobot, ui);
                break;
        }
    }

        else if (ui == PlayRam) {                                                            //else if ((ui == PlayRam) || (ui == PlayFlash)) 
        switch (btn_pressed) {
            case A:
                //Serial.println("Continue Play");

                break;
            case B:
                //Serial.println("Pause");

                break;
            case C:
                //Serial.println("Stop");
                ui = Menu;
                break;
        }
    }

        else if (ui == RecordRam) {                                                              //=else if ((ui == RecordRam) || (ui == RecordFlash)) {
        switch (btn_pressed) {
            case B:
                //Serial.println("Save and Stop");
                break;
            case C:
                //Serial.println("stop record");
                break;
        }

        ui = Menu;
        MainControl::displayInfo(myCobot, ui);
    }

}

/*
 * Function: interface display
 * Return value: no return value
 * Parameter description: the second parameter is which interface to display
 */

void MainControl::displayInfo(MyCobotBasic &myCobot, byte ui_mode)
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
                M5.Lcd.drawString("  MyCobot-拖动示教", 20, 40, 1);
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
                M5.Lcd.printf("myCobot");
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
                //M5.Lcd.drawString("储存卡", 160, buttom_y, 1);
                M5.Lcd.drawString("返回", 260, buttom_y, 1);
            } else if (language == English) {
                M5.Lcd.setCursor(20, 40);
                M5.Lcd.print("Playing for Ram/Flash?");
                M5.Lcd.setCursor(40, buttom_1y);
                M5.Lcd.print("Ram");
               // M5.Lcd.setCursor(136, buttom_1y);
               // M5.Lcd.print("Flash");
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
               // M5.Lcd.drawString("储存卡", 160, buttom_y, 1);
                M5.Lcd.drawString("返回", 260, buttom_y, 1);
            } else if (language == English) {
                M5.Lcd.setCursor(20, 40);
                M5.Lcd.print("Recording to Ram/Flash?");
                M5.Lcd.setCursor(40, buttom_1y);
                M5.Lcd.print("Ram");
              //  M5.Lcd.setCursor(136, buttom_1y);
              //  M5.Lcd.print("Flash");
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

        // case PlayFlash: {
        //     M5.Lcd.fillScreen(0);
        //     if (language == Chinese) {
        //         M5.Lcd.drawString(" 正在执行储存卡中的路径", 20, 40, 1);
        //         M5.Lcd.drawString(" 播放中...", 20, 70, 1);
        //         M5.Lcd.drawString("开始", 60, buttom_y, 1);
        //         M5.Lcd.drawString("暂停", 160, buttom_y, 1);
        //         M5.Lcd.drawString("结束", 260, buttom_y, 1);
        //     } else if (language == English) {
        //         M5.Lcd.setCursor(0, 40);
        //         M5.Lcd.print(" Play from Flash/nPlaying");
        //         M5.Lcd.setCursor(40, buttom_2y);
        //         M5.Lcd.print("Play");
        //         M5.Lcd.setCursor(130, buttom_2y);
        //         M5.Lcd.print("Pause");
        //         M5.Lcd.setCursor(230, buttom_2y);
        //         M5.Lcd.print("Stop");
        //     }
        //     break;
        //}

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

        // case RecordFlash: {
        //     M5.Lcd.fillScreen(0);
        //     if (language == Chinese) {
        //         M5.Lcd.drawString(" 录制并存入储存卡", 20, 40, 1);
        //         M5.Lcd.drawString(" 录制中...", 20, 70, 1);
        //         M5.Lcd.drawString(" 停止录制并保存", 5, buttom_y, 1);
        //     }
        //     if (language == English) {
        //         M5.Lcd.setCursor(0, 40);
        //         M5.Lcd.print("Record into Flash\nRecording...");
        //         M5.Lcd.setCursor(5, buttom_y);
        //         M5.Lcd.print("Stop Recording and Save");
        //     }

        //     break;
        // }

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

        // case GDataFlash: { // Play from Flash
        //     M5.Lcd.fillScreen(0);
        //     if (language == Chinese) {
        //         M5.Lcd.drawString("从储存卡中获取数据", 20, 40, 1);
        //     } else if (language == English) {
        //         M5.Lcd.setCursor(0, 40);
        //         M5.Lcd.print("Getting data from Flash");
        //     }
        //     break;
        // }

        // case SDataFlash: { // Record from Flash
        //     M5.Lcd.fillScreen(0);
        //     if (language == Chinese) {
        //         M5.Lcd.drawString("将数据保存到储存卡", 20, 40, 1);
        //     } else if (language == English) {
        //         M5.Lcd.setCursor(0, 40);
        //         M5.Lcd.print("Saving Data into Flash");
        //     }
        //     break;
        // }

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

/*
 * Function: ram track recording
 */
void MainControl::record(MyCobotBasic &myCobot)
{
    myCobot.setLEDRGB(255, 255, 0);
    // record mode : 1- record to ram;  2- record to flash
    rec_data_len = 0;
    Angles _data;
    int _encoder = 0;
    if (myCobot.isServoEnabled(7)) {
        gripper_state = true;
    } else {
        gripper_state = false;
    }
//    Serial.print("encoder == ");
//    Serial.println(_encoder);
    Encoders encoders;
    Angles temp_speeds;
    Angles temp_back;
    int iswrite=0;
    bool flag = true;
    jae.clear();
    speeds.clear();
    __delay__time.clear();
    delay(35);
    if (!sync) {
        delay(100);
    }
    int data_index = 0;
    unsigned t_begin = millis();
    unsigned t_end;
    int delay_begin=0;
    int delay_time=0; 
    int delay_temp=0;
    while (data_index < data_len_max) {
        M5.update();
        if (M5.BtnA.wasReleased() || M5.BtnB.wasReleased()
                || M5.BtnC.wasReleased()) break;    
        //unsigned t_begin = millis();    
        if (sync) {
            temp_speeds = myCobot.getServoSpeeds();
            Serial.print("time1 = ");//测试录制时间用
            Serial.println(millis() - t_begin);//测试录制时间用
            t_end=millis()-t_begin;
            if(t_end >65000) break;
            #if defined MyCobot_Pro_350
                delay(5);
            #else
                delay(20);
            #endif    
        }    
        //t_begin = millis(); 
        encoders = myCobot.getEncoders();
        //Serial.print(encoders[1]);//这里是有更新的,但是到了下面就没有更新
        /*Serial.print("time2 = ");
        Serial.println(millis() - t_begin);*/
#if defined MyCobot_Pro_350
        delay(5);
#else
        delay(20);
#endif
        /*Serial.print("time = ");
        Serial.println(millis() - t_begin);*/
//        Serial.print("speed == ");
        if(!__delay__time.empty()) delay_begin=delay_temp;//将上次点位的时间记录为本次间隔的起点时间
        Serial.print("begin == ");
        Serial.print(delay_begin);
        for (int i = 0; i < 6; i++) {
            if (M5.BtnA.wasReleased() || M5.BtnB.wasReleased()
                || M5.BtnC.wasReleased()) break;
            if (sync) {
              if(encoders[i] > 0 && encoders[i]<4096 && temp_speeds[i] != -10000){
                if(abs(encoders[i]-temp_back[i]) > 22 && (!jae.empty()) ) iswrite=1;
              }else {
                  iswrite=0;  break;
                }
             }
 
            }                                                                          //插入当前时间,并记录该次时间,下次直接减去
              if(jae.empty()){jae.push_back(encoders);speeds.push_back(temp_speeds);__delay__time.push_back(t_end);delay_temp=t_end;temp_back=jae.back();}//__delay__time.push_back(millis())
              if(iswrite==1){//插入一条数据后,先不走这个地方,第二条再走                                                                                                               //这是当没有数据时记录第一个点位当前的时间
                iswrite=0;
                delay_temp=t_end;//本次间隔的终点
                // Serial.print("temp == ");
                // Serial.print(delay_temp);
                delay_time=delay_temp-delay_begin;//时间间隔等于当前时间减去上一个点位的时间
                //------------------------------------------------------------//
                // Serial.print("delay_time ==");
                // Serial.print(delay_time);
                //-------------------------------------------------------------//
                __delay__time.push_back(delay_time);
                jae.push_back(encoders);
                temp_back=jae.back();
                speeds.push_back(temp_speeds);
                //-------------------------------------------------------------------//
                // Serial.print("time == ");
                // Serial.print(__delay__time.back());
                //-----------------------------------------------------------------------//
                // for(int i=0;i<6;i++){
                // Serial.print(" temp_back ==  ");
                // Serial.print(temp_back[i]);
                // Serial.print(" ");
                // Serial.print("jae.size()=");
                // Serial.print(jae.size());
                // }
            //  Serial.print("speed == ");
            //  Serial.print(temp_speeds[i]);
            //  Serial.print(" ");
              }
        //Serial.println();
        if (gripper_state) {
            //unsigned long time = millis();
            girrep_data.push_back(myCobot.getEncoder(7));                                              //  girrep_data[data_index] = myCobot.getEncoder(7);
#if defined MyCobot_Pro_350
        delay(5);
#else
        delay(20);
#endif
        /*time = millis() - time;
        Serial.println(time);*/
//            Serial.print("_encoder ");
//            Serial.println(_encoder);
        }
        //rec_data_len++;//取消,取决于容器大小
        data_index++;
    }
    /*for (int i = 0; i < data_index; i++) {
       Serial.print("speed == ");
       for (int j = 0; j < 6; j++){
          if (sync) {
              Serial.print(speeds[i][j]);
              Serial.print(" ");
          }
          Serial.print(jae[i][j]);
          Serial.print(" ");
       }  
      Serial.println();
    }*/
//#endif
    for (int i = 1; i < 7; ++i) {
        myCobot.focusServo(i);
        delay(100);
    }
    myCobot.setLEDRGB(0, 255, 0); //record end,led to green
    return;
}

/*
 * Function: judge is in position目前无法正常使用
 */
bool MainControl::IsInposition(MyCobotBasic &myCobot, Angles target_encoders, Angles move_encoders)
{
    Angles current_encoders = myCobot.getEncoders();
    //int EncodersEpsilon = 11;
    //int perc[3] = {source_encoders*0.05, source_encoders*0.1, 0.2};
    float prec[6];
    //判断是否到达点位 误差为1°--》11电位值 移动电位值>300才做判断 0.05x-->1 20 1-->11 300
    for (int i = 0; i < 6; i++) {
        /*Serial.print("current_encoders == ");
        Serial.print (current_encoders[i]);
        Serial.print("  target_encoders == ");
        Serial.print (target_encoders[i]);
        Serial.print("- == ");
        Serial.print(abs(~(int)(target_encoders[i] - current_encoders[i])));*/
//        Serial.print("move_encoders == ");
//        Serial.print(i);
//        Serial.print("   ");
//        Serial.print (move_encoders[i]);
       if (move_encoders[i] < 300)
          continue; 
       else if (move_encoders[i] < 1000) 
          prec[i] = 0.05;
       else if (move_encoders[i] > 1000 && move_encoders[i] < 2000)
          prec[i] = 0.10;
        else if(move_encoders[i] > 2000 && move_encoders[i] < 3000)
          prec[i] = 0.15;
        else if(move_encoders[i] > 3000 && move_encoders[i] < 4096)
          prec[i] = 0.20;
        if(abs(~(int)(target_encoders[i] - current_encoders[i])) > move_encoders[i] * prec[i]) {
            return 0;
        } 
    }
    return 1;
}

/*
 * Function: ram track playback
 */
void MainControl::play(MyCobotBasic &myCobot)  
{
    myCobot.setLEDRGB(0, 255, 0);

    bool is_stop = 0;
    bool is_pause = 0;

    while (1) {
        // play once
        for (int index = 0 ; index < jae.size(); index++) {
            M5.update();
            if (!sync) {
                myCobot.setEncoders(jae[index], 100);
            } else {          
                if (index == 0) {
                      myCobot.setEncodersDrag(jae[index], speeds[index]);
                      delay(5);
                      Angles source_encoders = myCobot.getEncoders();
                      Angles move_encoders;
                      float temp_encoder = 0;
                      //float last_encoder = 0;
                      float max_encoder = 0;
                      for (int i = 0; i < 6; i++) {
                          temp_encoder = abs(~(int)(jae[index][i] - source_encoders[i]));
                          move_encoders[i] = temp_encoder;
                          if (temp_encoder > max_encoder)
                              max_encoder = temp_encoder;
                      }
      //                Serial.println(max_encoder);
                      delay(max_encoder+500);
                      /*while (!IsInposition(myCobot, jae[index], move_encoders)) {
                          myCobot.setEncodersDrag(jae[index], speeds[index]);
                          delay(60);
                      }*/
                  } else {
                          delay(__delay__time[index]-20);
                          myCobot.setEncodersDrag(jae[index], speeds[index]);
                  }
            }
#if defined MyCobot_Pro_350
                delay(20);
#else
                if (!sync)
                    delay(30);
                else
                    delay(74);
#endif
            if (gripper_state) {
                myCobot.setEncoder(7, girrep_data[index]);
#if defined MyCobot_Pro_350
                delay(20);
#else
                delay(35);
#endif
            }
            /*for (int j = 0; j < 6; j++){
                if (sync) {
                    Serial.print(speeds[index][j]);
                    Serial.print(" ");
                }
                Serial.print(jae[index][j]);
                Serial.print(" ");
             }  
             Serial.println();  */     
            // check pause button
            if (M5.BtnB.wasReleased()) {
                MainControl::displayInfo(myCobot, pause);
                while (1) {
                    M5.update();
                    if (M5.BtnA.wasReleased()) {
                        MainControl::displayInfo(myCobot, 11);
                        break;
                    }
                    if (M5.BtnC.wasReleased()) {
                        myCobot.clearServoQueue();
                        is_stop = 1;
                        break;
                    }
                }
            }

            // check stop button
            if (M5.BtnC.wasReleased()) {
                M5.update();
                myCobot.clearServoQueue();
                is_stop = 1;
                break;
            }

            // check stop button
            if (is_stop == 1) break;
        }

        // stop button will also end loop
        if (is_stop == 1) {
            break;
        }

        // data too short
        if (jae.size() < 10) break;

        // quick loop
        if (M5.BtnC.wasReleased())break;
    }

    // recover to play page
    M5.update();
    ui = PlayMenu;
    MainControl::displayInfo(myCobot, ui);
}

/*
 * Function: Play the track in the memory card
 */

// void MainControl::playFromFlash(MyCobotBasic &myCobot)
// {
//     M5.update();
//     MainControl::displayInfo(myCobot, GDataFlash);

//     // initialization first
//     if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
//         //Serial.println("SPIFFS Mount Failed");
//         return;
//     }

//     //Serial.printf("Reading file: %s\r\n", FILENAME);
//     File file = SPIFFS.open(FILENAME);
//     if (!file || file.isDirectory()) {
//         //Serial.println("- failed to open file for reading");
//         return;
//     }
//     String this_line = "";
//     int index = 0;

//     int t1 = millis();

//     while (file.available()) {
//         char this_char = char(file.read());
//         this_line += this_char;
//         if (this_char == '\n') {
//             MyCobotSaver::saver_angles_enc sae_this;
//             sae_this = myCobot.saver.processStringIntoInts(this_line);

//             for (int jn = 0; jn < 6; jn++) {
//                 jae[index][jn] = sae_this.joint_angle[jn];
//             }
//             girrep_data[index] = sae_this.joint_angle[6];
//             index ++;
//             this_line = "";
//         }
//         if (index > data_len_max) break;
//     }

//     // update the len
//     rec_data_len = index - 1;

//     // play from flash
//     M5.update();
//     MainControl::displayInfo(myCobot, ui);
//     MainControl::play(myCobot);
// }

// /*
//  * Function: save the track to the memory card
//  */

// void MainControl::recordIntoFlash(MyCobotBasic &myCobot)
// {
//     // recording data
//     MainControl::record(myCobot);
//     M5.update();
//     // show saving to recording
//     MainControl::displayInfo(myCobot, RecordSave);

//     // initialize flash
//     if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
//         //Serial.println("SPIFFS Mount Failed");
//         return;
//     }

//     // list exisiting files
//     myCobot.saver.listDir(SPIFFS, "/", 0);

//     // clean exsiting file
//     myCobot.saver.writeFile(SPIFFS, FILENAME, " ");

//     // check time
//     File file = SPIFFS.open(FILENAME, FILE_APPEND);

//     for (int data_index = 0; data_index < rec_data_len; data_index ++) {
//         String data_output = "";
//         for (int l = 0; l < 6 ; l ++) {
//             data_output +=  jae[data_index][l];
//             data_output += ",";
//         }
//         data_output += girrep_data[data_index];
//         data_output += "\n";
//         file.print(data_output);
//     }

//     // recover to original
//     ui = Menu;
//     MainControl::displayInfo(myCobot, ui);
// }

void MainControl::IO(MyCobotBasic &myCobot)
{
    int pin_data = digitalRead(control_pin);

    return;
    if (pin_data == 1) {
        MainControl::displayInfo(myCobot, 51);
        delay(error_display_time);

        //ui = PlayFlash;
        MainControl::play(myCobot);
    }
}
