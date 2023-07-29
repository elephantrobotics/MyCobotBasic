#include <MyCobotBasic.h>

MyCobotBasic myCobot;

Coords coords = {175, -48, 130, 165, 45, 152};

void setup()
{
    myCobot.setup(); //This api is required
    delay(100);
    myCobot.powerOn();//robot poweron
    delay(100);
    myCobot.writeAngles({5, 10, 68, -27, -24, 34}, 50);//Initial attitude of coordinate control
    delay(6000);
}

void loop()
{
    myCobot.writeCoord((Axis)3, 100, 30); //let x to 30mm
    delay(300); //Once in place, proceed to the next step
    myCobot.writeCoords(coords, 30); //Multi-coordinate control
    delay(5000);
}