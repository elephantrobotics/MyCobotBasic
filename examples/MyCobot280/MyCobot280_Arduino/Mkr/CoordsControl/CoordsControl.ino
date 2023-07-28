#include <MyCobotBasic.h>

MyCobotBasic myCobot;

Coords coords = {194.700,-67.400,131.300,-177.260,-3.760,-110.880};

void setup()
{
    myCobot.setup(); //This api is required
    delay(100);
    myCobot.powerOn();//robot poweron
    delay(100);
    myCobot.writeAngles({0, -10, -123, 45, 0, 20}, 50);//Initial attitude of coordinate control
    delay(6000);
}

void loop()
{
    myCobot.writeCoord((Axis)1, 30, 30); //let x to 30mm
    delay(300); //Once in place, proceed to the next step
    myCobot.writeCoords(coords, 30); //Multi-coordinate control
    delay(5000);
}