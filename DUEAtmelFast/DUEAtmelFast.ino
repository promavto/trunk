#define __SAM3X8E__

#include <SdFat.h>
#include <SdFatUtil.h>
#include <StdioStream.h>
/*
* We use two ADC channels for this A0 -A14:

/* Tracking Time*/
#define TRACKING_TIME 1
/* Transfer Period */
#define TRANSFER_PERIOD 1 

/** Total number of ADC channels in use */
#define NUM_CHANNELS (15)

/** ADC convention done mask. */
#define ADC_DONE_MASK ( (1<<NUM_CHANNELS) - 1 )

/** Size of the receive buffer and transmit buffer. */
#define BUFFER_SIZE NUM_CHANNELS

/** Reference voltage for ADC, in mv. */
#define VOLT_REF (3300)

/** The maximal digital value */
#define MAX_DIGITAL (4095)

/** ADC channel for potentiometer */
#define ADC_CHANNEL_POTENTIOMETER ADC_CHANNEL_5

#define ADC_CHANNEL_CAM1 ADC_CHANNEL_0
#define ADC_CHANNEL_CAM2 ADC_CHANNEL_1
#define ADC_CHANNEL_CAM3 ADC_CHANNEL_2
#define ADC_CHANNEL_CAM4 ADC_CHANNEL_3
#define ADC_CHANNEL_CAM5 ADC_CHANNEL_4
#define ADC_CHANNEL_CAM6 ADC_CHANNEL_5
#define ADC_CHANNEL_CAM7 ADC_CHANNEL_6
#define ADC_CHANNEL_CAM8 ADC_CHANNEL_7
#define ADC_CHANNEL_CAM9 ADC_CHANNEL_8
#define ADC_CHANNEL_CAM10 ADC_CHANNEL_9
#define ADC_CHANNEL_CAM11 ADC_CHANNEL_10
#define ADC_CHANNEL_CAM12 ADC_CHANNEL_11
#define ADC_CHANNEL_CAM13 ADC_CHANNEL_12
#define ADC_CHANNEL_CAM14 ADC_CHANNEL_13
#define ADC_CHANNEL_CAM15 ADC_CHANNEL_14

/** ADC test mode structure */
struct 
{
uint8_t uc_trigger_mode;
uint8_t uc_pdc_en;
uint8_t uc_sequence_en;
uint8_t uc_gain_en;
uint8_t uc_offset_en;
} g_adc_test_mode;

/** ADC trigger modes */
enum
{
TRIGGER_MODE_SOFTWARE = 0,
TRIGGER_MODE_ADTRG,
TRIGGER_MODE_TIMER,
TRIGGER_MODE_PWM,
TRIGGER_MODE_FREERUN
} e_trigger_mode;

/** ADC sample data */
struct
{
uint8_t uc_ch_num[NUM_CHANNELS];
uint16_t us_value[NUM_CHANNELS];
uint16_t us_done;
} g_adc_sample_data;

/**Channel list for sequence*/
enum adc_channel_num_t ch_list[15] = 
{
ADC_CHANNEL_CAM1,
ADC_CHANNEL_CAM2,
ADC_CHANNEL_CAM3,
ADC_CHANNEL_CAM4,
ADC_CHANNEL_CAM5,
ADC_CHANNEL_CAM6,
ADC_CHANNEL_CAM7,
ADC_CHANNEL_CAM8,
ADC_CHANNEL_CAM9,
ADC_CHANNEL_CAM10,
ADC_CHANNEL_CAM11,
ADC_CHANNEL_CAM12,
ADC_CHANNEL_CAM13,
ADC_CHANNEL_CAM14,
ADC_CHANNEL_CAM15
};

/** Global timestamp in milliseconds since start of application */
static volatile uint32_t gs_ul_ms_ticks = 0;
/**
* \brief Display ADC configuration menu.
*/
 static void display_menu(void)
 {
 uint8_t uc_char;
 
	// puts(MENU_HEADER);
	 uc_char = (g_adc_test_mode.uc_trigger_mode == TRIGGER_MODE_SOFTWARE) ? 'X' : ' ';
	 Serial.print("[%c] 0: Set ADC trigger mode: Software.\n\r");
	 Serial.print(uc_char);

	 uc_char = (g_adc_test_mode.uc_trigger_mode == TRIGGER_MODE_ADTRG) ? 'X' : ' ';
	 Serial.print("[%c] 1: Set ADC trigger mode: ADTRG.\n\r");
	  Serial.print(uc_char);
	 uc_char = (g_adc_test_mode.uc_trigger_mode == TRIGGER_MODE_TIMER) ? 'X' : ' ';
	 Serial.print("[%c] 2: Set ADC trigger mode: Timer TIOA.\n\r");
	  Serial.print( uc_char);
	/* #if SAM3S || SAM3U || SAM3XA || SAM4S*/
	 uc_char = (g_adc_test_mode.uc_trigger_mode == TRIGGER_MODE_PWM) ? 'X' : ' ';
	 Serial.print("[%c] 3: Set ADC trigger mode: PWM Event Line.\n\r");
	 Serial.print( uc_char);
	/* #endif
	 #if SAM3S || SAM3N || SAM3XA || SAM4S*/
	 uc_char = (g_adc_test_mode.uc_trigger_mode == TRIGGER_MODE_FREERUN) ? 'X' : ' ';
	Serial.print("[%c] 4: Set ADC trigger mode: Free run mode.\n\r");
	Serial.print(uc_char);
	/* #endif*/
	 uc_char = (g_adc_test_mode.uc_pdc_en) ? 'E' : 'D';
	 Serial.print("[%c] T: Enable/Disable to transfer with PDC.\n\r");
	 Serial.print(uc_char);
	 //#if SAM3S || SAM3N || SAM3XA || SAM4S
	 uc_char = (g_adc_test_mode.uc_sequence_en) ? 'E' : 'D';
	Serial.print("[%c] S: Enable/Disable to use user sequence mode.\n\r");
	Serial.print(uc_char);
	/*	 #endif
		 #if SAM3S8 || SAM3SD8 || SAM4S || SAM3N || SAM3U*/
	//++ uc_char = (g_adc_test_mode.uc_power_save_en) ? 'E' : 'D';
	Serial.print("[%c] P: Enable/Disable ADC power save mode.\n\r");
	Serial.print(uc_char);
	/*	 #endif 
		 #if SAM3S || SAM3U || SAM3XA || SAM4S*/
	 uc_char = (g_adc_test_mode.uc_gain_en) ? 'E' : 'D';
	Serial.print("[%c] G: Enable/Disable to set gain=2 for potentiometer channel.\n\r");
	Serial.print(uc_char);

	 uc_char = (g_adc_test_mode.uc_offset_en) ? 'E' : 'D';
	Serial.print("[%c] O: Enable/Disable offset for potentiometer channel.\n\r");
	 Serial.print(uc_char);
	/*	 #endif
		 #if SAM3S8 || SAM3SD8 || SAM4S*/
//++ uc_char = (g_adc_test_mode.uc_auto_calib_en) ? 'E' : 'D';
	Serial.print("[%c] C: Enable Auto Calibration Mode.\n\r");
	Serial.print(uc_char);

	/* #endif*/
	 Serial.print(" Q: Quit configuration and start ADC.\r");
	 Serial.print("============================================ =============\r");
 }

/*
* \brief Set ADC test mode.
*/
 static void set_adc_test_mode(void)
 {
 uint8_t uc_key;
 uint8_t uc_key1;
 uint8_t uc_done = 0;
 
 while (!uc_done) 
 {
   while (uc_key = Serial.read())
	/*   uc_key1 = uc_key;
	  if(uc_key1 != 255) 
		  {
			  Serial.println(uc_key1);
		  }*/
 
   switch (uc_key)
		 {
		 case '0':
			 Serial.println("TRIGGER_MODE_SOFTWARE");
			 g_adc_test_mode.uc_trigger_mode = TRIGGER_MODE_SOFTWARE;
			 break;
 
		 case '1':
			  Serial.println("TRIGGER_MODE_ADTRG");
			 g_adc_test_mode.uc_trigger_mode = TRIGGER_MODE_ADTRG;
			 break;
 
		 case '2':
			  Serial.println("TRIGGER_MODE_TIMER");
			 g_adc_test_mode.uc_trigger_mode = TRIGGER_MODE_TIMER;
			 break;
		 case '3':
			  Serial.println("TRIGGER_MODE_PWM");
			 g_adc_test_mode.uc_trigger_mode = TRIGGER_MODE_PWM;
			 break;
		 case '4':
		 g_adc_test_mode.uc_trigger_mode = TRIGGER_MODE_FREERUN;
		 break;
		 case 't':
		 case 'T':
			 Serial.println("T");
			 if (g_adc_test_mode.uc_pdc_en) 
				 {
					 Serial.println("g_adc_test_mode.uc_pdc_en = 0");
					 g_adc_test_mode.uc_pdc_en = 0;
				 }
			 else 
				 {
					 Serial.println("g_adc_test_mode.uc_pdc_en = 1");
					 g_adc_test_mode.uc_pdc_en = 1;
				 }
		  break;

		 case 's':
		 case 'S':
			 Serial.println("S");
			 if (g_adc_test_mode.uc_sequence_en) 
				 {
				 g_adc_test_mode.uc_sequence_en = 0;
				 }
			 else 
				 {
				 g_adc_test_mode.uc_sequence_en = 1;
				 }
			 break;
	//		 #endif
			 #if SAM3S8 || SAM3SD8 || SAM4S || SAM3N || SAM3U
		 case 'p':
		 case 'P':
			 Serial.println("P");
			 if (g_adc_test_mode.uc_power_save_en) 
			 {
			 g_adc_test_mode.uc_power_save_en = 0;
			 }
			 else
			 {
			 g_adc_test_mode.uc_power_save_en = 1;
			 }
			 break;
			 #endif
			 #if SAM3S || SAM3U || SAM3XA || SAM4S
		 case 'g':
		 case 'G':
			 Serial.println("G");
			 if (g_adc_test_mode.uc_gain_en) 
			 {
			 g_adc_test_mode.uc_gain_en = 0;
			 }
			 else 
			 {
			 g_adc_test_mode.uc_gain_en = 1;
			 }
			 break;
 
		 case 'o':
		 case 'O':
			 Serial.println("O");
			 if (g_adc_test_mode.uc_offset_en)
			 {
			 g_adc_test_mode.uc_offset_en = 0;
			 } else {
			 g_adc_test_mode.uc_offset_en = 1;
			 }
			 break;
			 #endif
			 #if SAM3S8 || SAM3SD8 || SAM4S
		 case 'c':
		 case 'C':
			 Serial.println("C");
			 if (g_adc_test_mode.uc_auto_calib_en)
			 {
			 g_adc_test_mode.uc_auto_calib_en = 0;
			 } else {
			 g_adc_test_mode.uc_auto_calib_en = 1;
			 }
			 break;
			 #endif
		 case 'q':
		 case 'Q':
			 Serial.println("Q");
			 uc_done = 1;
			 break;
 
			 default:
			 break;
		 }
 
	// display_menu();
   }
 }


/*
* \brief Read converted data through PDC channel.
*
* \param p_adc The pointer of adc peripheral.
* \param p_s_buffer The destination buffer.
* \param ul_size The size of the buffer.
*/
static uint32_t adc_read_buffer(Adc * p_adc, uint16_t * p_s_buffer, uint32_t ul_size)
{
	/* Check if the first PDC bank is free. */
	if ((p_adc->ADC_RCR == 0) && (p_adc->ADC_RNCR == 0))
		{
			p_adc->ADC_RPR = (uint32_t) p_s_buffer;
			p_adc->ADC_RCR = ul_size;
			p_adc->ADC_PTCR = ADC_PTCR_RXTEN;
			return 1;
		}
	else 
		{ /* Check if the second PDC bank is free. */
			if (p_adc->ADC_RNCR == 0) 
				{
					p_adc->ADC_RNPR = (uint32_t) p_s_buffer;
					p_adc->ADC_RNCR = ul_size;

					return 1;
				} 
			else 
				{
				return 0;
				}
		}
}

/**
* \brief Start ADC sample.
* Initialize ADC, set clock and timing, and set ADC to given mode.
*/
static void start_adc(void)
	{
	/* Enable peripheral clock. */
	uint32_t i;
	pmc_enable_periph_clk(ID_ADC);

	/* Initialize ADC. */
	/*
	* Formula: ADCClock = MCK / ( (PRESCAL+1) * 2 )
	* For example, MCK = 64MHZ, PRESCAL = 4, then:
	* ADCClock = 64 / ((4+1) * 2) = 6.4MHz;
	*/
	/* Formula:
	* Startup Time = startup value / ADCClock
	* Startup time = 64 / 6.4MHz = 10 us
	*/


	//+++++++++++++++++++++++++++
	//++!!adc_init(ADC, sysclk_get_cpu_hz(), 6400000, ADC_STARTUP_TIME_4);
	adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST);

	memset((void *)&g_adc_sample_data, 0, sizeof(g_adc_sample_data));

	/* Set ADC timing. */

	/* Formula:
	* Transfer Time = (TRANSFER * 2 + 3) / ADCClock
	* Tracking Time = (TRACKTIM + 1) / ADCClock
	* Settling Time = settling value / ADCClock
	*
	* Transfer Time = (1 * 2 + 3) / 6.4MHz = 781 ns
	* Tracking Time = (1 + 1) / 6.4MHz = 312 ns
	* Settling Time = 3 / 6.4MHz = 469 ns
	*/
	adc_configure_timing(ADC, TRACKING_TIME, ADC_SETTLING_TIME_3, TRANSFER_PERIOD);


	/* Enable channel number tag. */
	adc_enable_tag(ADC);
	/* Enable/disable sequencer. */
	if (g_adc_test_mode.uc_sequence_en) 
		{
			/* Set user defined channel sequence. */
			adc_configure_sequence(ADC, ch_list, 15);

			/* Enable sequencer. */
			adc_start_sequencer(ADC);

			/* Enable channels. */
			for (i = 0; i < 15; i++) 
				{
					adc_enable_channel(ADC, (enum adc_channel_num_t)i);
				}
			/* Update channel number. */
			g_adc_sample_data.uc_ch_num[0] = ch_list[0];
			g_adc_sample_data.uc_ch_num[1] = ch_list[1];
			g_adc_sample_data.uc_ch_num[2] = ch_list[2];
			g_adc_sample_data.uc_ch_num[3] = ch_list[3];
			g_adc_sample_data.uc_ch_num[4] = ch_list[4];
			g_adc_sample_data.uc_ch_num[5] = ch_list[5];
			g_adc_sample_data.uc_ch_num[6] = ch_list[6];
			g_adc_sample_data.uc_ch_num[7] = ch_list[7];
			g_adc_sample_data.uc_ch_num[8] = ch_list[8];
			g_adc_sample_data.uc_ch_num[9] = ch_list[9];
			g_adc_sample_data.uc_ch_num[10] = ch_list[10];
			g_adc_sample_data.uc_ch_num[11] = ch_list[11];
			g_adc_sample_data.uc_ch_num[12] = ch_list[12];
			g_adc_sample_data.uc_ch_num[13] = ch_list[13];
			g_adc_sample_data.uc_ch_num[14] = ch_list[14];
			g_adc_sample_data.uc_ch_num[15] = ch_list[15];
		} 
	else 
		{
		/* Disable sequencer. */
		adc_stop_sequencer(ADC);

		/* Enable channels. */
		adc_enable_channel(ADC, ADC_CHANNEL_CAM1);
		adc_enable_channel(ADC, ADC_TEMPERATURE_SENSOR);
		/* Update channel number. */
		g_adc_sample_data.uc_ch_num[0] = ADC_CHANNEL_CAM1;
		// #if SAM3S || SAM3XA || SAM4S
		// g_adc_sample_data.uc_ch_num[1] = ADC_TEMPERATURE_SENSOR;
		// #else
		 g_adc_sample_data.uc_ch_num[1] = ADC_CHANNEL_POTENTIOMETER;
		// #endif
		}


	// /* Enable the temperature sensor (CH15). */
	 adc_enable_ts(ADC);

	/* Set gain and offset (only single ended mode used here). */
	//#if SAM3S || SAM3XA || SAM4S
	adc_disable_anch(ADC); /* Disable analog change. Отключить аналоговый изменения.*/

	if (g_adc_test_mode.uc_gain_en) 
		{
			adc_enable_anch(ADC);
			/* gain = 2 */
			adc_set_channel_input_gain(ADC, ADC_CHANNEL_CAM1, ADC_GAINVALUE_2);
		} 
	else 
		{
			/* gain = 1 */
			adc_set_channel_input_gain(ADC, ADC_CHANNEL_CAM1, ADC_GAINVALUE_0);
		}

	if (g_adc_test_mode.uc_offset_en) 
		{
			adc_enable_anch(ADC);
			adc_enable_channel_input_offset(ADC, ADC_CHANNEL_CAM1);
			//#elif SAM3U
			//#ifdef ADC_12B
			//adc12b_enable_input_offset(ADC12B);
			//#endif
		}
	else 
		{
			adc_disable_channel_input_offset(ADC, ADC_CHANNEL_CAM1);
		}
	/* Transfer with/without PDC. */
	if (g_adc_test_mode.uc_pdc_en) 
		{
			adc_read_buffer(ADC, g_adc_sample_data.us_value, BUFFER_SIZE);
			/* Enable PDC channel interrupt. */
			adc_enable_interrupt(ADC, ADC_IER_RXBUFF);
		}
	else 
		{
		/* Enable Data ready interrupt. */
			adc_enable_interrupt(ADC, ADC_IER_DRDY);
		}
	/* Enable ADC interrupt. */
	NVIC_EnableIRQ(ADC_IRQn);

	/* Configure trigger mode and start convention. */
	switch (g_adc_test_mode.uc_trigger_mode) 
		{
		case TRIGGER_MODE_SOFTWARE:
			adc_configure_trigger(ADC, ADC_TRIG_SW, 0); /* Disable hardware trigger. */
		break;

		case TRIGGER_MODE_ADTRG:
		  //pio_configure_pin(PINS_ADC_TRIG, PINS_ADC_TRIG_FLAG);
		   adc_configure_trigger(ADC, ADC_TRIG_EXT, 0);
		break;

		case TRIGGER_MODE_FREERUN:
		adc_configure_trigger(ADC, ADC_TRIG_SW, 1);
		break;

		default:
		break;
		}
}

/**
* \brief Systick handler.
*/
//void SysTick_Handler ( void )
//{
//gs_ul_ms_ticks++;
//}


//#if SAM3S || SAM3N || SAM3XA || SAM4S
/**
* \brief Interrupt handler for the ADC.
*/
void ADC_Handler(void)
{
	uint32_t i;
	uint32_t ul_temp;
	uint8_t uc_ch_num;

	/* With PDC transfer */
	if (g_adc_test_mode.uc_pdc_en) 
		{
			if ((adc_get_status(ADC) & ADC_ISR_RXBUFF) == ADC_ISR_RXBUFF) 
				{
					g_adc_sample_data.us_done = ADC_DONE_MASK;
					adc_read_buffer(ADC, g_adc_sample_data.us_value, BUFFER_SIZE);
					/* Only keep sample value, and discard channel number. */
						for (i = 0; i < NUM_CHANNELS; i++) 
						{
							g_adc_sample_data.us_value[i] &= ADC_LCDR_LDATA_Msk;
						}
				}
		}
	else
		{ /* Without PDC transfer */
		if ((adc_get_status(ADC) & ADC_ISR_DRDY) ==
		ADC_ISR_DRDY) 
			{
				ul_temp = adc_get_latest_value(ADC);
				for (i = 0; i < NUM_CHANNELS; i++) 
					{
						uc_ch_num = (ul_temp & ADC_LCDR_CHNB_Msk) >>
						ADC_LCDR_CHNB_Pos;
						if (g_adc_sample_data.uc_ch_num[i] == uc_ch_num) 
							{
								g_adc_sample_data.us_value[i] =	ul_temp & ADC_LCDR_LDATA_Msk;
								g_adc_sample_data.us_done |= 1 << i;
							}
					}
			}
		}
}

/**
* Wait for the given number of milliseconds (using the dwTimeStamp generated
* by the SAM microcontrollers' system tick).
* \param ul_dly_ticks Delay to wait for, in milliseconds.
*/
static void mdelay(uint32_t ul_dly_ticks)
	{
		uint32_t ul_cur_ticks;

		ul_cur_ticks = gs_ul_ms_ticks;
		while ((gs_ul_ms_ticks - ul_cur_ticks) < ul_dly_ticks);
	}


void setup()
{
	 Serial.begin(115200);


  // put your setup code here, to run once:

}

void loop() 
{
	 uint32_t i;
	 uint8_t uc_key;
	/* Initialize the SAM system. */
//	sysclk_init();
	//!!board_init();

	WDT->WDT_MR = WDT_MR_WDDIS;   //  Отключить сторожевой таймер

	//configure_console();

	// Serial.println("Configure system tick to get 1ms tick period.\r");

	/* Set default ADC test mode. */
	memset((void *)&g_adc_test_mode, 0, sizeof(g_adc_test_mode));
	g_adc_test_mode.uc_trigger_mode = TRIGGER_MODE_FREERUN;
	g_adc_test_mode.uc_pdc_en = 1;
	g_adc_test_mode.uc_sequence_en = 1;
	g_adc_test_mode.uc_gain_en = 0;
	g_adc_test_mode.uc_offset_en = 0;

//	display_menu();
	start_adc();

	Serial.println("Press any key to display configuration menu.\r");
	while (1) 
	{
	/* ADC software trigger per 1s */
	 //if (g_adc_test_mode.uc_trigger_mode == TRIGGER_MODE_SOFTWARE) 
	 //{
		// delay(1000);
	 //	 adc_start(ADC);
	 //}

	 /* Check if the user enters a key. */
	 //if (uc_key = Serial.read()) 
		//{
		////	#if SAM3S || SAM3N || SAM3XA || SAM4S
		//	adc_disable_interrupt(ADC, 0xFFFFFFFF); /* Disable all adc interrupt. */
		////	#elif SAM3U
		////		#ifdef ADC_12B
		////		adc12b_disable_interrupt(ADC12B, 0xFFFFFFFF); /* Disable all adc interrupt. */
		////		#else
		////		adc_disable_interrupt(ADC, 0xFFFFFFFF); /* Disable all adc interrupt. */
		////		#endif
		////	#endif
		//}

	//display_menu();
	//set_adc_test_mode();
	start_adc();
	//puts("Press any key to display configuration menu.\r");


	/* Check if ADC sample is done. */
	if (g_adc_sample_data.us_done == ADC_DONE_MASK) 
		{
			for (i = 0; i < NUM_CHANNELS; i++) 
				{

					Serial.print("CH%02d: %04d mv. ");
					Serial.print((int)g_adc_sample_data.uc_ch_num[i]);
					Serial.print((int)(g_adc_sample_data.us_value[i]*VOLT_REF/MAX_DIGITAL));
		
				}
		Serial.print("\r");
		g_adc_sample_data.us_done = 0;
		}
	}
}