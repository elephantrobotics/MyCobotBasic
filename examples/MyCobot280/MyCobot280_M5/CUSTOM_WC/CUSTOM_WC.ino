/*
 * 程序描述：
 * 该程序用于控制MyCobot机器人的基本运动。它通过检测两个输入引脚（IN_1和IN_2）的状态变化来暂停和恢复机器人的运动。
 * 当IN_1引脚检测到持续200毫秒的下降沿信号时，机器人暂停；
 * 当IN_2引脚检测到持续200毫秒的上升沿信号时，机器人恢复运动。
 * 如果两个信号同时有效，则信号无效，不会进行任何操作。
 *
 * 使用方法：
 * 1. 硬件连接：
 *    - 将IN_1引脚连接到一个带上拉电阻的输入源（例如按钮），默认高电平。
 *    - 将IN_2引脚连接到一个带下拉电阻的输入源（例如按钮），默认低电平。
 * 2. 软件设置：
 *    - 通过MyCobotBasic库初始化和控制机器人。
 *    - 在Arduino IDE中编译和上传该代码到对应的硬件平台。
 *
 * 程序功能：
 * 1. 初始化MyCobot机器人，并设置输入引脚的模式。
 * 2. 在主循环中持续检测输入引脚的状态变化。
 * 3. 根据检测到的信号状态，控制机器人的暂停和恢复。
 * 4. 提供调试信息，通过串口输出当前输入引脚的状态和检测结果。
 */

#include <MyCobotBasic.h>

#define IN_1 2
#define IN_2 5

MyCobotBasic myCobot;
//如果要变更点位，修改此处即可
Angles angles[6] = {{0, 0, 0, 0, 0, 0}, {45, 20, 0, 0, 90, 90},
                {-2.19, -4.92, -79.98, -2.81, -3.95, 19.77}, {49.13, -1.14, -81.38, -4.74, 0.7, 19.77},
                {18.89, 70.75, -94.13, -60.2, -1.93, 19.77}, {19.24, -76.99, 123.57, -68.2, -1.93, 19.77}};
int sp = 90;     
int ios_count[2] = {0,0}; 
byte play_index = 0;    
bool is_pause = false, io_states[2] = {0, 0};    
 
int currentState[2] = {-1, -1};  
int previousState[2] = {-1, -1}; 
unsigned long lowStartTime = 0; 
unsigned long highStartTime = 0; 
#define PLAY_TIME 1000
//#define DEBUG

void setup()
{
    myCobot.setup(); //This api is required
    delay(100);
    myCobot.powerOn();//robot poweron
    delay(100);
    InitIo();
    xTaskCreatePinnedToCore(TaskDealRecvIn, "TaskDealRecvIn", 10000, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(TaskDebug, "TaskDebug", 10000, NULL, 1, NULL, 1);
}

/**
 * 运行6个点位
 */
void loop()
{
    while (!is_pause) {
#ifdef DEBUG
          Serial.printf("digitalRead1 %d, %d\n", digitalRead(IN_1), digitalRead(IN_2));
#endif
          myCobot.writeAngles(angles[play_index], sp); //All joints to zero, speed 50
          vTaskDelay(PLAY_TIME);
          if(++play_index == 7)
            play_index = 0;
    }
}

static void TaskDealRecvIn(void *p)
{
    DealRecvIn();
}

static void TaskDebug(void *p)
{
    Debug();
}

/**
 * 处理io信号
 */
void DealRecvIn()
{
    while (1) {
        DoTaskA();
        DoTaskB();
        vTaskDelay(10);
    }
}

/**
 * debug接口
 */
void Debug()
{
    while (1) {
        Serial.printf("%d, %d \n", ios_count[0], ios_count[1]);
        vTaskDelay(10);
    }
}

/**
 * 初始化引脚为上拉电阻
 */
void InitIo()
{
    pinMode(IN_1, INPUT_PULLUP);
    pinMode(IN_2, INPUT_PULLDOWN);
}

/** 读取io当前状态
 *  参数一：引脚号
 * 返回值：状态，0-默认状态 1-检测到下降沿 2-检测到上升沿 
 */
byte getBasicIn(byte pin_no)
{
    byte io_index = 0;
    if (pin_no == 5)
       io_index = 1;
    byte state = 0;
    currentState[io_index] = digitalRead(pin_no); // 读取当前引脚状态
#ifdef DEBUG
    Serial.printf("检测到 %d, %d, %d, %d, %d, %d\n", previousState[io_index], currentState[io_index],
      io_states[0], io_states[1], digitalRead(pin_no), io_index);
#endif
    // 检测到下降沿
    if (previousState[io_index] == HIGH && currentState[io_index] == LOW) {
      io_states[io_index] = true;
      lowStartTime = millis(); // 记录引脚变为低电平的时间
    }
    // 检查引脚是否持续低电平200毫秒
    if (((currentState[io_index] = digitalRead(pin_no)) == LOW) 
            && (millis() - lowStartTime) > 200
            && io_states[io_index]) {
        state = 1;
        io_states[io_index] = false;
#ifdef DEBUG
        Serial.printf("%d,%d,%d 引脚从高电平变为低电平并持续200毫秒\n", pin_no,
          io_states[0], io_states[1]);
#endif
    }

    if (previousState[io_index] == LOW && currentState[io_index] == HIGH) {
        io_states[io_index] = true;
        highStartTime = millis(); // 记录引脚变为高电平的时间
    }
    // 检查引脚是否持续高电平200毫秒
    if (((currentState[io_index] = digitalRead(pin_no)) == HIGH) 
            && (millis() - highStartTime) > 200
            && io_states[io_index]) {
      state = 2;
      io_states[io_index] = false;
#ifdef DEBUG
      Serial.printf(" %d,%d,%d 引脚从低电平变为高电平并持续200毫秒", pin_no,io_states[0], io_states[1]);
#endif
    }
    previousState[io_index] = currentState[io_index]; // 更新前一个引脚状态
    return state;
}

/**
 * 检测到下降沿，机器暂停运动
 */
void DoTaskA()
{
    if (getBasicIn(IN_1) == 1) {
          ++ios_count[0];
          if (!is_pause) {
              is_pause = true;
              myCobot.ProgramPause();
          }
#ifdef DEBUG
          Serial.println("pause ---------------------------------------------------------------------------");
#endif
    }
    return;
}

/**
 * 检测到上升沿，机器恢复运动
 */
void DoTaskB()
{
    if (getBasicIn(IN_2) == 2) {
          ++ios_count[1];
          if (is_pause) {
              is_pause = false;
              myCobot.ProgramResume();
          }
#ifdef DEBUG
          Serial.println("resume ---------------------------------------------------------------------------");
#endif
    }
    return;
}
