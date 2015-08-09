// TsGrabber.h: CTsGrabber �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include <vector>
#include "MediaDecoder.h"
#include "TsStream.h"


/////////////////////////////////////////////////////////////////////////////
// TS�O���o(�O���t��ʉ߂���CTsPacket�ɃA�N�Z�X�����i��񋟂���)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CMediaData		���̓f�[�^
// Output	#0	: CMediaData		�o�̓f�[�^
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
