// TsGrabber.h: CTsGrabber クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include <vector>
#include "MediaDecoder.h"
#include "TsStream.h"


/////////////////////////////////////////////////////////////////////////////
// TSグラバ(グラフを通過するCTsPacketにアクセスする手段を提供する)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CMediaData		入力データ
// Output	#0	: CMediaData		出力データ
/////////////////////////////////////////////////////////////////////////////

class CTsGrabber : public CMediaDecoder
{
public:
	class ABSTRACT_CLASS_DECL ITsHandler
	{
	public:
		virtual ~ITsHandler() {}
		virtual bool OnPacket(const CTsPacket *pPacket) { return true; }
		virtual void OnReset() {}
	};

	CTsGrabber(IEventHandler *pEventHandler = NULL);
	virtual ~CTsGrabber();

// CMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CMediaGrabber
	bool AddTsHandler(ITsHandler *pTsHandler);
	bool RemoveTsHandler(ITsHandler *pTsHandler);

protected:
	std::vector<ITsHandler*> m_TsHandlerList;
};
