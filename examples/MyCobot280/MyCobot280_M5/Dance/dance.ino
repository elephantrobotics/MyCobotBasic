#include <MyCobotBasic.h>

MyCobotBasic myCobot;
Angles angles = {{0, 0, 0, 0, 0, 0}, {90, 70, 0, 0, 90, 90},
                {90, -70, 0, 0, -90, -90}, {-90, -70, 0, 0, -90, -90},
                {120, 55, -90, 90, -90, 90}, {-120, -55, 90, -90, 90, -90}};
int sp = 90;                

void setup()
{
    myCobot.setup(); //This api is required
    delay(100);
    myCobot.powerOn();//robot poweron
    delay(100);
}

void loop()
{
    for (int i = 0; i < angles.size(); ++i) {
        myCobot.writeAngles(angles[i], sp); //All joints to zero, speed 50
        delay(2500);
    }
}