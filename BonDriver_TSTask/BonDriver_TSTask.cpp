#include "stdafx.h"
#include "../Common/TSTaskCommon.h"
#define dllimport dllexport
#include "IBonDriver2.h"
#if !defined(BONDRIVER_TSTASK_CLIENT) && !defined(BONDRIVER_TSTASK_NO_CLIENT)
// クライアントとして登録してタスクの終了/起動に追従する
#define BONDRIVER_TSTASK_CLIENT
#endif
#ifdef BONDRIVER_TSTASK_CLIENT
#include "TaskClient.h"
#endif
#include "../Common/DebugDef.h"


class CBonDriver_TSTask
	: public IBonDriver2
#ifdef BONDRIVER_TSTASK_CLIENT
	, protected CTaskClient
#endif
{
public:
	CBonDriver_TSTask();
	~CBonDriver_TSTask();

// IBonDriver
	const BOOL OpenTuner() override;
	void CloseTuner() override;
	const BOOL SetChannel(const BYTE bCh) override;
	const float GetSignalLevel() override;
	const DWORD WaitTsStream(const DWORD dwTimeOut) override;
	const DWORD GetReadyCount() override;
	const BOOL GetTsStream(BYTE *pDst,DWORD *pdwSize,DWORD *pdwRemain) override;
	const BOOL GetTsStream(BYTE **ppDst,DWORD *pdwSize,DWORD *pdwRemain) override;
	void PurgeTsStream() override;
	void Release() override;

// IBonDriver2
	LPCTSTR GetTunerName() override;
	const BOOL IsTunerOpening() override;
	LPCTSTR EnumTuningSpace(const DWORD dwSpace) override;
	LPCTSTR EnumChannelName(const DWORD dwSpace,const DWORD dwChannel) override;
	const BOOL SetChannel(const DWORD dwSpace,const DWORD dwChannel) override;
	const DWORD GetCurSpace() override;
	const DWORD GetCurChannel() override;

private:
	void Close();
	bool CheckEnded();

#ifdef BONDRIVER_TSTASK_CLIENT
// CTaskClient
	void OnTaskStarted(TSTask::TaskID TaskID) override;
	void OnTaskEnded(TSTask::TaskID TaskID) override;
#endif

	static const DWORD INVALID_CHANNEL=0xFFFFFFFFUL;

	TSTask::CStreamPool m_StreamPool;
	TSTask::CStreamPool::CStreamPosition m_StreamPos;
	bool m_fStreamPoolCreated;
	TSTask::CDataBlock m_Buffer;
	size_t m_BufferSize;
	TSTask::CLocalMessageClient m_MessageClient;
	TSTask::CServerTaskSharedInfoReader m_SharedInfoReader;
	DWORD m_CurChannel;
	TSTask::CLocalLock m_Lock;
};

CBonDriver_TSTask::CBonDriver_TSTask()
	: m_fStreamPoolCreated(false)
	, m_BufferSize(188*1024)
	, m_CurChannel(INVALID_CHANNEL)
{
}

CBonDriver_TSTask::~CBonDriver_TSTask()
{
	CloseTuner();
}

const BOOL CBonDriver_TSTask::OpenTuner()
{
	TSTask::CBlockLock Lock(m_Lock);

	if (!m_Buffer.Reserve(m_BufferSize))
		return FALSE;

	InitializeClient();

	return TRUE;
}

void CBonDriver_TSTask::CloseTuner()
{
	TSTask::CBlockLock Lock(m_Lock);

	Close();
	m_Buffer.Free();
	FinalizeClient();
}

const BOOL CBonDriver_TSTask::SetChannel(const BYTE bCh)
{
	return SetChannel(0,bCh);
}

const float CBonDriver_TSTask::GetSignalLevel()
{
	if (!CheckEnded()) {
		TSTask::StreamStatistics Statistics;
		if (m_SharedInfoReader.GetStreamStatistics(&Statistics))
			return Statistics.SignalLevel;
	}

	return 0.0f;
}

const DWORD CBonDriver_TSTask::WaitTsStream(const DWORD dwTimeOut)
{
	// 未実装
	::Sleep(0);
	return WAIT_OBJECT_0;
}

const DWORD CBonDriver_TSTask::GetReadyCount()
{
	// 未実装
	return 0;
}

const BOOL CBonDriver_TSTask::GetTsStream(BYTE *pDst,DWORD *pdwSize,DWORD *pdwRemain)
{
	*pdwRemain=0;

	if (*pdwSize<188)
		return FALSE;

	const DWORD MaxLength=*pdwSize/188;
	DWORD Length=MaxLength;
	if (m_StreamPool.Read(pDst,&Length,&m_StreamPos)) {
		*pdwSize=Length*188;
		*pdwRemain=Length<MaxLength?0:1;
		return TRUE;
	} else {
		CheckEnded();
	}

	*pdwSize=0;

	return FALSE;
}

const BOOL CBonDriver_TSTask::GetTsStream(BYTE **ppDst,DWORD *pdwSize,DWORD *pdwRemain)
{
	const DWORD MaxLength=(DWORD)(m_Buffer.GetReservedSize()/188);
	DWORD Length=MaxLength;
	if (m_StreamPool.Read(m_Buffer.GetBuffer(),&Length,&m_StreamPos)) {
		*ppDst=static_cast<BYTE*>(m_Buffer.GetBuffer());
		*pdwSize=Length*188;
		*pdwRemain=Length<MaxLength?0:1;
		return TRUE;
	} else {
		CheckEnded();
	}

	*pdwSize=0;
	*pdwRemain=0;

	return FALSE;
}

void CBonDriver_TSTask::PurgeTsStream()
{
	m_StreamPos.SetTail();
}

void CBonDriver_TSTask::Release()
{
	delete this;
}

LPCTSTR CBonDriver_TSTask::GetTunerName()
{
	return TEXT("TSTask");
}

const BOOL CBonDriver_TSTask::IsTunerOpening()
{
	return m_StreamPool.IsOpened();
}

LPCTSTR CBonDriver_TSTask::EnumTuningSpace(const DWORD dwSpace)
{
	if (dwSpace!=0)
		return nullptr;

	return TEXT("TSTask");
}

LPCTSTR CBonDriver_TSTask::EnumChannelName(const DWORD dwSpace,const DWORD dwChannel)
{
	if (dwSpace!=0 || dwChannel>=TSTask::MAX_SERVER_TASKS)
		return nullptr;

	static TCHAR szText[32];
	// 最後の "ch" は、TVTest でコマンドラインからチャンネル指定できるようにするためのおまじない
	TSTask::FormatString(szText,_countof(szText),TEXT("タスク%uch"),dwChannel+1);

	return szText;
}

const BOOL CBonDriver_TSTask::SetChannel(const DWORD dwSpace,const DWORD dwChannel)
{
	if (dwSpace!=0 || dwChannel>=TSTask::MAX_SERVER_TASKS)
		return FALSE;

	TSTask::CBlockLock Lock(m_Lock);

	Close();

	TSTask::TaskID TaskID=TSTask::TaskID(dwChannel+1);

	TSTask::String Name;
	if (!TSTask::TaskUtility::GetServerTaskLocalMessageServerName(TaskID,&Name))
		return FALSE;
	m_MessageClient.SetServer(Name);

#ifdef BONDRIVER_TSTASK_CLIENT
	SetServerTask(TaskID);
#endif

	TSTask::CMessage Message(TSTask::MESSAGE_CreateStreamPool);
	TSTask::CMessage Response;
	TSTask::String Result;
	if (!m_MessageClient.SendMessage(&Message,&Response)
			|| !Response.GetProperty(TSTask::MESSAGE_PROPERTY_Result,&Result)
			|| Result!=TSTask::MESSAGE_RESULT_OK)
		//return FALSE;
		return TRUE;

	m_fStreamPoolCreated=true;

	if (!TSTask::TaskUtility::GetStreamPoolName(TaskID,&Name)) {
		Close();
		return FALSE;
	}

	if (!m_StreamPool.Open(Name.c_str())) {
		Close();
		return FALSE;
	}

	m_SharedInfoReader.Open(TaskID);

	m_CurChannel=dwChannel;

	m_StreamPos.SetTail();

	return TRUE;
}

const DWORD CBonDriver_TSTask::GetCurSpace()
{
	return 0;
}

const DWORD CBonDriver_TSTask::GetCurChannel()
{
	return m_CurChannel;
}

void CBonDriver_TSTask::Close()
{
	TSTask::CBlockLock Lock(m_Lock);

	m_StreamPool.Close();
	if (m_fStreamPoolCreated) {
		TSTask::CMessage Message(TSTask::MESSAGE_CloseStreamPool);
		m_MessageClient.SendMessage(&Message);
		m_fStreamPoolCreated=false;
	}
	m_Buffer.SetSize(0);
	m_SharedInfoReader.Close();
	m_CurChannel=INVALID_CHANNEL;
	m_StreamPos.SetHead();
}

bool CBonDriver_TSTask::CheckEnded()
{
	TSTask::CBlockLock Lock(m_Lock);

	if (m_StreamPool.IsEnded()) {
		m_StreamPool.Close();
		m_fStreamPoolCreated=false;
		m_SharedInfoReader.Close();
		return true;
	}

	return false;
}

#ifdef BONDRIVER_TSTASK_CLIENT

void CBonDriver_TSTask::OnTaskStarted(TSTask::TaskID TaskID)
{
	if (m_CurChannel==TaskID-1)
		SetChannel(0,m_CurChannel);
}

void CBonDriver_TSTask::OnTaskEnded(TSTask::TaskID TaskID)
{
	if (m_CurChannel==TaskID-1)
		CheckEnded();
}

#endif


static HINSTANCE g_hInstance=nullptr;
static CBonDriver_TSTask *g_pBonDriver=nullptr;

BOOL APIENTRY DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		g_hInstance=hinstDLL;
		TSTASK_DEBUG_INITIALIZE;
		break;

	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

extern "C" __declspec(dllexport) IBonDriver * CreateBonDriver()
{
	if (g_pBonDriver==nullptr)
		g_pBonDriver=new CBonDriver_TSTask;

	return g_pBonDriver;
}
