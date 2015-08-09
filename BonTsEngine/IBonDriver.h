// IBonDriver.h: IBonDriver �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#if !defined(_IBONDRIVER_H_)
#define _IBONDRIVER_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// �}�h���C�o�C���^�t�F�[�X
class IBonDriver
{
public:
	virtual const BOOL OpenTuner(void) = 0;
	virtual void CloseTuner(void) = 0;

	virtual const BOOL SetChannel(const BYTE bCh) = 0;
	virtual const float GetSignalLevel(void) = 0;

	virtual const DWORD WaitTsStream(const DWORD dwTimeOut = 0) = 0;
	virtual const DWORD GetReadyCount(void) = 0;

	virtual const BOOL GetTsStream(BYTE *pDst, DWORD *pdwSize, DWORD *pdwRemain) = 0;
	virtual const BOOL GetTsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain) = 0;

	virtual void PurgeTsStream(void) = 0;

	virtual void Release(void) = 0;
};


// �C���X�^���X�������\�b�h
extern "C" __declspec(dllimport) IBonDriver * CreateBonDriver();


#endif // !defined(_IBONDRIVER_H_)
