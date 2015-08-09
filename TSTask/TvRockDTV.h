#pragma once
#include <windows.h>

// TvRockDTV���C�u�����͈ȉ��̃v���O�����œǂݍ��݂܂�
//
// typedef TvRockDTV *(*CreateTvRockDTV)(HINSTANCE);
// TvRockDTV *pTvRock = NULL;
// HINSTANCE plug = LoadDTVModule();
// if ( plug != NULL ) {
// 	CreateTvRockDTV func = (CreateTvRockDTV)GetProcAddress(plug,TRDTV_MODULE_FUNC);
//	if ( func != NULL )
//		pTvRock = func(plug);
//
namespace TvRock {

#define TRDTV_TVROCK_NAME		L"TvRock"
#define TRDTV_MODULE_NAME		L"\\TvRockDTV.dll"
#define TRDTV_MODULE_FUNC		 "CreateTvRockDTV"

#if defined(__STDC__) || defined(__ANSI_CPP__) || defined(WIN32)	/* ANSI C */
#define _QUOTE(str)	#str
#else	/* Non-ANSI C */
#define _QUOTE(str)	"str"
#endif

#define __CLASS(classname)					\
public:										\
	static bool LaterThanClass(DtvObject *obj1) {				\
		return (memcmp(obj1->GetClassName(),_QUOTE(classname),strlen(_QUOTE(classname))) <= 0); \
	}															\
	static LPCSTR ClassName() {									\
		return _QUOTE(classname);								\
	}															\
	LPCSTR GetClassName() const { return _QUOTE(classname); }	\
	virtual bool LaterThan(LPCSTR cname) {						\
		return (memcmp(_QUOTE(classname),cname,strlen(cname)) >= 0); \
	}															\
	virtual bool LaterThan(DtvObject *obj1) {					\
		return classname::LaterThanClass(obj1);					\
	}
//	DWORD GetClassTypeId() { return classTypeId(); }

// ���W���[���ǂݍ���
static inline LONG __read_software_registry(LPCWSTR key, LPCWSTR name, LPWSTR buf, const DWORD maxsz) { 
	HKEY hKey; 
	DWORD sz=maxsz; 
	LONG rs=ERROR_PATH_NOT_FOUND;
	if (RegOpenKeyEx(HKEY_CURRENT_USER,key,0,KEY_READ,&hKey)==ERROR_SUCCESS) {
		rs=RegQueryValueEx(hKey,name,NULL,NULL,(LPBYTE)buf,&sz);
		RegCloseKey(hKey);
	} 
	return rs;
}
static inline HINSTANCE LoadDTVModule() {
	WCHAR path[MAX_PATH];
	memset(path,0,sizeof(path));
	if (__read_software_registry(TEXT("SOFTWARE\\") TRDTV_TVROCK_NAME,TEXT("DOCUMENT"),path,MAX_PATH*sizeof(WCHAR)))
		return NULL;
	wcscat_s(path,MAX_PATH,TRDTV_MODULE_NAME);
	return LoadLibrary(path);
}

// ���
class DtvObject {
public:
	virtual ~DtvObject() {}
	virtual LPCSTR GetClassName() const = 0;
	static inline bool SameClass(DtvObject *obj1,DtvObject *obj2) {
		return (strcmp(obj1->GetClassName(),obj2->GetClassName())==0);
	}
	virtual inline bool SameClass(DtvObject *obj) {
		return SameClass(obj,this);
	}
	virtual bool LaterThan(LPCSTR cname) = 0;
	virtual bool LaterThan(DtvObject *obj1) = 0;
//	virtual DWORD GetClassTypeId() = 0;
};


// �Ǐ��
class StationInfo_V100 : public DtvObject {
	__CLASS(StationInfo_V100)

public:
	// �ǖ�
	virtual void  GetName(LPWSTR lpPath, SIZE_T MaxSize) = 0;
	virtual void  SetName(LPCWSTR lpPath) = 0;
	// �`�����l��
	virtual DWORD GetChannel() const = 0;
	virtual void  SetChannel(const DWORD) = 0;
	// �T�[�r�X
	virtual DWORD GetServiceID() const = 0;
	virtual void  SetServiceID(const DWORD) = 0;
};

#define StationInfo		StationInfo_V100


// �v���p�e�B
class RockProperty_V100 : public DtvObject {
	__CLASS(RockProperty_V100)

// �`���[�i�[�ݒ���
public:
	// ���e���X�V����TvRock�֓`����
	virtual void Notify() = 0;
	// �n��g
	virtual bool UseTS() const = 0;
	// BS
	virtual bool UseBS() const = 0;
	// CS
	virtual bool UseCS() const = 0;
	// �A�v���P�[�V�����t�H���_
	virtual void GetAppPath(LPWSTR lpPath, SIZE_T MaxSize) = 0;
	virtual void SetAppPath(LPCWSTR lpPath) = 0;
	// ���s�A�v�����i�����L��j
	virtual void GetAppName(LPWSTR lpPath, SIZE_T MaxSize) = 0;
	virtual void SetAppName(LPCWSTR lpPath) = 0;
	// ���s�A�v�����i���������j
	virtual void GetAppNameNP(LPWSTR lpPath, SIZE_T MaxSize) = 0;
	virtual void SetAppNameNP(LPCWSTR lpPath) = 0;
	// ���s�A�v���E�I�v�V�����i�����L��j
	virtual void GetAppOption(LPWSTR lpPath, SIZE_T MaxSize) = 0;
	virtual void SetAppOption(LPCWSTR lpPath) = 0;
	// ���s�A�v���E�I�v�V�����i���������j
	virtual void GetAppOptionNP(LPWSTR lpPath, SIZE_T MaxSize) = 0;
	virtual void SetAppOptionNP(LPCWSTR lpPath) = 0;
	// �^��t�@�C���g���q
	virtual void GetFileExtension(LPWSTR lpExt, SIZE_T MaxSize) = 0;
	virtual void SetFileExtension(LPCWSTR lpExt) = 0;
	// �^��t�H���_
	virtual void GetRecordPath(LPWSTR lpPath, SIZE_T MaxSize) = 0;
	virtual void SetRecordPath(LPCWSTR lpPath) = 0;
	// �Ǐ��
	virtual StationInfo* CreateNewStationInfo() = 0;
	virtual bool GetStationInfo(const int, StationInfo *) = 0;
	virtual bool SetStationInfo(StationInfo *) = 0;
};

#define RockProperty	RockProperty_V100


// �f�o�C�X�X�e�[�^�X
//	 �X�e�[�^�X�̏���Rock�o�[��TvRock�̃��O�Ɏc����̂�
//	 TvRockDTV�A�C�R���ɕ\�������X�e�[�^�X�Ƃ͕ʂ̂��̂ł��B
class RockStatus_V100 : public DtvObject {
	__CLASS(RockStatus_V100)

public:
	// �X�e�[�^�X�E�X�}�[�g�J�[�h
	virtual DWORD GetStatusSmartCard() const = 0;
	virtual void  SetStatusSmartCard(const DWORD val) = 0;
	// �X�e�[�^�X�E�M�����x��
	virtual float GetStatusSignalLevel() const = 0;
	virtual void  SetStatusSignalLevel(const float val) = 0;
	// �X�e�[�^�X�E�r�b�g���[�g
	virtual DWORD GetStatusBitrate() const = 0;
	virtual void  SetStatusBitrate(const DWORD val) = 0;
	// �X�e�[�^�X�E�G���[�p�P�b�g
	virtual DWORD GetStatusErrorPacket() const = 0;
	virtual void  SetStatusErrorPacket(const DWORD val) = 0;
	// �X�e�[�^�X�E�����R��
	virtual DWORD GetStatusScramblingPacket() const = 0;
	virtual void  SetStatusScramblingPacket(const DWORD val) = 0;
};

#define RockStatus	RockStatus_V100


// TvRock����̗v�����󂯎��N���X
class RockRequest_V100 : public DtvObject {
	__CLASS(RockRequest_V100)

public:
	// �^�[�Q�b�g�I��
	virtual void PowerOff() = 0;
	// �`�����l���؂�ւ�
	virtual bool ChannelChange(const DWORD dwChannel, const DWORD dwService) = 0;
	// �^��J�n
	virtual bool RecordStart(LPWSTR lpFileName) = 0;
	// �N�C�b�N�^��J�n
	virtual bool QuickRecord(LPWSTR lpFileName) = 0;
	// �^��I��
	virtual bool RecordStop() = 0;
	// ��ʃL���v�`�� (Rock�o�[)
	virtual void ScreenCapture() = 0;
	// �I�v�V�����{�^�� (Rock�o�[)
	virtual void OnOption() = 0;
	// �r�f�I�E�B���h�E�n���h����Ԃ� (Rock�o�[)
	virtual HWND GetVideoHandle() = 0;
	// �^�[�Q�b�g���̎擾
	virtual void CollectSetting() = 0;
	// ���b�Z�[�W����
	virtual LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) = 0;
};

#define RockRequest	RockRequest_V100


// TvRockDTV���C�u����
class TvRockDTV_V100 : public DtvObject {
	__CLASS(TvRockDTV_V100)

public:
	// ������
	virtual	bool Initialize(const DWORD dwDevice, RockRequest *pRequest = NULL) = 0;
	// �I��
	virtual	void Dispose() = 0;
	// �X�g���[���������s���܂�
	virtual	bool ProcessTS(BYTE *pBuffer, const DWORD dwSize) = 0;
	// �X�g���[�����؂�ւ���ꂽ�Ƃ��ɌĂяo���܂�
	virtual bool OnStreamChange(const DWORD dwChannel, const DWORD dwService) = 0;
	// �����g�̎��Ԃ�Ԃ� (nowTime + diffTime)
	virtual void GetBroadCastTime(SYSTEMTIME *m_Time) = 0;

public:
	// �v���p�e�B�擾
	virtual RockProperty* GetProperty() = 0;
	// �X�e�[�^�X�擾
	virtual RockStatus* GetStatus() = 0;
	// �璷
	virtual void Verbose(const BOOL on) = 0;
	// TvRockDTV�̃E�B���h�E�n���h����Ԃ��܂�
	virtual HWND GetWindowHandle() = 0;

public:
	// Debug�p
	virtual void Trace(LPCWSTR lpszFormat, ...) = 0;
	virtual void Trace(LPCSTR lpszFormat, ...) = 0;
};

#define TvRockDTV	TvRockDTV_V100

}
