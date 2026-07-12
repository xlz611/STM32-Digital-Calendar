/**
 * @file    main.c
 * @brief   基于 STM32F103C8T6 的多功能数字式万年历 — 主程序
 *          功能：时钟 / 日历 / 闹钟 / 秒表 四种模式，按键切换与参数校准
 * @author  叶加彬
 * @note    依赖外设驱动头文件：led_sd.h / delay.h / sys.h / usart.h / key.h / timer.h
 *          这些为 STM32F1 标准外设库工程脚手架，需配合 Keil MDK5 工程编译。
 */

#include "stm32f10x.h"
#include "led_sd.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "key.h"
#include "timer.h"

/* ---------- 全局时间/参数变量 ---------- */
uint8_t flag = 0, flag_set = 0;                         /* flag: 模式索引; flag_set: 设置项索引 */
uint8_t hour = 12, min = 34, sec = 56;                 /* 时钟：时/分/秒 */
uint8_t year = 24, mon = 11, day = 26;                 /* 日历：年/月/日（年为偏移值，实际显示需+2000） */
uint8_t B_hour = 12, B_min = 35, B_sec = 0;            /* 闹钟：时/分/秒 */
uint8_t shi = 1, fen = 0, miao = 0;                    /* 秒表：时/分/秒 */

/**
 * @brief  LED 数码管动态扫描显示
 * @param  i,j,k  依次显示的十位/个位/分隔符组成的数值组（时:分:秒 或 年:月:日）
 */
void show_num(uint8_t i, uint8_t j, uint8_t k)
{
    LED_SD_Dpy(8, i / 10 % 10);
    delay_ms(1);
    LED_SD_Dpy(7, i % 10);
    delay_ms(1);
    LED_SD_Dpy(6, 10);          /* 10 表示显示 "-" 分隔符 */
    delay_ms(1);
    LED_SD_Dpy(5, j / 10 % 10);
    delay_ms(1);
    LED_SD_Dpy(4, j % 10);
    delay_ms(1);
    LED_SD_Dpy(3, 10);
    delay_ms(1);
    LED_SD_Dpy(2, k / 10 % 10);
    delay_ms(1);
    LED_SD_Dpy(1, k % 10);
    delay_ms(1);
}

/**
 * @brief  按键扫描与处理（模式切换 / 加 / 减 / 确认启停）
 *         采用 10ms 软件延时消抖，查询方式检测 KEY1~KEY4
 */
void KEY()
{
    /* KEY1：模式切换键 —— 循环切换 时钟/日历/手动校准/秒表 */
    if (KEY1_STA == KEY_DN)
    {
        delay_ms(1);
        if (KEY1_STA == KEY_DN)
        {
            while (KEY1_STA == KEY_DN);
            flag_set++;
            if (flag_set > 3)
            {
                flag_set = 0;
            }
        }
    }

    /* KEY2：加键 —— 在设置模式下对应参数 +1，越界循环 */
    if (KEY2_STA == KEY_DN)
    {
        delay_ms(1);
        if (KEY2_STA == KEY_DN)
        {
            while (KEY2_STA == KEY_DN);
            if (flag == 0)                /* 时钟设置 */
            {
                if (flag_set == 1)      { sec++;  if (sec > 59)  sec = 0; }
                else if (flag_set == 2) { min++;  if (min > 59)  min = 0; }
                else if (flag_set == 3) { hour++; if (hour > 23) hour = 0; }
            }
            else if (flag == 1)          /* 日历设置 */
            {
                if (flag_set == 1)      { year++; }
                else if (flag_set == 2) { mon++;  if (mon > 12)  mon = 1; }
                else if (flag_set == 3) { day++;  if (day > 31)  day = 1; }
            }
            else if (flag == 3)          /* 闹钟设置 */
            {
                if (flag_set == 1)      { B_sec++;  if (B_sec > 59)  B_sec = 0; }
                else if (flag_set == 2) { B_min++;  if (B_min > 59)  B_min = 0; }
                else if (flag_set == 3) { B_hour++; if (B_hour > 23) B_hour = 0; }
            }
            else if (flag == 2)          /* 秒表设置（此处为手动校准入口） */
            {
                if (flag_set == 1)      { miao++; if (miao > 59) miao = 0; }
                else if (flag_set == 2) { fen++;  if (fen > 59)  fen = 0; }
                else if (flag_set == 3) { shi++;  if (shi > 23)  shi = 0; }
            }
        }
    }

    /* KEY3：减键 —— 在设置模式下对应参数 -1，越界循环 */
    if (KEY3_STA == KEY_DN)
    {
        delay_ms(1);
        if (KEY3_STA == KEY_DN)
        {
            if (flag == 0)
            {
                if (flag_set == 1)      { sec--;  if (sec < 0)  sec = 59; }
                else if (flag_set == 2) { min--;  if (min < 0)  min = 59; }
                else if (flag_set == 3) { hour--; if (hour < 0) hour = 59; }
            }
            else if (flag == 1)
            {
                if (flag_set == 1)      { year--; }
                else if (flag_set == 2) { mon--;  if (mon < 0)  mon = 12; }
                else if (flag_set == 3) { day--;  if (day < 0)  day = 31; }
            }
            else if (flag == 3)
            {
                if (flag_set == 1)      { B_sec--;  if (B_sec < 0)  B_sec = 59; }
                else if (flag_set == 2) { B_min--;  if (B_min < 0)  B_min = 59; }
                else if (flag_set == 3) { B_hour--; if (B_hour < 0) B_hour = 23; }
            }
            else if (flag == 2)
            {
                if (flag_set == 1)      { miao--; if (miao < 0) miao = 59; }
                else if (flag_set == 2) { fen--;  if (fen < 0)  fen = 59; }
                else if (flag_set == 3) { shi--;  if (shi < 0)  shi = 23; }
            }
            while (KEY3_STA == KEY_DN);
        }
    }

    /* KEY4：确认/启停键 —— 切换功能模式（flag 0→1→2→3→0） */
    if (KEY4_STA == KEY_DN)
    {
        delay_ms(1);
        if (KEY4_STA == KEY_DN)
        {
            flag++;
            flag_set = 0;
            if (flag > 2)        /* 注意：实际模式循环为 0/1/2/3，见下方说明 */
            {
                flag = 0;
            }
            while (KEY4_STA == KEY_DN);
        }
    }
}

/**
 * @brief  主函数：初始化外设，循环显示当前模式并处理按键
 */
int main(void)
{
    LED_SD_Cfg();                /* LED 数码管 GPIO 配置 */
    delay_init();                /* 延时初始化 */
    Key_Cfg();                   /* 按键 GPIO 配置（上拉输入） */
    NVIC_Configuration();        /* 中断优先级配置 */
    TIM3_Int_Init(1000, 7199);   /* TIM3：10KHz 计数，周期 1000 → 1 秒定时中断 */

    while (1)
    {
        if (flag == 0)           show_num(hour, min, sec);     /* 时钟模式 */
        else if (flag == 1)      show_num(year, mon, day);     /* 日历模式 */
        else if (flag == 3)      show_num(B_hour, B_min, B_sec); /* 闹钟模式 */
        else if (flag == 2)      show_num(shi, fen, miao);     /* 秒表模式 */

        /* 拉高蜂鸣器与按键相关引脚（此处为初始化/释放状态） */
        GPIO_SetBits(GPIOB, GPIO_Pin_12);
        GPIO_SetBits(GPIOB, GPIO_Pin_9);
        GPIO_SetBits(GPIOB, GPIO_Pin_10);
        GPIO_SetBits(GPIOB, GPIO_Pin_11);

        KEY();                   /* 按键扫描 */
    }
}

/**
 * @brief  TIM3 中断服务函数（1 秒触发一次）
 *         - 秒表模式（flag==2 且未在设置）：秒表递减计时
 *         - 其他模式：时钟秒数递增，完成 60 进位逻辑
 */
void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);

        if (flag == 2 && flag_set == 0)       /* 秒表运行（递减） */
        {
            if (miao > 0)         { miao--; }
            else if (fen > 0)     { fen--;  miao = 59; }
            else if (shi > 0)     { shi--;  fen = 59; miao = 59; }
            else                  { miao = 0; }
        }
        else                      /* 时钟递增 */
        {
            sec++;
            if (sec == 60)  { sec = 0;  min++;
                if (min == 60) { min = 0; hour++;
                    if (hour == 24) { hour = 0; }
                }
            }
        }
    }
}
