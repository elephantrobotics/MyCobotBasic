#ifndef MainControl_h
#define MainControl_h

#include <MyCobotBasic.h>
#include "config.h"
#include "ServerBase.h"
#include <vector>

class MainControl: public ServerBase
{
private:
    void updateMode(MyCobotBasic &myCobot, byte btn_pressed);
    void changePD(MyCobotBasic &myCobot, bool change_pdi);
    void getPDI(MyCobotBasic &myCobot);
    void displayInfo(MyCobotBasic &myCobot, byte mc_mode);
    void record(MyCobotBasic &myCobot);
    void play(MyCobotBasic &myCobot);
    //void playFromFlash(MyCobotBasic &myCobot);
    //void recordIntoFlash(MyCobotBasic &myCobot);
    void IO(MyCobotBasic &myCobot);
    bool IsInposition(MyCobotBasic &myCobot, Angles target_encoders, Angles move_encoders);
public:
    void run(MyCobotBasic &myCobot);
    static ServerBase *createInstance()
    {
        return new MainControl();
    }

public:
    //The maincontrol interface displays GDataFlash-->get data from flash SDataFlash-->save data to flash
    enum UI : int {Menu = 0, PlayMenu = 1, RecordMenu = 2, PlayRam = 11, RecordRam = 21,  pause = 32,
                   RecordSave = 33, IoState = 51, NotPlay = 52, NoData = 53  //RecordFlash = 22,PlayFlash = 12, GDataFlash = 41, SDataFlash = 42
                  } ui = (enum UI)0;
    //Button              
    enum BTN : int {A = 1, B, C} btn;
    bool gripper_state{false};
    uint8_t first_pdi[6][3] = {{0, 0, 0}};
    uint8_t pdi[3] = {10, 0, 1};
    uint8_t cmd[3] = {21, 22, 23}; 
    bool sync{false}; //同步为true 异步为fasle 
};

#endif
