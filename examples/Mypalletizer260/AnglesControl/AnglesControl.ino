#include <MyPalletizerBasic.h>

MyPalletizerBasic myPal;
MyPalletizerAngles angles = {0, 0, 0, 0};

void setup()
{
    myPal.setup(); //This api is required
    delay(100);
    myPal.powerOn();//robot poweron
    delay(100);
}

void loop()
{
    myPal.writeAngle((Joint)1, 100, 30); //Single joint control，J1 to 100°,speed 30
    delay(1000); //Once in place, proceed to the next step
    myPal.writeAngles(angles, 50); //All joints to zero, speed 50
    delay(5000);
}