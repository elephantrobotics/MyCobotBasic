#include <MyPalletizer.h>

MyPalletizer myPal;

void setup()
{
    myPal.setup(); //This api is required
    delay(100);
    myPal.powerOn();//robot poweron
    delay(100);
}

void loop()
{
    myPal.setGripperValue(80, 50); //let the gripper to 80°,speed 50
    delay(500); //Once in place, proceed to the next step
    myPal.setGripperValue(20, 50); //let the gripper to 20°,speed 50
    delay(500); 
    myPal.setGripperState(0, 30); //open the gripper,speed 30
    delay(600);
    myPal.setGripperState(1, 30); //close the gripper, speed 30
    delay(600);
}