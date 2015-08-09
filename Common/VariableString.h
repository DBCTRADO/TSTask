#ifndef TSTASK_VARIABLE_STRING_H
#define TSTASK_VARIABLE_STRING_H


namespace TSTask
{

	class CVariableStringMap abstract
	{
	public:
		virtual bool BeginFormat() { return true; }
		virtual void EndFormat() {}
		virtual bool GetString(LPCWSTR pszKeyword,String *pString) = 0;
		virtual bool IsFileName() const { return false; }

	protected:
		bool GetTimeString(LPCWSTR pszKeyword,const SYSTEMTIME &Time,String *pString) const;
	};

	bool FormatVariableString(String *pString,CVariableStringMap *pVariableMap,LPCWSTR pszFormat);

	class CEventVariableStringMap : public CVariableStringMap
	{
	public:
		struct EventInfo
		{
			ChannelInfo Channel;
			TSTask::EventInfo Event;
		};

		struct ParameterInfo
		{
			LPCWSTR pszParameter;
			LPCWSTR pszText;
		};

		CEventVariableStringMap(const EventInfo &Info);
		bool BeginFormat() override;
		bool GetString(LPCWSTR pszKeyword,String *pString) override;
		bool IsFileName() const override { return true; }

		static bool GetParameterInfo(int Index,ParameterInfo *pInfo);
		static bool GetSampleEventInfo(EventInfo *pInfo);

	private:
		static void GetEventTitle(const String &EventName,String *pTitle);

		const EventInfo &m_EventInfo;
		SYSTEMTIME m_CurrentTime;
	};

}


#endif
