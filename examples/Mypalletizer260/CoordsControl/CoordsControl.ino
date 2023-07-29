#include <MyPalletizer.h>

MyPalletizerBasic myPal;
MyPalletizerCoords coords = {200.8,-87.400,113.300,-178.260};

void setup()
{
    myPal.setup(); //This api is required
    delay(100);
    myPal.powerOn();//robot poweron
    delay(100);
    myPal.writeAngles({0, -10, -123, 45}, 50);//Initial attitude of coordinate control
    delay(6000);
}

void loop()
{
    myPal.writeCoord((Axis)3, 260, 30); //let x to 30mm
    delay(300); //Once in place, proceed to the next step
    myPal.writeCoords(coords, 30); //Multi-coordinate control
    delay(5000);
}