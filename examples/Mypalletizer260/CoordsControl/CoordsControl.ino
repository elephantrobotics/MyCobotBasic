#include <MyPalletizerBasic.h>

MyPalletizerBasic myPal;
MyPalletizerCoords coords = {179,-9.6, 126, 119};

void setup()
{
    myPal.setup(); //This api is required
    delay(100);
    myPal.powerOn();//robot poweron
    delay(100);
    myPal.writeAngles({3.33, 6.67, -2.98, 98}, 50);//Initial attitude of coordinate control
    delay(6000);
}

void loop()
{
    myPal.writeCoord((MyPalletizerAxis)2, 100, 30); //let x to 30mm
    delay(1000); //Once in place, proceed to the next step
    myPal.writeCoords(coords, 30); //Multi-coordinate control
    delay(5000);
}