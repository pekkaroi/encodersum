#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/systick.h>


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


volatile uint16_t in1_oldcount;
volatile uint16_t in2_oldcount;
volatile int32_t in_pos;
volatile int32_t out_pos;

volatile uint8_t dir1;
volatile uint8_t dir2;
volatile uint8_t dir_out;
static uint8_t quadrature_seq[] = {0, 1, 3, 2};

void gen_pulses(void);
void get_pos(void);


/* Set STM32 to 72 MHz. */
static void clock_setup(void)
{
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	/* Enable GPIOC clock. */
	rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);

}


static void timer_setup(void)
{

    //Setup TIM2 (PA0, PA1) to read encoder.
    rcc_periph_clock_enable(RCC_TIM2);
    timer_set_period(TIM2, 65535);
    timer_slave_set_mode(TIM2, 0x3); // encoder
    timer_ic_set_input(TIM2, TIM_IC1, TIM_IC_IN_TI1);
    timer_ic_set_input(TIM2, TIM_IC2, TIM_IC_IN_TI2);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
    		      GPIO_CNF_INPUT_PULL_UPDOWN, GPIO0|GPIO1);
    gpio_set(GPIOA, GPIO0|GPIO1);
    
    timer_enable_counter(TIM2);

    //Setup TIM3 (PA6, PA7) to read encoder.
    rcc_periph_clock_enable(RCC_TIM3);
    timer_set_period(TIM3, 65535);
    timer_slave_set_mode(TIM3, 0x3); // encoder
    timer_ic_set_input(TIM3, TIM_IC1, TIM_IC_IN_TI1);
    timer_ic_set_input(TIM3, TIM_IC2, TIM_IC_IN_TI2);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
    		      GPIO_CNF_INPUT_PULL_UPDOWN, GPIO6|GPIO7);
    gpio_set(GPIOA, GPIO6|GPIO7);
    

    timer_enable_counter(TIM3);



    //Setup TIM4 for loop timer at around 12kHz
    rcc_periph_clock_enable(RCC_TIM4);
    rcc_periph_reset_pulse(RST_TIM4);
    timer_set_mode(TIM4, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_period(TIM4, (36*40)-1);
	timer_disable_preload(TIM4);
	timer_continuous_mode(TIM4);
    nvic_set_priority(NVIC_TIM4_IRQ, 4);
    nvic_enable_irq(NVIC_TIM4_IRQ);
    timer_enable_irq(TIM4, TIM_DIER_UIE);
    timer_enable_counter(TIM4);

    //initialize outputs
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_10_MHZ,
    		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO8|GPIO9);
    gpio_clear(GPIOA, GPIO8|GPIO9);

}


void get_pos(void)
{
	uint16_t now = timer_get_counter(TIM2);
	int16_t delta =  (int16_t)(now - in1_oldcount);
    in1_oldcount = now;
    in_pos += dir1*delta;
    
    now = timer_get_counter(TIM3);
	delta =  (int16_t)(now - in2_oldcount);
    in2_oldcount = now;
    in_pos += dir2*delta;

}

void gen_pulses(void)
{
    uint8_t seq;
    if(in_pos > out_pos)
    {
        out_pos++;

    }
    if(in_pos < out_pos)
    {
        out_pos--;

    }
    
    seq = quadrature_seq[(out_pos & 3)];
    if(seq&2)
        gpio_set(GPIOA, GPIO8);
    else
        gpio_clear(GPIOA, GPIO8);

    if(seq&1)
        gpio_set(GPIOA, GPIO9);
    else
        gpio_clear(GPIOA, GPIO9);
}

void tim4_isr(void)
{
    if(timer_get_flag(TIM4, TIM_SR_UIF))
    {
        timer_clear_flag(TIM4, TIM_SR_UIF);
        get_pos();
        gen_pulses();
    }
}


int main(void)
{
    in1_oldcount = 0;
    in2_oldcount = 0;
    in_pos = 0;
    out_pos = 0;
    dir1=1;
    dir2=1;

    clock_setup();
    timer_setup();

    while(1){
        for (int i = 0; i < 100; i++) {
			__asm__("nop");
		}
    }
}
