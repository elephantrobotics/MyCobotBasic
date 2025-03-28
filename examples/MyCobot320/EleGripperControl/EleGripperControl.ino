/**
 * @file EleGripperControl.ino
 * @author Elephant Robotics
 * @brief 
 * @version 2.0
 * @date 2023-07-28
 * 
 * @copyright Copyright (c) 2023
 * About the eletric gripper,powered on successfully, the red light blinks
        after the claw initialization is successful, the blue light will blink
 */
#include <MyCobotBasic.h>

MyCobotBasic myCobot;

void setup()
{
    myCobot.setup(); //This api is required
    delay(100);
    myCobot.powerOn();//robot poweron
    delay(100);

    myCobot.InitEletricGripper(); //when the gripper power on,need first init,then control
    delay(100);
}

void loop()
{
    myCobot.setEletricGripper(0); //open the gripper
    delay(600);
    myCobot.setEletricGripper(1); //close the gripper
    delay(600);
}