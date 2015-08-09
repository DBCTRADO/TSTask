#pragma once
#include <windows.h>

// TvRockDTVライブラリは以下のプログラムで読み込みます
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

// モジュール読み込み
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

// 基底
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


// 局情報
class StationInfo_V100 : public DtvObject {
	__CLASS(StationInfo_V100)

public:
	// 局名
	virtual void  GetName(LPWSTR lpPath, SIZE_T MaxSize) = 0;
	virtual void  SetName(LPCWSTR lpPath) = 0;
	// チャンネル
	virtual DWORD GetChannel() const = 0;
	virtual void  SetChannel(const DWORD) = 0;
	// サービス
	virtual DWORD GetServiceID() const = 0;
	virtual void  SetServiceID(const DWORD) = 0;
};

#define StationInfo		StationInfo_V100


// プロパティ
class RockProperty_V100 : public DtvObject {
	__CLASS(RockProperty_V100)

// チューナー設定情報
public:
	// 内容を更新してTvRockへ伝える
	virtual void Notify() = 0;
	// 地上波
	virtual bool UseTS() const = 0;
	// BS
	virtual bool UseBS() const = 0;
	// CS
	virtual bool UseCS() const = 0;
	// アプリケーションフォルダ
	virtual void GetAppPath(LPWSTR lpPath, SIZE_T MaxSize) = 0;
	virtual void SetAppPath(LPCWSTR lpPath) = 0;
	// 実行アプリ名（視聴有り）
	virtual void GetAppName(LPWSTR lpPath, SIZE_T MaxSize) = 0;
	virtual void SetAppName(LPCWSTR lpPath) = 0;
	// 実行アプリ名（視聴無し）
	virtual void GetAppNameNP(LPWSTR lpPath, SIZE_T MaxSize) = 0;
	virtual void SetAppNameNP(LPCWSTR lpPath) = 0;
	// 実行アプリ・オプション（視聴有り）
	virtual void GetAppOption(LPWSTR lpPath, SIZE_T MaxSize) = 0;
	virtual void SetAppOption(LPCWSTR lpPath) = 0;
	// 実行アプリ・オプション（視聴無し）
	virtual void GetAppOptionNP(LPWSTR lpPath, SIZE_T MaxSize) = 0;
	virtual void SetAppOptionNP(LPCWSTR lpPath) = 0;
	// 録画ファイル拡張子
	virtual void GetFileExtension(LPWSTR lpExt, SIZE_T MaxSize) = 0;
	virtual void SetFileExtension(LPCWSTR lpExt) = 0;
	// 録画フォルダ
	virtual void GetRecordPath(LPWSTR lpPath, SIZE_T MaxSize) = 0;
	virtual void SetRecordPath(LPCWSTR lpPath) = 0;
	// 局情報
	virtual StationInfo* CreateNewStationInfo() = 0;
	virtual bool GetStationInfo(const int, StationInfo *) = 0;
	virtual bool SetStationInfo(StationInfo *) = 0;
};

#define RockProperty	RockProperty_V100


// デバイスステータス
//	 ステータスの情報はRockバーやTvRockのログに残るもので
//	 TvRockDTVアイコンに表示されるステータスとは別のものです。
class RockStatus_V100 : public DtvObject {
	__CLASS(RockStatus_V100)

public:
	// ステータス・スマートカード
	virtual DWORD GetStatusSmartCard() const = 0;
	virtual void  SetStatusSmartCard(const DWORD val) = 0;
	// ステータス・信号レベル
	virtual float GetStatusSignalLevel() const = 0;
	virtual void  SetStatusSignalLevel(const float val) = 0;
	// ステータス・ビットレート
	virtual DWORD GetStatusBitrate() const = 0;
	virtual void  SetStatusBitrate(const DWORD val) = 0;
	// ステータス・エラーパケット
	virtual DWORD GetStatusErrorPacket() const = 0;
	virtual void  SetStatusErrorPacket(const DWORD val) = 0;
	// ステータス・複合漏れ
	virtual DWORD GetStatusScramblingPacket() const = 0;
	virtual void  SetStatusScramblingPacket(const DWORD val) = 0;
};

#define RockStatus	RockStatus_V100


// TvRockからの要求を受け取るクラス
class RockRequest_V100 : public DtvObject {
	__CLASS(RockRequest_V100)

public:
	// ターゲット終了
	virtual void PowerOff() = 0;
	// チャンネル切り替え
	virtual bool ChannelChange(const DWORD dwChannel, const DWORD dwService) = 0;
	// 録画開始
	virtual bool RecordStart(LPWSTR lpFileName) = 0;
	// クイック録画開始
	virtual bool QuickRecord(LPWSTR lpFileName) = 0;
	// 録画終了
	virtual bool RecordStop() = 0;
	// 画面キャプチャ (Rockバー)
	virtual void ScreenCapture() = 0;
	// オプションボタン (Rockバー)
	virtual void OnOption() = 0;
	// ビデオウィンドウハンドルを返す (Rockバー)
	virtual HWND GetVideoHandle() = 0;
	// ターゲット情報の取得
	virtual void CollectSetting() = 0;
	// メッセージ処理
	virtual LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) = 0;
};

#define RockRequest	RockRequest_V100


// TvRockDTVライブラリ
class TvRockDTV_V100 : public DtvObject {
	__CLASS(TvRockDTV_V100)

public:
	// 初期化
	virtual	bool Initialize(const DWORD dwDevice, RockRequest *pRequest = NULL) = 0;
	// 終了
	virtual	void Dispose() = 0;
	// ストリーム処理を行います
	virtual	bool ProcessTS(BYTE *pBuffer, const DWORD dwSize) = 0;
	// ストリームが切り替えられたときに呼び出します
	virtual bool OnStreamChange(const DWORD dwChannel, const DWORD dwService) = 0;
	// 放送波の時間を返す (nowTime + diffTime)
	virtual void GetBroadCastTime(SYSTEMTIME *m_Time) = 0;

public:
	// プロパティ取得
	virtual RockProperty* GetProperty() = 0;
	// ステータス取得
	virtual RockStatus* GetStatus() = 0;
	// 冗長
	virtual void Verbose(const BOOL on) = 0;
	// TvRockDTVのウィンドウハンドルを返します
	virtual HWND GetWindowHandle() = 0;

public:
	// Debug用
	virtual void Trace(LPCWSTR lpszFormat, ...) = 0;
	virtual void Trace(LPCSTR lpszFormat, ...) = 0;
};

#define TvRockDTV	TvRockDTV_V100

}
