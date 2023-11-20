#include "lcd.h"
#include "graphics.h"
#include "color.h"
#include "ports.h"
#include "lab8.h"

int ch1[128]; int ch2[128]; int ch3[128]; int ch4[128];
int freq[5] = {240, 350, 1000, 2000, 4000};
int index = 0, flag = 0, mode = 0;

void introScreen(void) {
    clearScreen(1);
    setColor(COLOR_16_WHITE);
    drawLine(16, 0, 16, 89);
    drawLine(0, 89, 159, 89);
    drawString(0, 0, FONT_MD, "CH");
    drawString(20, 0, FONT_MD, "Logic Analyzer");
    drawString(7, 36, FONT_MD, "2");
    drawString(7, 72, FONT_MD, "4");
    drawString(2, 96, FONT_SM, "Trigger: OFF");
    drawString(2, 106, FONT_SM, "Frequency: 10 kHz");
    drawString(2, 116, FONT_SM, "Samples: 256");
    setColor(COLOR_16_CYAN);
    drawString(7, 18, FONT_MD, "1");
    drawString(7, 54, FONT_MD, "3");

    int i = 44;
    while (i < 150) {
        drawLine(i, 88, i, 87); i+=14;
    }
}

void writeData(uint8_t data) {
    P2OUT &= ~LCD_CS_PIN;
    P4OUT |= LCD_DC_PIN;
    UCB0TXBUF = data;
    while(UCB0STATW & UCBUSY);
    P2OUT |= LCD_CS_PIN;
}

void writeCommand(uint8_t command) {
    P2OUT &= ~LCD_CS_PIN;
    P4OUT &= ~LCD_DC_PIN;
    UCB0TXBUF = command;
    while(UCB0STATW & UCBUSY);
    P2OUT |= LCD_CS_PIN;
}

void initMSP430(void) {

    /*********************** Test Data Generator 1 kHz *********************/
    P9DIR   |= BIT1 | BIT2 | BIT3 | BIT4;   // Test Data Output
    TA2CTL   = TASSEL__SMCLK | MC__CONTINUOUS | TACLR;
    TA2CCR1  = 1000;                                    // 1MHz * 1/1000 Hz
    TA2CCTL1 = CCIE;                                    // enable interrupts
    testcnt  = 0;                                       // start test counter at 0

    /**************************** PWM Backlight ****************************/

    P1DIR   |= BIT3;
    P1SEL0  |= BIT3;
    P1SEL1  &= ~BIT3;

    TA1CCR0  = 511;
    TA1CCTL2 = OUTMOD_7;
    TA1CCR2  = 255;
    TA1CTL   = TASSEL__ACLK | MC__UP | TACLR;

    /******************************** SPI **********************************/

    P2DIR  |=   LCD_CS_PIN;                     // DC and CS
    P4DIR  |=   LCD_DC_PIN;
    P1SEL0 |=   LCD_MOSI_PIN | LCD_UCBCLK_PIN;      // MOSI and UCBOCLK
    P1SEL1 &= ~(LCD_MOSI_PIN | LCD_UCBCLK_PIN);

    UCB0CTLW0 |= UCSWRST;       // Reset UCB0
    UCB0CTLW0 |= UCSSEL__SMCLK | UCCKPH | UCMSB | UCMST | UCMODE_0 | UCSYNC;
    UCB0BR0   |= 0x01;         // Clock = SMCLK/1
    UCB0BR1    = 0;
    UCB0CTL1  &= ~UCSWRST;     // Clear UCSWRST to release the eUSCI for operation
    PM5CTL0   &= ~LOCKLPM5;    // Unlock ports from power manager

    TB0CTL = TBSSEL__SMCLK | MC__CONTINUOUS | TBCLR; //SMCLK, /1, continuous
    TB0CCR1 = 100; TB0CCTL1 |= CCIE;

    P3DIR &= ~0x0F; P3REN |= 0x0F; P3OUT &= ~0x0F;
    P1DIR   &= ~(BIT1 | BIT2); P1OUT   |= BIT1 | BIT2; P1REN   |= BIT1 | BIT2;
    P1IES   |= BIT1 | BIT2; P1IFG   &= ~(BIT1 | BIT2); P1IE    |= BIT1 | BIT2;

    __enable_interrupt();

}
void display(void){
    int i, j = 0;

    setColor(COLOR_16_BLACK); i = 13;
    while (i<87) {
        drawLine(20, i , 157, i);
        i++;
    }

    setColor(COLOR_16_WHITE); i = 20;
    while (i < 156) {
        setColor(COLOR_16_CYAN);
        if ((ch1[j] && ch1[j+1] == 0) || (ch1[j] == 0 && ch1[j+1]))
            drawLine(i+1, 16, i+1, 26);
        if ((ch3[j] && ch3[j+1] == 0) || (ch3[j] == 0 && ch3[j+1]))
            drawLine(i+1, 52, i+1, 62);
        setColor(COLOR_16_WHITE);
        if ((ch2[j] && ch2[j+1] == 0) || (ch2[j] == 0 && ch2[j+1]))
            drawLine(i+1, 34, i+1, 44);
        if ((ch4[j] && ch4[j+1] == 0) || (ch4[j] == 0 && ch4[j+1]))
            drawLine(i+1, 70, i+1, 80);

        setColor(COLOR_16_CYAN);
        drawLine(i, 26-(ch1[j]*10), i+1, 26-(ch1[j]*10));
        drawLine(i, 62-(ch3[j]*10), i+1, 62-(ch3[j]*10));
        setColor(COLOR_16_WHITE);
        drawLine(i, 44-(ch2[j]*10), i+1, 44-(ch2[j]*10));
        drawLine(i, 80-(ch4[j]*10), i+1, 80-(ch4[j]*10));
        j++; i+=2;
    }
    setColor(COLOR_16_BLACK);
    int k = 106;
    while (k < 115) {
        drawLine(0, k, 159, k); k++;
    }
    setColor(COLOR_16_WHITE);
    switch(mode) {
        case 0: drawString(2, 106, FONT_SM, "Frequency: 10 kHz"); break;
        case 1: drawString(2, 106, FONT_SM, "Frequency: 5 kHz"); break;
        case 2: drawString(2, 106, FONT_SM, "Frequency: 2 kHz"); break;
        case 3: drawString(2, 106, FONT_SM, "Frequency: 500 Hz"); break;
        case 4: drawString(2, 106, FONT_SM, "Frequency: 100 Hz"); break;
    }
    flag = 0; TB0CCTL1 |= CCIE; index = 0;
}


void main(void) {
    WDTCTL = WDTPW | WDTHOLD;
    initMSP430();
    __delay_cycles(10);
    initLCD();
    introScreen();
    while (TRUE) {
        if (flag){
            display();
        }
        _nop();
    }
}

#pragma vector=TIMER2_A1_VECTOR
__interrupt void TIMER2_A1_ISR(void)
{
  switch(__even_in_range(TA2IV,2))
  {
    case  0: break;
    case  2:
        P9OUT   &= ((testcnt << 1) | 0xE1 );
        P9OUT   |= ((testcnt << 1) & 0x1E );
        testcnt ++;
        testcnt &= 0x0F;
        TA2CCR1 += 1000;
        break;
    default: break;
  }
}

#pragma vector = TIMER0_B1_VECTOR
__interrupt void T0B1_ISR(void) {
    switch(__even_in_range(TB0IV,14)){
        case 0: //no interrupt
            break;
        case 2://CCR1
            if (index > 127){
                TB0CCTL1 &= ~CCIE; flag = 1;
                break;
            }
            TB0CCR1 += freq[mode]; TB0CCTL1 &= ~CCIFG;
            if (P3IN & BIT0){
                ch1[index%128] = 1;
            }
            else{
                ch1[index%128] = 0;
            }

            if (P3IN & BIT1){
                ch2[index%128] = 1;
            }else{
                ch2[index%128] = 0;
            }
            if (P3IN & BIT2){
                ch3[index%128] = 1;
            }else{
                ch3[index%128] = 0;
            }
            if (P3IN & BIT3){
                ch4[index%128] = 1;
            }else{
                ch4[index%128] = 0;
            }
            index++;
        case 4://CCR2
            P1IE |= BIT1 | BIT2;
            if (P1IES & BIT1){
                P1IES &= ~(BIT1 | BIT2);
            }
            else {
                P1IES |= BIT1 | BIT2;
            }
            P1IFG &= ~(BIT1 | BIT2);
            TB0CCTL2 &= ~CCIE;
            break;
        default:
            break;
    }
}

#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void) {
    if((P1IFG & BIT1) && (mode > 0)){
        TB0CCR1 = TB0R + freq[mode]; TB0CCTL1 |= CCIE;
        index = 0; mode--;
    }
    if((P1IFG & BIT2 ) && (mode < 5)){
        TB0CCR1 = TB0R + freq[mode]; TB0CCTL1 |= CCIE;
        index = 0; mode++;
    }
    TB0CCR2 = TB0R + 20000; TB0CCTL2 |= CCIE; P1IFG &= ~(BIT1 | BIT2);
}
