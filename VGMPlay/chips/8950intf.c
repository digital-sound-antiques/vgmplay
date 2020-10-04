/******************************************************************************
* FILE
*   Yamaha 3812 emulator interface - MAME VERSION
*
* CREATED BY
*   Ernesto Corvi
*
* UPDATE LOG
*   JB  28-04-2002  Fixed simultaneous usage of all three different chip types.
*                       Used real sample rate when resample filter is active.
*       AAT 12-28-2001  Protected Y8950 from accessing unmapped port and keyboard handlers.
*   CHS 1999-01-09  Fixes new ym3812 emulation interface.
*   CHS 1998-10-23  Mame streaming sound chip update
*   EC  1998        Created Interface
*
* NOTES
*
******************************************************************************/
#include <stddef.h>	// for NULL
#include "mamedef.h"
//#include "attotime.h"
//#include "sndintrf.h"
//#include "streams.h"
//#include "cpuintrf.h"
#include "8950intf.h"
//#include "fm.h"
#ifdef ENABLE_ALL_CORES
#include "fmopl.h"
#endif
#include "emu8950.h"

#ifdef ENABLE_ALL_CORES
#define EC_EMU8950	0x01
#endif
#define EC_MAME		0x00	// YM3826 core from MAME

typedef struct _y8950_state y8950_state;
struct _y8950_state
{
	//sound_stream *	stream;
	//emu_timer *		timer[2];
	void *			chip;
	//const y8950_interface *intf;
	//const device_config *device;
};


extern UINT8 CHIP_SAMPLING_MODE;
extern INT32 CHIP_SAMPLE_RATE;
static UINT8 EMU_CORE = 0x00;

#define MAX_CHIPS	0x02
static y8950_state Y8950Data[MAX_CHIPS];

/*INLINE y8950_state *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == SOUND);
	assert(sound_get_type(device) == SOUND_Y8950);
	return (y8950_state *)device->token;
}*/


static void IRQHandler(void *param,int irq)
{
	y8950_state *info = (y8950_state *)param;
	//if (info->intf->handler) (info->intf->handler)(info->device, irq ? ASSERT_LINE : CLEAR_LINE);
	//if (info->intf->handler) (info->intf->handler)(irq ? ASSERT_LINE : CLEAR_LINE);
}
/*static TIMER_CALLBACK( timer_callback_0 )
{
	y8950_state *info = (y8950_state *)ptr;
	y8950_timer_over(info->chip,0);
}
static TIMER_CALLBACK( timer_callback_1 )
{
	y8950_state *info = (y8950_state *)ptr;
	y8950_timer_over(info->chip,1);
}*/
//static void TimerHandler(void *param,int c,attotime period)
static void TimerHandler(void *param,int c,int period)
{
	y8950_state *info = (y8950_state *)param;
	//if( attotime_compare(period, attotime_zero) == 0 )
	if( period == 0 )
	{	/* Reset FM Timer */
		//timer_enable(info->timer[c], 0);
	}
	else
	{	/* Start FM Timer */
		//timer_adjust_oneshot(info->timer[c], period, 0);
	}
}


static unsigned char Y8950PortHandler_r(void *param)
{
	y8950_state *info = (y8950_state *)param;
	/*if (info->intf->portread)
		return info->intf->portread(0);*/
	return 0;
}

static void Y8950PortHandler_w(void *param,unsigned char data)
{
	y8950_state *info = (y8950_state *)param;
	/*if (info->intf->portwrite)
		info->intf->portwrite(0,data);*/
}

static unsigned char Y8950KeyboardHandler_r(void *param)
{
	y8950_state *info = (y8950_state *)param;
	/*if (info->intf->keyboardread)
		return info->intf->keyboardread(0);*/
	return 0;
}

static void Y8950KeyboardHandler_w(void *param,unsigned char data)
{
	y8950_state *info = (y8950_state *)param;
	/*if (info->intf->keyboardwrite)
		info->intf->keyboardwrite(0,data);*/
}

static void _emu8950_calc_stereo(OPL *opl, INT32 **out, int samples)
{
	INT32 *bufL = out[0];
	INT32 *bufR = out[1];
	INT32 buffers[2];
	int i;

	for (i = 0; i < samples; i++)
	{
		OPL_calcStereo(opl, buffers);
		bufL[i] = buffers[0] << 1;
		bufR[i] = buffers[1] << 1;
	}
}

//static STREAM_UPDATE( y8950_stream_update )
void y8950_stream_update(UINT8 ChipID, stream_sample_t **outputs, int samples)
{
	//y8950_state *info = (y8950_state *)param;
	y8950_state *info = &Y8950Data[ChipID];

	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_EMU8950:
		_emu8950_calc_stereo(info->chip, outputs, samples);
		break;
#endif
	case EC_MAME:
		y8950_update_one(info->chip, outputs, samples);
		break;
	}
}

static void _stream_update(void *param/*, int interval*/)
{
	y8950_state *info = (y8950_state *)param;
	//stream_update(info->stream);
	
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_EMU8950:
		_emu8950_calc_stereo(info->chip, DUMMYBUF, 0);
		break;
#endif
	case EC_MAME:
		y8950_update_one(info->chip, DUMMYBUF, 0);
		break;
	}
}

//static DEVICE_START( y8950 )
int device_start_y8950(UINT8 ChipID, int clock)
{
	//static const y8950_interface dummy = { 0 };
	//y8950_state *info = get_safe_token(device);
	y8950_state *info;
	int rate;
	
	if (ChipID >= MAX_CHIPS)
		return 0;
	
	info = &Y8950Data[ChipID];
	rate = clock/72;
	if ((CHIP_SAMPLING_MODE == 0x01 && rate < CHIP_SAMPLE_RATE) ||
		CHIP_SAMPLING_MODE == 0x02)
		rate = CHIP_SAMPLE_RATE;
	//info->intf = device->static_config ? (const y8950_interface *)device->static_config : &dummy;
	//info->intf = &dummy;
	//info->device = device;

	/* stream system initialize */
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_EMU8950:
		info->chip = OPL_new(clock, rate);
		OPL_setChipType(info->chip, 0);
		break;
#endif
	case EC_MAME:
		info->chip = y8950_init(clock, rate);
		//assert_always(info->chip != NULL, "Error creating Y8950 chip");

		/* ADPCM ROM data */
		//y8950_set_delta_t_memory(info->chip, device->region, device->regionbytes);
		y8950_set_delta_t_memory(info->chip, NULL, 0x00);

		//info->stream = stream_create(device,0,1,rate,info,y8950_stream_update);

		/* port and keyboard handler */
		y8950_set_port_handler(info->chip, Y8950PortHandler_w, Y8950PortHandler_r, info);
		y8950_set_keyboard_handler(info->chip, Y8950KeyboardHandler_w, Y8950KeyboardHandler_r, info);

		/* Y8950 setup */
		y8950_set_timer_handler(info->chip, TimerHandler, info);
		y8950_set_irq_handler(info->chip, IRQHandler, info);
		y8950_set_update_handler(info->chip, _stream_update, info);

		//info->timer[0] = timer_alloc(device->machine, timer_callback_0, info);
		//info->timer[1] = timer_alloc(device->machine, timer_callback_1, info);
		break;
	}
	
	return rate;
}

//static DEVICE_STOP( y8950 )
void device_stop_y8950(UINT8 ChipID)
{
	//y8950_state *info = get_safe_token(device);
	y8950_state *info = &Y8950Data[ChipID];
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_EMU8950:
		OPL_delete(info->chip);
		break;
#endif
	case EC_MAME:
		y8950_shutdown(info->chip);
		break;
	}
}

//static DEVICE_RESET( y8950 )
void device_reset_y8950(UINT8 ChipID)
{
	//y8950_state *info = get_safe_token(device);
	y8950_state *info = &Y8950Data[ChipID];
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_EMU8950:
		OPL_reset(info->chip);
		break;
#endif
	case EC_MAME:
		y8950_reset_chip(info->chip);
		break;
	}
}


//READ8_DEVICE_HANDLER( y8950_r )
UINT8 y8950_r(UINT8 ChipID, offs_t offset)
{
	//y8950_state *info = get_safe_token(device);
	y8950_state* info = &Y8950Data[ChipID];
	switch (EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_EMU8950:
		OPL_writeIO(info->chip, 0, offset);
		return OPL_readIO(info->chip);
#endif
	case EC_MAME:
		return y8950_read(info->chip, offset & 1);
	default:
		return 0x00;
	}
}

//WRITE8_DEVICE_HANDLER( y8950_w )
void y8950_w(UINT8 ChipID, offs_t offset, UINT8 data)
{
	//y8950_state *info = get_safe_token(device);
	y8950_state *info = &Y8950Data[ChipID];
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_EMU8950:
		OPL_writeIO(info->chip, offset & 1, data);
		break;
#endif
	case EC_MAME:
		y8950_write(info->chip, offset & 1, data);
		break;
	}
}

//READ8_DEVICE_HANDLER( y8950_status_port_r )
UINT8 y8950_status_port_r(UINT8 ChipID, offs_t offset)
{
	return y8950_r(ChipID, 0);
}
//READ8_DEVICE_HANDLER( y8950_read_port_r )
UINT8 y8950_read_port_r(UINT8 ChipID, offs_t offset)
{
	return y8950_r(ChipID, 1);
}
//WRITE8_DEVICE_HANDLER( y8950_control_port_w )
void y8950_control_port_w(UINT8 ChipID, offs_t offset, UINT8 data)
{
	y8950_w(ChipID, 0, data);
}
//WRITE8_DEVICE_HANDLER( y8950_write_port_w )
void y8950_write_port_w(UINT8 ChipID, offs_t offset, UINT8 data)
{
	y8950_w(ChipID, 1, data);
}


void y8950_write_data_pcmrom(UINT8 ChipID, offs_t ROMSize, offs_t DataStart,
							  offs_t DataLength, const UINT8* ROMData)
{
	y8950_state* info = &Y8950Data[ChipID];
	
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_EMU8950:
		OPL_writeADPCMData(info->chip, 1, DataStart, DataLength, ROMData);
		break;
#endif
	case EC_MAME:
		y8950_write_pcmrom(info->chip, ROMSize, DataStart, DataLength, ROMData);
		break;
	}
	return;
}

static void _emu8950_set_mute_mask(OPL *opl, UINT32 MuteMask)
{
	unsigned char CurChn;
	UINT32 ChnMsk;

	for (CurChn = 0; CurChn < 14; CurChn++)
	{
		if (CurChn < 9)
		{
			ChnMsk = OPL_MASK_CH(CurChn);
		}
		else
		{
			switch (CurChn)
			{
			case 9:
				ChnMsk = OPL_MASK_BD;
				break;
			case 10:
				ChnMsk = OPL_MASK_SD;
				break;
			case 11:
				ChnMsk = OPL_MASK_TOM;
				break;
			case 12:
				ChnMsk = OPL_MASK_CYM;
				break;
			case 13:
				ChnMsk = OPL_MASK_HH;
				break;
			default:
				ChnMsk = 0;
				break;
			}
		}
		if ((MuteMask >> CurChn) & 0x01)
			opl->mask |= ChnMsk;
		else
			opl->mask &= ~ChnMsk;
	}
}

void y8950_set_emu_core(UINT8 Emulator)
{
#ifdef ENABLE_ALL_CORES
	EMU_CORE = (Emulator < 0x02) ? Emulator : 0x00;
#else
	EMU_CORE = EC_MAME;
#endif

	return;
}

void y8950_set_mute_mask(UINT8 ChipID, UINT32 MuteMask)
{
	y8950_state *info = &Y8950Data[ChipID];
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_EMU8950:
		_emu8950_set_mute_mask(info->chip, MuteMask);
		break;
#endif
	case EC_MAME:
		opl_set_mute_mask(info->chip, MuteMask);
		break;
	}
}


/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( y8950 )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(y8950_state);				break;

		// --- the following bits of info are returned as pointers to data or functions ---
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( y8950 );				break;
		case DEVINFO_FCT_STOP:							info->stop = DEVICE_STOP_NAME( y8950 );				break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME( y8950 );				break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "Y8950");							break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Yamaha FM");						break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}*/
