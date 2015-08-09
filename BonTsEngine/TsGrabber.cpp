// TsGrabber.cpp: CTsGrabber �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsGrabber.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// �\�z/����
//////////////////////////////////////////////////////////////////////

CTsGrabber::CTsGrabber(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 1UL)
{
}

CTsGrabber::~CTsGrabber()
{
}

void CTsGrabber::Reset(void)
{
	CBlockLock Lock(&m_DecoderLock);

	// �R�[���o�b�N�ɒʒm����
	for (std::vector<ITsHandler*>::iterator i=m_TsHandlerList.begin();i!=m_TsHandlerList.end();i++)
		(*i)->OnReset();
}

const bool CTsGrabber::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	/*
	if (dwInputIndex >= GetInputNum())
		return false;

	CTsPacket *pTsPacket = dynamic_cast<CTsPacket *>(pMediaData);

	// ���̓��f�B�A�f�[�^�͌݊������Ȃ�
	if (!pTsPacket)
		return false;
	*/

	// �R�[���o�b�N�ɒʒm����
	CTsPacket *pTsPacket = static_cast<CTsPacket *>(pMediaData);
	bool bOutput = true;
	for (std::vector<ITsHandler*>::iterator i=m_TsHandlerList.begin();i!=m_TsHandlerList.end();i++) {
		if (!(*i)->OnPacket(pTsPacket))
			bOutput = false;
	}

	// ���ʃf�R�[�_�Ƀf�[�^��n��
	if (bOutput)
		OutputMedia(pMediaData);

	return true;
}

bool CTsGrabber::AddTsHandler(ITsHandler *pTsHandler)
{
	if (!pTsHandler)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	m_TsHandlerList.push_back(pTsHandler);

	return true;
}

bool CTsGrabber::RemoveTsHandler(ITsHandler *pTsHandler)
{
	CBlockLock Lock(&m_DecoderLock);

	for (std::vector<ITsHandler*>::iterator i=m_TsHandlerList.begin();i!=m_TsHandlerList.end();i++) {
		if (*i==pTsHandler) {
			m_TsHandlerList.erase(i);
			return true;
		}
	}

	return false;
}
