#include <MyCobotBasic.h>

MyCobotBasic myCobot;
Angles angles = {0, 0, 0, 0, 0, 0};
Angles run[13]={
{-0.79, -81.21, 179.47, -1.23, 85.07, -13.44},
{-0.79, 48.42, -154.16, -0.7, -77.43, -13.62},
{-0.35, 8.43, -90.17, 0.7, -63.19, -13.62},
{-0.26, 8.61, -90.35, 36.82, -63.36, 172.35},
{-0.35, 8.43, -90.17, 0.7, -63.19, -13.62},
{-0.7, 10.63, -90.35, -46.23, -73.21, -164.0},
{-0.35, 8.43, -90.17, 0.7, -63.19, -13.62},
{43.76, 8.78, -90.35, 0.43, -63.45, -13.18},
{43.76, -41.57, -90.35, 0.87, -63.45, -13.18},
{-43.76, 8.78, -90.35, 0.43, -63.45, -13.18},
{-43.76, -41.57, -90.35, 0.43, -63.45, -13.18},
{-43.76, 8.78, -90.35, 0.43, -63.45, -13.18},
{-0.79, 48.42, -154.16, -0.7, -77.43, -13.62},
};

void setup()
{
  myCobot.setup();
  delay(100);
  myCobot.powerOn();
  delay(100);
  myCobot.writeAngles({0, 0, 0, 0, 0, 0}, 50);
}

void loop()
{
  int i=0;
  for(int i=0;i<11;i++)
  {
      angles=run[i];
      delay(700);
      myCobot.writeAngles(angles, 50);
      delay(500);
  }
}
// MyCobotBasic myCobot;
// Angles angles = {0, 0, 0, 0, 0, 0};

// void setup()
// {
//     myCobot.setup(); //This api is required
//     delay(100);
//     myCobot.powerOn();//robot poweron
//     delay(100);
// }

// void loop()
// {
//     myCobot.writeAngle((Joint)1, 100, 30); //Single joint control，J1 to 100°,speed 30
//     delay(200); //Once in place, proceed to the next step
//     myCobot.writeAngles(angles, 50); //All joints to zero, speed 50
//     delay(5000);
// }
