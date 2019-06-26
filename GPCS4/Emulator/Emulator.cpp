#include "Emulator.h"
#include "GameThread.h"


CEmulator::CEmulator()
{
}


CEmulator::~CEmulator()
{
}

bool CEmulator::Init()
{
	bool bRet = false;
	do 
	{
		if (!RegisterModules())
		{
			break;
		}

		m_pLinker = std::make_shared<CLinker>();

#ifdef GPCS4_WINDOWS
		m_pTlsHandler = std::make_unique<CTLSHandlerWin>();
#else

#endif // GPCS4_WINDOWS

		if (!m_pTlsHandler->Install())
		{
			break;
		}

		bRet = true;
	} while (false);
	return bRet;
}

void CEmulator::Unit()
{

}

bool CEmulator::LoadEboot(const std::string& strEbtPath)
{
	bool bRet = false;
	do 
	{
		m_oEboot.SetLinker(m_pLinker);

		if (!m_oEboot.Load(strEbtPath))
		{
			break;
		}

		bRet = true;
	} while (false);
	return bRet;
}

bool CEmulator::Run()
{

	bool bRet = false;
	do 
	{

		void* pEntryPoint = m_oEboot.EntryPoint();
		if (!pEntryPoint)
		{
			break;
		}

		const int nEnvNum = 0x10;
		uint64 pEnv[nEnvNum] = { 0xDEADBEE1, 0xDEADBEE2, 0xDEADBEE3, 0xDEADBEE4, 0xDEADBEE5,
			0xDEADBEE6, 0xDEADBEE7, 0xDEADBEE8, 0xDEADBEE9, 0xDEADBEEA };

		LOG_DEBUG("run into eboot.");
		CGameThread oMainThread(pEntryPoint, pEnv, (void*)CEmulator::LastExitHandler);
		if (!oMainThread.Start())
		{
			break;
		}

		// block the emulator's thread
		if (!oMainThread.Join(NULL))
		{
			break;
		}

		bRet = true;
	} while (false);
	return bRet;
}

void PS4API CEmulator::LastExitHandler(void)
{
	LOG_DEBUG("program exit.");
}
