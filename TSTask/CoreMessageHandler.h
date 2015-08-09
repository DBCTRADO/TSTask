#ifndef TSTASK_CORE_MESSAGE_HANDLER_H
#define TSTASK_CORE_MESSAGE_HANDLER_H


#include "TSTaskCore.h"
#include "TSTaskAppCore.h"


namespace TSTask
{

	class CCoreMessageHandler
	{
	public:
		CCoreMessageHandler(CTSTaskCore &Core,CTSTaskAppCore &AppCore);
		~CCoreMessageHandler();
		bool SetHandler(CMessageMap *pMessageMap);

	private:
		bool OnHello(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnEndTask(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnLoadBonDriver(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnUnloadBonDriver(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnGetBonDriver(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnOpenTuner(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnCloseTuner(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnSetChannel(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnGetChannel(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnSetService(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnGetService(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnGetServiceList(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnStartRecording(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnStopRecording(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnChangeRecordingFile(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnGetRecordingInfo(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnGetStreamStatistics(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnResetErrorStatistics(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnGetEventInfo(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnGetLog(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnGetBonDriverChannelList(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnGetScannedChannelList(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnCreateStreamPool(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnCloseStreamPool(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnGetTvRockInfo(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnStartStreaming(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnStopStreaming(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnGetStreamingInfo(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnGetStreamingSettings(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnGetSetting(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnIniSettingsChanged(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);

		CTSTaskCore &m_Core;
		CTSTaskAppCore &m_AppCore;
	};

}


#endif
