
#include <msp430.h>
#include "peripherals.h"
#define CALADC12_15V_30C  *((unsigned int *)0x1A1A)
#define CALADC12_15V_85C  *((unsigned int *)0x1A1C)

// ****************--- Function Prototypes ---****************\\

void displayTime(unsigned long int timeinseconds);
void resetGlobals();
void runtimerA2();
void stoptimerA2();
void flushbuffer();
void cleardisp();
float setuptempsensor();
void displayTemp(float tempinC);
float averageTemp(float t[5]);
unsigned long int whatmonth (unsigned long int secondspassed);
unsigned long int whatday (unsigned long int dayspassed,  unsigned long int month);
float scrollWheel();
char boardbuttons();
void BuzzerOnfrequency(int frequency);

// ****************--- Declare globals ---**************** \\

unsigned long int timer = 0;
unsigned long int secs = 24552324;
unsigned int in_temp;
unsigned int checkeverysec = 0, buttons = 0;
unsigned int potentiovolt;
int click = 0;
int cnt = 0;
float temps[] = {0, 0, 0, 0, 0};
// syntax for ISR

#pragma vector = TIMER2_A0_VECTOR
__interrupt void Timer_A2_ISR(void)
{
    secs++;                                                                                            //increase timer by 1 each interrupt
    checkeverysec = 4;
}


// ****************--Main--**************** \\

void main(void)
{
     _BIS_SR(GIE);                                                                                 // Global Interrupt enable
    WDTCTL = WDTPW | WDTHOLD;                                                                      // Stop watchdog timer
    REFCTL0 &= ~REFMSTR;
    ADC12CTL0 = ADC12SHT0_9 | ADC12REFON | ADC12ON | ADC12MSC;
    ADC12CTL1 = ADC12SHP |  ADC12CONSEQ_1;
    P6SEL |= BIT0; // Port 6 pin 0 to function mode in ADC
    ADC12MCTL0 = ADC12SREF_0 | ADC12INCH_0 ; //ref = 3.3V, ch = A0
    ADC12MCTL1 = ADC12SREF_1 | ADC12INCH_10 | ADC12EOS;

    __delay_cycles(50);
    ADC12CTL0 |= ADC12ENC;

//****************--- Peripherals Configuration ---**************** \\

    initLeds();                                                                                    // 4 board LEDS
    configDisplay();                                                                               // LCD
    configKeypad();                                                                                // Numpad
    configButtons();                                                                               // 4 board buttons
    cleardisp();
    runtimerA2();
//****************--- Main Loop ---**************** \\

    while (1)
    {
        displayTime(secs);
        click = boardbuttons();
        if (click != 0)
            buttons = boardbuttons();
        while (cnt < 5)
        {
            temps[cnt] = setuptempsensor();
            cnt++;
        }
        if (cnt == 5)
            cnt = 0;
        if (checkeverysec == 4)
        {
            temps[cnt] = setuptempsensor();
            float tempC = averageTemp(temps);
            displayTemp(tempC);
            checkeverysec = 0;
        }

        if(buttons == 1)
            {
            potentiovolt = scrollWheel() * 170;
            BuzzerOnfrequency(potentiovolt);
            }
        else if(buttons == 2)
            BuzzerOff();
    }
}




//****************-- Functions --****************\\

void displayTime(unsigned long int timeinseconds)
{
    char date[] = {'M', 'M', 'M', '-', 'D', 'D' , '\0'};
    char time[] = {'H', 'H', ':', 'M', 'M', ':', 'S', 'S', '\0'};
    unsigned long int months, days, hours, minutes, seconds, dayspassed;
    months = whatmonth(timeinseconds);
    dayspassed = (unsigned long int)timeinseconds/ 24L / 3600L;
    days = whatday(dayspassed, months);
    timeinseconds -= dayspassed * 24 * 3600;
    hours = timeinseconds / 3600L;
    timeinseconds -= hours * 3600L;
    minutes = timeinseconds / 60L;
    timeinseconds -= minutes * 60L;
    seconds = timeinseconds;
    if (months == 1)
    {
        date[0] = 'J';
        date[1] = 'A';
        date[2] = 'N';
    }
    else if (months == 2)
    {
        date[0] = 'F';
        date[1] = 'E';
        date[2] = 'B';
    }
    else if (months == 3)
    {
        date[0] = 'M';
        date[1] = 'A';
        date[2] = 'R';
    }
    else if (months == 4)
    {
        date[0] = 'A';
        date[1] = 'P';
        date[2] = 'R';
    }
    else if (months == 5)
    {
        date[0] = 'M';
        date[1] = 'A';
        date[2] = 'Y';
    }
    else if (months == 6)
    {
        date[0] = 'J';
        date[1] = 'U';
        date[2] = 'N';
    }
    else if (months == 7)
    {
        date[0] = 'J';
        date[1] = 'U';
        date[2] = 'L';
    }
    else if (months == 8)
    {
        date[0] = 'A';
        date[1] = 'U';
        date[2] = 'G';
    }
    else if (months == 9)
    {
        date[0] = 'S';
        date[1] = 'E';
        date[2] = 'P';
    }
    else if (months == 10)
    {
        date[0] = 'O';
        date[1] = 'C';
        date[2] = 'T';
    }
    else if (months == 11)
    {
        date[0] = 'N';
        date[1] = 'O';
        date[2] = 'V';
    }
    else if (months == 12)
    {
        date[0] = 'D';
        date[1] = 'E';
        date[2] = 'C';
    }

    date[4] = days/10 + 0x30;
    date[5] = days%10 + 0x30;

    time[0] = hours/10 + 0x30;
    time[1] = hours%10 + 0x30;

    time[3] = minutes/10 + 0x30;
    time[4] = minutes%10 + 0x30;

    time[6] = seconds/10 + 0x30;
    time[7] = seconds%10 + 0x30;

    Graphics_drawStringCentered(&g_sContext, date , AUTO_STRING_LENGTH, 50, 25, OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext, time , AUTO_STRING_LENGTH, 50, 45, OPAQUE_TEXT);
    flushbuffer();
}

unsigned long int whatmonth (unsigned long int secondspassed)
{
    unsigned long int dayspassed = (unsigned long int)secondspassed / 3600L / 24L;
    unsigned long int month;
    if (dayspassed <= 30)
        month = 1;
    else if (31 <= dayspassed && dayspassed <= 58)
        month = 2;
    else if (59 <= dayspassed && dayspassed <= 89)
        month = 3;
    else if (90 <= dayspassed && dayspassed <= 119)
        month = 4;
    else if (120 <= dayspassed && dayspassed <= 150)
        month = 5;
    else if (151 <= dayspassed && dayspassed <= 180)
        month = 6;
    else if (181 <= dayspassed && dayspassed <= 211)
        month = 7;
    else if (212 <= dayspassed && dayspassed <= 242)
        month = 8;
    else if (243L <= dayspassed && dayspassed <= 272)
        month = 9;
    else if (273 <= dayspassed && dayspassed <= 303)
        month = 10;
    else if (304 <= dayspassed && dayspassed <= 333)
        month = 11;
    else if (334 <= dayspassed && dayspassed <= 364)
        month = 12;
    return month;
}

unsigned long int whatday (unsigned long int dayspassed,  unsigned long int month)
{
    unsigned long int dayis = 0;
    if (month == 1)
        dayis = dayspassed + 1;
    else if (month == 2)
        dayis = dayspassed - 31 + 1;
    else if (month == 3)
        dayis = dayspassed - 31 - 28 + 1;
    else if (month == 4)
        dayis = dayspassed - 31 - 28 - 31 + 1;
    else if (month == 5)
        dayis = dayspassed - 31 - 28 - 31 - 30 + 1;
    else if (month == 6)
        dayis = dayspassed - 31 - 28 - 31 - 30 - 31 + 1;
    else if (month == 7)
        dayis = dayspassed - 31 - 28 - 31 - 30 - 31 - 30 + 1;
    else if (month == 8)
        dayis = dayspassed - 31 - 28 - 31 - 30 - 31 - 30 - 31 + 1;
    else if (month == 9)
        dayis = dayspassed - 31 - 28 - 31 - 30 - 31 - 30 - 31 - 31 + 1;
    else if (month == 10)
        dayis = dayspassed - 31 - 28 - 31 - 30 - 31 - 30 - 31 - 31 - 30 + 1;
    else if (month == 11)
        dayis = dayspassed - 31 - 28 - 31 - 30 - 31 - 30 - 31 - 31 - 30 - 31 + 1;
    else if (month == 12)
        dayis = dayspassed - 31 - 28 - 31 - 30 - 31 - 30 - 31 - 31 - 30 - 31 - 30 + 1;
    return dayis;
}

void resetGlobals()                                                                                   //Reset global variables
{

}
void runtimerA2(void)                                                                                 //Run Timer A2
{
    TA2CTL = (TASSEL_1 | MC_1 | ID_0);
    TA2CCR0 = 32767;                                           // 32768 / 32768 = 1 sec resolution
    TA2CCTL0 = CCIE;
}

void stoptimerA2(void)                                                                                //Stop Timer A2
{
    TA2CTL = MC_0;
    TA2CCTL0 &= ~CCIE;
}
void cleardisp()                                                                                      //Clear display, simplify syntax
{
    Graphics_clearDisplay(&g_sContext);
}

void flushbuffer()                                                                                    //Flush display, simplify syntax
{
    Graphics_flushBuffer(&g_sContext);
}

void ledsOff()                                                                                        //Turn off the 4 labboard LEDs
{
    setLeds(0);
}

float setuptempsensor()
{
    float temperatureDegC;
    float temperatureDegF;
    float degC_per_bit;
    unsigned int bits30, bits85;
    bits30 = CALADC12_15V_30C;
    bits85 = CALADC12_15V_85C;
    degC_per_bit = ((float)(85.0 - 30.0))/((float)(bits85-bits30));
    ADC12CTL0 |= ADC12SC;
    while (ADC12CTL1 & ADC12BUSY)
            __no_operation();
    in_temp = ADC12MEM1 & 0x0FFF;
    temperatureDegC = (float)((long)in_temp - CALADC12_15V_30C) * degC_per_bit +30.0;
    temperatureDegF = temperatureDegC * 1.8 + 32;
    return temperatureDegC;
}

float scrollWheel()
{
    unsigned int potReading = 0;
    float potVolts;
    ADC12CTL0 |= ADC12SC;
    while (ADC12CTL1 & ADC12BUSY)
            __no_operation();
    potReading = ADC12MEM0 & 0x0FFF; //keep low 12 bits
    potVolts = (float)potReading * 3.3/ 4096; // convert to volts
    return potVolts;
}


void displayTemp(float tempinC)
{
    char temperatureC[] = {'d', 'd', 'd', '.', 'f', 'C', '\0'};
    char temperatureF[] = {'d', 'd', 'd', '.', 'f', 'F', '\0'};
    float tempinF = tempinC *1.8 + 32;
    temperatureC[0] = (int)tempinC / 100 + 0x30;
    temperatureC[1] = (int)tempinC / 10 % 10 + 0x30;
    temperatureC[2] = (int)tempinC % 10 + 0x30;
    temperatureC[4] = (int)(tempinC * 10) % 10 + 0x30;

    temperatureF[0] = (int)tempinF / 100 + 0x30;
    temperatureF[1] = (int)tempinF / 10 % 10 + 0x30;
    temperatureF[2] = (int)tempinF % 10 + 0x30;
    temperatureF[4] = (int)(tempinF * 10) % 10 + 0x30;

    Graphics_drawStringCentered(&g_sContext, temperatureC , AUTO_STRING_LENGTH, 50, 65, OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext, temperatureF , AUTO_STRING_LENGTH, 50, 75, OPAQUE_TEXT);
    flushbuffer();
}

float averageTemp(float t[5])
{
    float average = (t[1] + t[2] + t[3] + t[4] + t[5])/5;
    return average;
}

void BuzzerOnfrequency(int frequency)                                                                //Buzz the buzzer with different frequency depends on the int frequency
{
    // Initialize PWM output on P3.5, which corresponds to TB0.5
    P3SEL |= BIT5; // Select peripheral output mode for P3.5
    P3DIR |= BIT5;

    TB0CTL  = (TBSSEL__ACLK|ID_1|MC__UP);  // Configure Timer B0 to use ACLK, divide by 1, up mode
    TB0CTL  &= ~TBIE;                       // Explicitly Disable timer interrupts for safety

    // Now configure the timer period, which controls the PWM period
    // Doing this with a hard coded values is NOT the best method
    // We do it here only as an example. You will fix this in Lab 2.
    TB0CCR0   = 32768/frequency;                   // Set the PWM period in ACLK ticks                                      //This line changes the frequency
    TB0CCTL0 &= ~CCIE;                  // Disable timer interrupts                                                         //Using ACLK, so pitch = 32768/freq

    // Configure CC register 5, which is connected to our PWM pin TB0.5
    TB0CCTL5  = OUTMOD_7;                   // Set/reset mode for PWM
    TB0CCTL5 &= ~CCIE;                      // Disable capture/compare interrupts
    TB0CCR5   = TB0CCR0/2;                  // Configure a 50% duty cycle
}

char boardbuttons()                                                                                   //Get board buttons, return a char according to the board button pressed
{
    char pressed = 0;
    if ((P7IN & BIT0)== 0x00)
        pressed = 1;
    if ((P3IN & BIT6)== 0x00)
        pressed = 2;
    if ((P2IN & BIT2)== 0x00)
        pressed = 3;
    if ((P7IN & BIT4)== 0x00)
        pressed = 4;
    return pressed;
}
