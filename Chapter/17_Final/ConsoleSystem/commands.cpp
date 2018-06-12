#include "SkyConsole.h"
#include "SkyOS.h"
#include "ProcessManager.h"
#include "ZetPlane.h"
#include "RMEFunc.h"
#include "PCI.h"
#include "SystemProfiler.h"
#include "Process.h"
#include "Thread.h"
#include "SkyDebugger.h"
#include "SkyGUILauncher.h"
#include "lua.h"
#include "lualib.h"

long CmdCls(char *theCommand)
{
	SkyConsole::Clear();
	return false;
}

long CmdKill(char *theCommand)
{
	int id = atoi(theCommand);

	kEnterCriticalSection();

	Process* pProcess = ProcessManager::GetInstance()->FindProcess(id);

	kLeaveCriticalSection();

	if (pProcess != nullptr)
	{
		SkyConsole::Print("kill process : %s, ID : %d\n", pProcess->m_processName, pProcess->GetProcessId());
		ProcessManager::GetInstance()->RemoveProcess(pProcess->GetProcessId());
	}
	else
		SkyConsole::Print("process don't exist(%d)\n", id);	

	return false;
}

long CmdProcessList(char *theCommand)
{
	kEnterCriticalSection();
	SkyConsole::Print(" ID : Process Name\n");

	ProcessManager::ProcessList* processlist = ProcessManager::GetInstance()->GetProcessList();
	map<int, Process*>::iterator iter = processlist->begin();

	for (; iter != processlist->end(); ++iter)
	{
		Process* pProcess = (*iter).second;
		SkyConsole::Print("  %d %s\n", pProcess->GetProcessId(), pProcess->m_processName);
	}

	kLeaveCriticalSection();

	return true;
}

long cmdMemState(char *theCommand)
{
	SystemProfiler::GetInstance()->PrintMemoryState();
	return false;
}

long cmdCreateWatchdogTask(char* pName)
{
	kEnterCriticalSection();
	
	Process* pProcess = ProcessManager::GetInstance()->CreateProcessFromMemory("WatchDog", WatchDogProc, NULL, PROCESS_KERNEL);	
	kLeaveCriticalSection();
	
	if(pProcess == nullptr)
		SkyConsole::Print("Can't create process\n");	

	return false;
}

long cmdTaskCount(char *theCommand)
{
	kEnterCriticalSection();

	ProcessManager::TaskList* taskList = ProcessManager::GetInstance()->GetTaskList();
	SkyConsole::Print("current task count %d\n", taskList->size());

	kLeaveCriticalSection();
	return false;
}

long cmdGlobalState(char *theCommand)
{
	SystemProfiler::GetInstance()->PrintGlobalState();
	return false;
}

long CmdExec(char *theCommand)
{

	Process* pProcess = ProcessManager::GetInstance()->CreateProcessFromFile(theCommand, nullptr, PROCESS_USER);

	if (pProcess == nullptr)
		SkyConsole::Print("Can't create process %s\n", theCommand);
	

	return false;
}
#include "SkyMockInterface.h"
#include "I_LuaModule.h"
extern SKY_FILE_Interface g_FileInterface;
extern SKY_ALLOC_Interface g_allocInterface;
extern SKY_Print_Interface g_printInterface;

typedef void(*PSetSkyMockInterface)(SKY_ALLOC_Interface, SKY_FILE_Interface, SKY_Print_Interface);
typedef I_LuaModule*(*PGetLuaModule)();

extern FILE* g_stdOut;
extern FILE* g_stdIn;
extern FILE* g_stdErr;

I_LuaModule* pLuaModule = nullptr;
long cmdLua3(char *theCommand)
{
	g_printInterface.sky_stdin = g_stdIn;
	g_printInterface.sky_stdout = g_stdOut;
	g_printInterface.sky_stderr = g_stdErr;

	if (theCommand == nullptr)
		return false;

	StorageManager::GetInstance()->SetCurrentFileSystemByID('L');

	if (pLuaModule != nullptr)
	{
		bool result = pLuaModule->DoFile(theCommand);

		if (result == false)
			SkyConsole::Print("Lua Exec Fail\n");

		return false;
	}
	
	MODULE_HANDLE hwnd = SkyModuleManager::GetInstance()->LoadModuleFromMemory("LUA3_DLL");

	if (hwnd == nullptr)
	{
		HaltSystem("LUA3_DLL Module Load Fail!!");
	}

	PSetSkyMockInterface SetSkyMockInterface = (PSetSkyMockInterface)SkyModuleManager::GetInstance()->GetModuleFunction(hwnd, "SetSkyMockInterface");
	PGetLuaModule GetLuaModuleInterface = (PGetLuaModule)SkyModuleManager::GetInstance()->GetModuleFunction(hwnd, "GetLuaModule");

	//디버그 엔진에 플랫폼 종속적인 인터페이스를 넘긴다.
	SetSkyMockInterface(g_allocInterface, g_FileInterface, g_printInterface);

	if (!GetLuaModuleInterface)
	{
		HaltSystem("Memory Module Load Fail!!");
	}

	pLuaModule = GetLuaModuleInterface();

	if (pLuaModule == nullptr)
	{
		HaltSystem("Lua Module Creation Fail!!");
	}


	pLuaModule->InitLua();	
	bool result = pLuaModule->DoFile(theCommand);
	//pLuaModule->CloseLua();

	if (result == false)
		SkyConsole::Print("Lua Exec Fail\n");

	return false;
}

int cpp_func(int arg1, int arg2)
{
	return arg1 + arg2;
}

static int cpp_int = 100;

#include "luatinker.h"
#include "lualib.h"

void TestLua51(lua_State* L)
{

	luatinker::def(L, "cpp_func", cpp_func);
	luatinker::dofile(L, "sample1.lua");
	int result = luatinker::call<int>(L, "lua_func", 3, 4);
	printf("lua_func(3,4) = %d\n", result);
}

void TestLua52(lua_State* L)
{
	
	// LuaTinker 를 이용해서 cpp_int 를 Lua에 전달
	luatinker::set(L, "cpp_int", cpp_int);

	// sample1.lua 파일을 로드/실행한다.
	luatinker::dofile(L, "sample2.lua");

	// sample1.lua 의 함수를 호출한다.
	int lua_int = luatinker::get<int>(L, "lua_int");

	// lua_int 값 출력
	printf("lua_int = %d\n", lua_int);

}

struct A
{
	A(int v) : value(v) {}
	int value;
};

struct base
{
	base() {}

	const char* is_base() { return "this is base"; }
};

class test : public base
{
public:
	test(int val) : _test(val) {}
	~test() {}

	const char* is_test() { return "this is test"; }

	void ret_void() {}
	int ret_int() { return _test; }
	int ret_mul(int m) const { return _test * m; }
	A get() { return A(_test); }
	void set(A a) { _test = a.value; }

	int _test;
};

//test g_test(11);

void TestLua53(lua_State* L)
{
	test g_test(11);
	
	// Lua 문자열 함수들을 로드한다.- string 사용
	luaopen_string(L);

	// base 클래스를 Lua 에 추가한다.
	luatinker::class_add<base>(L, "base");
	// base 의 함수를 등록한다.
	luatinker::class_def<base>(L, "is_base", &base::is_base);

	// test 클래스를 Lua 에 추가한다.
	luatinker::class_add<test>(L, "test");
	// test 가 base 에 상속 받았음을 알려준다.
	luatinker::class_inh<test, base>(L);

	// test 클래스 생성자를 등록한다.
	luatinker::class_con<test>(L, luatinker::constructor<test, int>);
	// test 의 함수들을 등록한다.
	luatinker::class_def<test>(L, "is_test", &test::is_test);
	luatinker::class_def<test>(L, "ret_void", &test::ret_void);
	luatinker::class_def<test>(L, "ret_int", &test::ret_int);
	luatinker::class_def<test>(L, "ret_mul", &test::ret_mul);
	luatinker::class_def<test>(L, "get", &test::get);
	luatinker::class_def<test>(L, "set", &test::set);
	luatinker::class_mem<test>(L, "_test", &test::_test);

	// Lua 전역 변수호 g_test 의 포인터를 등록한다.
	luatinker::set(L, "g_test", &g_test);

	// sample3.lua 파일을 로드/실행한다.
	luatinker::dofile(L, "sample3.lua");
}

void TestLua54(lua_State* L)
{
	
	// Lua 테이블을 생성하고 스택에 푸쉬한다.
	luatinker::table haha(L, "haha");

	// haha.value = 1 값을 넣는다.
	haha.set("value", 1);

	// table 내에 table을 만들어 넣는다.
	haha.set("inside", luatinker::table(L));

	// haha.inside 의 포인터를 스택위로 복사한다.
	luatinker::table inside = haha.get<luatinker::table>("inside");

	// inside.value = 2 값을 넣는다.
	inside.set("value", 2);

	// sample4.lua 파일을 로드/실행한다.
	luatinker::dofile(L, "sample4.lua");

	// Lua 에서 생성한 haha.test 값을 읽는다.
	const char* test = haha.get<const char*>("test");
	printf("haha.test = %s\n", test);

	// 전역에 등록하지 않고 Lua 스택에 빈 테이블을 생성한다.(지역변수)
	luatinker::table temp(L);

	// 빈 테이블.name 에 값을 넣는다.
	temp.set("name", "local table !!");

	// table을 의 인자로 사용하여 print_table 을 호출한다.
	luatinker::call<void>(L, "print_table", temp);

	// 함수가 리턴하는 table을 받는다.
	luatinker::table ret = luatinker::call<luatinker::table>(L, "return_table", "give me a table !!");
	printf("ret.name =\t%s\n", ret.get<const char*>("name"));

}

void show_error(const char* error)
{
	printf("_ALERT -> %s\n", error);
}

void TestLua55(lua_State* L)
{

	// lua_State* 가 갖고있는 현재의 스택을 보여준다.
	printf("%s\n", "-------------------------- current stack");
	luatinker::enum_stack(L);

	// 스택에 1을 밀어넣는다.
	lua_pushnumber(L, 1);

	// 현재 스택의 내용을 다시 출력한다.
	printf("%s\n", "-------------------------- stack after push '1'");
	luatinker::enum_stack(L);


	// sample5.lua 파일을 로드/실행한다.
	luatinker::dofile(L, "sample5.lua");

	// test_error() 함수를 호출한다.
	// test_error() 는 실행중 test_error_3() 호출을 시도하다 에러를 발생시킨다.
	// 함수 호출중 발생한 에러는 printf()를 통해서 출력된다.
	printf("%s\n", "-------------------------- calling test_error()");
	luatinker::call<void>(L, "test_error");

	// test_error_3()는 존재하지 않는 함수이다. 호출 자체가 실패한다.
	printf("%s\n", "-------------------------- calling test_error_3()");
	luatinker::call<void>(L, "test_error_3");

	// printf() 대신 유저가 제공하는 에러 출력 루틴을 사용할 수 있다.
	// 이 에러처리 함수는1개의 루아 문자열로 발생한 에러를 전달하게 된다.
	// C++ 에서 등록할 경우 void function(const char*) 형태가 적합하다.
	luatinker::def(L, "_ALERT", show_error);

	luatinker::call<void>(L, "_ALERT", "test !!!");

	// test_error() 함수를 호출한다.
	// 함수 호출중 발생한 에러는 Lua에 등록된 _ALERT()를 통해서 출력된다.
	printf("%s\n", "-------------------------- calling test_error()");
	luatinker::call<void>(L, "test_error");
}

// 함수 형태가 int(*)(lua_State*) 또는 int(*)(lua_State*,T1) 일 경우만 lua_yield() 를 쓸 수 있다.
// 함수 인자가 더 필요할 경우 lua_tinker.h 의 "functor (non-managed)" 코멘트 부분을 참고해서 직접 추가할 것
int TestFunc(lua_State* L)
{
	printf("# TestFunc 실행중\n");
	return lua_yield(L, 0);
}

int TestFunc2(lua_State* L, float a)
{
	printf("# TestFunc2(L,%f) 실행중\n", a);
	return lua_yield(L, 0);
}

class TestClass
{
public:

	// 함수 형태가 int(T::*)(lua_State*) 또는 int(T::*)(lua_State*,T1) 일 경우만 lua_yield() 를 쓸 수 있다.
	// 함수 인자가 더 필요할 경우 lua_tinker.h 의 "class member functor (non-managed)" 코멘트 부분을 참고해서 직접 추가할 것
	int TestFunc(lua_State* L)
	{
		printf("# TestClass::TestFunc 실행중\n");
		return lua_yield(L, 0);
	}

	int TestFunc2(lua_State* L, float a)
	{
		printf("# TestClass::TestFunc2(L,%f) 실행중\n", a);
		return lua_yield(L, 0);
	}
};

void TestLua56(lua_State* L)
{
	// Lua 문자열 함수들을 로드한다.- string 사용
	luaopen_string(L);

	// TestFunc 함수를 Lua 에 등록한다.
	luatinker::def(L, "TestFunc", &TestFunc);
	luatinker::def(L, "TestFunc2", &TestFunc2);

	// TestClass 클래스를 Lua 에 추가한다.
	luatinker::class_add<TestClass>(L, "TestClass");
	// TestClass 의 함수를 등록한다.
	luatinker::class_def<TestClass>(L, "TestFunc", &TestClass::TestFunc);
	luatinker::class_def<TestClass>(L, "TestFunc2", &TestClass::TestFunc2);

	// TestClass 를 전역 변수로 선언한다.
	TestClass g_test;
	luatinker::set(L, "g_test", &g_test);

	// sample3.lua 파일을 로드한다.
	luatinker::dofile(L, "sample6.lua");

	// Thread 를 시작한다.
	lua_State *co = lua_newthread(L);
	lua_getglobal(co, "ThreadTest");
	// Thread 를 시작한다.
	int result = 0;
	printf("* lua_resume() call\n");
	lua_resume(co, L, 0, &result);

	// Thread 를 다시 시작한다.
	printf("* lua_resume() call\n");
	lua_resume(co, L, 0, &result);

	// Thread 를 다시 시작한다.
	printf("* lua_resume() call\n");
	lua_resume(co, L, 0, &result);

	// Thread 를 다시 시작한다.
	printf("* lua_resume() call\n");
	lua_resume(co, L, 0, &result);


	// Thread 를 다시 시작한다.
	printf("* lua_resume() call\n");
	lua_resume(co, L, 0, &result);
}

long cmdLua5(char *theCommand)
{
	StorageManager::GetInstance()->SetCurrentFileSystemByID('L');
	
	if (theCommand == nullptr)
		return false;

	kEnterCriticalSection();
	lua_State* L;
	L = luaL_newstate();

	luaopen_base(L);

	if (strcmp(theCommand, "sample1.lua") == 0)
	{
		TestLua51(L);
	}

	if (strcmp(theCommand, "sample2.lua") == 0)
	{
		TestLua52(L);
	}

	if (strcmp(theCommand, "sample3.lua") == 0)
	{
		TestLua53(L);
	}

	if (strcmp(theCommand, "sample4.lua") == 0)
	{
		TestLua54(L);
	}

	if (strcmp(theCommand, "sample5.lua") == 0)
	{
		TestLua55(L);
	}
	if (strcmp(theCommand, "sample6.lua") == 0)
	{
		TestLua56(L);
	}

	lua_close(L);
	
	kLeaveCriticalSection();
	return false;
}

long cmdGUI(char *theCommand)
{
	RequesGUIResolution();

	return false;
}

long cmdCD(char *theCommand)
{
	int size = strlen(theCommand);
	if (size != 2 || theCommand[1] != ':')
	{
		SkyConsole::Print("Invalid Argument\n");
		return false;
	}
	char driveLetter = toupper(theCommand[0]);
	StorageManager::GetInstance()->SetCurrentFileSystemByID(driveLetter);

	return false;
}

void FillRect8(int x, int y, int w, int h, char col, int actualX, int actualY)
{
	char* lfb = (char*)0xF0000000;

	for (int k = 0; k < h; k++)
		for (int j = 0; j < w; j++)
		{
			int index = ((j + x) + (k + y) * actualX);
			lfb[index] = col;
			index++;
		}

}

long cmdSwitchGUI(char *theCommand)
{
	//성공적으로 그래픽 모드가 전환되는지 확인
	if(true == SwitchGUIMode(1024, 768, 261))
	{
		//그래픽 버퍼 주소를 매핑해야 한다.
		VirtualMemoryManager::CreateVideoDMAVirtualAddress(VirtualMemoryManager::GetCurPageDirectory(), 0xE0000000, 0xE0000000, 0xE0FF0000);
		VirtualMemoryManager::CreateVideoDMAVirtualAddress(VirtualMemoryManager::GetCurPageDirectory(), 0xF0000000, 0xF0000000, 0xF0FF0000);
		
		//사각형을 그린다.
		FillRect8(100, 100, 100, 100, 8, 1024, 768);
		for (;;);
	}
	
	return false;
}


long cmdPCI(char *theCommand)
{
	RequestPCIList();

	return false;
}

long cmdCallStack(char *theCommand)
{
	SkyDebugger::GetInstance()->TraceStack();

	return false;
}

long cmdCallStack2(char *theCommand)
{
	SkyDebugger::GetInstance()->TraceStackWithSymbol();

	return false;
}

long cmdDir(char *theCommand)
{
	StorageManager::GetInstance()->GetFileList();

	return false;
}


