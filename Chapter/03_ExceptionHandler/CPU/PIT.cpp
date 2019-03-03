#include "PIT.h"
#include "Hal.h"
#include "SkyConsole.h"

#define		I86_PIT_REG_COUNTER0		0x40
#define		I86_PIT_REG_COUNTER1		0x41
#define		I86_PIT_REG_COUNTER2		0x42
#define		I86_PIT_REG_COMMAND			0x43

volatile uint32_t _pitTicks = 0;
DWORD _lastTickCount = 0;

//타이머 인터럽트 핸들러
__declspec(naked) void InterruptPITHandler() 
{	
	_asm {
		cli
		pushad
	}

	_pitTicks++;
	
	_asm {
		mov al, 0x20 
		out 0x20, al
		popad
		sti
		iretd
	}
	//cli: clear interrupt(stop interrupt)
	//pushad: Stores all register values in the stack
	//mov: copy right to left. mov A, B -> B to A move data
	//out: port output
	//popad: pop all general purpose registers
	//sti: Set interrupt(Start interrupt)
	//iretd: return interrupt to task
	//AL: the lower 16 bits of EAX register (EAX : General purpose register)
	//https://www.tutorialspoint.com/assembly_programming/assembly_registers.htm
}

//타이머를 시작
void StartPITCounter(uint32_t freq, uint8_t counter, uint8_t mode) {

	if (freq == 0)
		return;

	uint16_t divisor = uint16_t(1193181 / (uint16_t)freq);

	//커맨드 전송
	uint8_t ocw = 0;
	ocw = (ocw & ~I86_PIT_OCW_MASK_MODE) | mode;
	ocw = (ocw & ~I86_PIT_OCW_MASK_RL) | I86_PIT_OCW_RL_DATA;
	ocw = (ocw & ~I86_PIT_OCW_MASK_COUNTER) | counter;
	SendPITCommand(ocw);

	//프리퀀시 비율 설정
	SendPITData(divisor & 0xff, 0);
	SendPITData((divisor >> 8) & 0xff, 0);

	//타이머 틱 카운트 리셋
	_pitTicks = 0;
}

//PIT 초기화
void InitializePIT()
{
	setvect(32, InterruptPITHandler);
}

uint32_t SetPITTickCount(uint32_t i) {

	uint32_t ret = _pitTicks;
	_pitTicks = i;
	return ret;
}


//! returns current tick count
uint32_t GetPITTickCount() {

	return _pitTicks;
}

unsigned int GetTickCount()
{
	return _pitTicks;
}

void _cdecl msleep(int ms)
{

	unsigned int ticks = ms + GetTickCount();
	while (ticks > GetTickCount())
		;
}

void SendPITCommand(uint8_t cmd) {

	OutPortByte(I86_PIT_REG_COMMAND, cmd);
}

void SendPITData(uint16_t data, uint8_t counter) {

	uint8_t	port = (counter == I86_PIT_OCW_COUNTER_0) ? I86_PIT_REG_COUNTER0 :
		((counter == I86_PIT_OCW_COUNTER_1) ? I86_PIT_REG_COUNTER1 : I86_PIT_REG_COUNTER2);

	OutPortByte(port, (uint8_t)data);
}