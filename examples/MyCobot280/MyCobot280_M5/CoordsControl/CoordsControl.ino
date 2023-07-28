#include <MyCobotBasic.h>

MyCobotBasic myCobot;

Coords coords = {200.8,-87.400,113.300,-178.260,-30.760,-60.880};

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
    myCobot.writeCoord((Axis)3, 260, 30); //let x to 30mm
    delay(300); //Once in place, proceed to the next step
    myCobot.writeCoords(coords, 30); //Multi-coordinate control
    delay(5000);
}