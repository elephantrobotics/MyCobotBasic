#include <MyCobotBasic.h>

MyCobotBasic myCobot;
Angles angles = {0, 0, 0, 0, 0, 0};

void setup()
{
    myCobot.setup(); //This api is required
    delay(100);
    myCobot.powerOn();//robot poweron
    delay(100);
}

void loop()
{
    myCobot.writeAngle((Joint)1, 100, 30); //Single joint control，J1 to 100°,speed 30
    delay(200); //Once in place, proceed to the next step
    myCobot.writeAngles(angles, 50); //All joints to zero, speed 50
    delay(5000);
}