#include <MyCobotBasic.h>

MyCobotBasic myCobot;

void setup()
{
    myCobot.setup(); //This api is required
    delay(100);
    myCobot.powerOn();//robot poweron
    delay(100);
}

void loop()
{
    myCobot.setGripperValue(80, 50); //let the gripper to 80°,speed 50
    delay(500); //Once in place, proceed to the next step
    myCobot.setGripperValue(20, 50); //let the gripper to 20°,speed 50
    delay(500); 
    myCobot.setGripperState(0, 30); //open the gripper,speed 30
    delay(600);
    myCobot.setGripperState(1, 30); //close the gripper, speed 30
    delay(600);
}