#include "stdafx.h"
#include "TSTask.h"
#include "TvRock.h"
#include "../Common/DebugDef.h"

using namespace TvRock;


#ifdef _WIN64
namespace TvRock
{
	static HINSTANCE LoadDTVModule64()
	{
		WCHAR path[MAX_PATH];
		memset(path,0,sizeof(path));
		if (__read_software_registry(TEXT("SOFTWARE\\") TRDTV_TVROCK_NAME,TEXT("DOCUMENT"),path,MAX_PATH*sizeof(WCHAR)))
			return NULL;
		wcscat_s(path,MAX_PATH,L"\\TvRockDTV.x64.dll");
		return LoadLibrary(path);
	}
}
#endif


namespace TSTask
{

// �n�f�W�̕����`�����l���͈̔�
#define TERRESTRIAL_CHANNEL_FIRST	13
#define TERRESTRIAL_CHANNEL_LAST	63

	// �n�f�W��TSID��?
	inline bool IsTerrestrialTSID(WORD TSID) {
		return (TSID & 0xFF00) == 0x7F00 || (TSID & 0xFF00) == 0x7E00;
	}
	// BS��NetworkID��?
	inline bool IsBsNetworkID(WORD NetworkID) {
		return NetworkID == 4;
	}
	// BS��TSID��?
	inline bool IsBsTSID(WORD TSID) {
		return (TSID & 0xF000) == 0x4000;
	}
	// CS��NetworkID��?
	inline bool IsCsNetworkID(WORD NetworkID) {
		return NetworkID >= 6 && NetworkID <= 10;
	}
	// CS��TSID��?
	inline bool IsCsTSID(WORD TSID) {
		return !IsBsTSID(TSID) && !IsTerrestrialTSID(TSID);
	}

	struct FreqInfo {
		WORD Frequency;
		WORD TSID;

		DWORD GetTvRockChannel() const
		{
			return ((DWORD)Frequency << 16) | (DWORD)TSID;
		}
	};

	// ���g���������Ɉ�v���Ȃ����̂�����̂́ATvRock�̎d�l��Friio�R���̂���
	static const FreqInfo DefaultBSChannelList[] = {
		{11727,	0x4010},	// BS1/TS0 BS����
		{11727,	0x4011},	// BS1/TS1 BS-TBS
		{11766,	0x4030},	// BS3/TS0 WOWOW�v���C��
		{11766,	0x4031},	// BS3/TS1 BS�W���p��
		{11804,	0x4450},	// BS5/TS0 WOWOW���C�u
		{11804,	0x4451},	// BS5/TS1 WOWOW�V�l�}
		{11843,	0x4470},	// BS7/TS0 �X�^�[�`�����l��2/3
		{11843,	0x4671},	// BS7/TS1 BS�A�j�}�b�N�X
		{11843,	0x4672},	// BS7/TS2 �f�B�Y�j�[�E�`�����l��
		{11880,	0x4090},	// BS9/TS0 BS11
		{11880,	0x4091},	// BS9/TS1 �X�^�[�`�����l��1
		{11880,	0x4092},	// BS9/TS2 TwellV
		{11919,	0x46B0},	// BS11/TS0 FOX bs238
		{11919,	0x46B1},	// BS11/TS1 BS�X�J�p�[�I
		{11919,	0x46B2},	// BS11/TS2 ������w
		{11958,	0x40D0},	// BS13/TS0 BS���e��
		{11958,	0x40D1},	// BS13/TS1 BS�t�W
		{11996,	0x40F1},	// BS15/TS1 NHK BS1
		{11996,	0x40F2},	// BS15/TS2 NHK BS�v���~�A��
		{12034,	0x4310},	// BS17/TS1 �n�f�W��΍�
		{12034,	0x4311},	// BS17/TS2 �n�f�W��΍�
		{12073,	0x4730},	// BS19/TS0 �O���[���`�����l��
		{12073,	0x4731},	// BS19/TS1 J SPORTS 1
		{12073,	0x4732},	// BS19/TS2 J SPORTS 2
		{12111,	0x4750},	// BS21/TS0 IMAGICA BS
		{12111,	0x4751},	// BS21/TS1 J SPORTS 3
		{12111,	0x4752},	// BS21/TS2 J SPORTS 4 
		{12149,	0x4770},	// BS23/TS0 BS�ނ�r�W����
		{12149,	0x4771},	// BS23/TS1 ���{�f����`�����l��
		{12149,	0x4772},	// BS23/TS2 D-Life
	};

	static const FreqInfo DefaultCSChannelList[] = {
		{12291,	0x6020},	// ND2 110CS #1
		{12331,	0x7040},	// ND4 110CS #2
		{12371,	0x7060},	// ND6 110CS #3
		{12411,	0x6080},	// ND8 110CS #4
		{12451,	0x60A0},	// ND10 110CS #5
		{12491,	0x70C0},	// ND12 110CS #6
		{12531,	0x70E0},	// ND14 110CS #7
		{12571,	0x7100},	// ND16 110CS #8
		{12611,	0x7120},	// ND18 110CS #9
		{12651,	0x7140},	// ND20 110CS #10
		{12691,	0x7160},	// ND22 110CS #11
		{12731,	0x7180},	// ND24 110CS #12
	};


	// �`�����l��
	class CChannelSearch
	{
		typedef std::vector<FreqInfo> ChannelList;

		struct FreqRange {
			WORD Min, Max;
			FreqRange() : Min(0), Max(0) {}
		};

		CTSTaskCore &m_Core;
		bool m_fInitialized;
		int m_TerrestrialIndex;
		int m_TerrestrialIndex2;
		int m_BSIndex;
		int m_CSIndex;
		bool m_fTerraIndexSpecified;
		bool m_fTerraIndex2Specified;
		bool m_fBSIndexSpecified;
		bool m_fCSIndexSpecified;
		ChannelList m_BSChannelList;
		ChannelList m_CSChannelList;
		FreqRange m_BSFrequencyRange;
		FreqRange m_CSFrequencyRange;
		CTuningSpaceList m_TuningSpaceList;

		// �`���[�j���O��Ԃ̃C���f�b�N�X�̎擾
		void GetSpaceIndex()
		{
			if (m_fInitialized)
				return;

			m_Core.GetScannedChannelList(&m_TuningSpaceList);

			bool fFound;
			const CChannelInfo *pChannelInfo;

			// �n�f�W�̃`���[�j���O��Ԃ̎擾
			if (m_TerrestrialIndex >= 0) {
				fFound = true;
				if (!m_fTerraIndexSpecified
						&& ((pChannelInfo = m_TuningSpaceList.GetChannelInfo(m_TerrestrialIndex, 0)) == nullptr
							|| !IsTerrestrialTSID(pChannelInfo->GetTransportStreamID()))) {
					fFound = false;
					for (size_t i = 0; i < m_TuningSpaceList.GetSpaceCount(); i++) {
						pChannelInfo = m_TuningSpaceList.GetChannelInfo(i, 0);
						if (pChannelInfo!=nullptr
								&& IsTerrestrialTSID(pChannelInfo->GetTransportStreamID())) {
							m_TerrestrialIndex = (int)i;
							fFound = true;
							break;
						}
					}
				}
				OutLog(LOG_INFO,
					   fFound ? L"�n�f�W�̃`���[�j���O��Ԃ��擾���܂����B(%d)":
					            L"�n�f�W�̃`���[�j���O��Ԃ��擾�ł��Ȃ����߁A%d �ɉ��肵�܂����B",
					   m_TerrestrialIndex);
				if (fFound && !m_fTerraIndex2Specified
						&& (pChannelInfo = m_TuningSpaceList.GetChannelInfo(m_TerrestrialIndex + 1, 0)) != nullptr
						&& IsTerrestrialTSID(pChannelInfo->GetTransportStreamID())) {
					m_TerrestrialIndex2 = m_TerrestrialIndex + 1;
				}
				if (m_TerrestrialIndex2 >= 0) {
					OutLog(LOG_INFO, L"�n�f�W2�̃`���[�j���O��Ԃ��擾���܂����B(%d)", m_TerrestrialIndex2);
				}
			}

			// BS�̃`���[�j���O��Ԃ̎擾
			if (m_BSIndex >= 0) {
				fFound = true;
				if (!m_fBSIndexSpecified
						&& ((pChannelInfo = m_TuningSpaceList.GetChannelInfo(m_BSIndex, 0)) == nullptr
							|| (pChannelInfo->GetNetworkID() != 0
								&& !IsBsNetworkID(pChannelInfo->GetNetworkID()))
							|| !IsBsTSID(pChannelInfo->GetTransportStreamID()))) {
					// �܂��ŏ��̃`�����l���̂ݒ��ׂ�
					fFound = false;
					for (size_t i = 0; i < m_TuningSpaceList.GetSpaceCount(); i++) {
						pChannelInfo = m_TuningSpaceList.GetChannelInfo(i, 0);
						if (pChannelInfo!=nullptr
								&& (IsBsNetworkID(pChannelInfo->GetNetworkID())
									|| IsBsTSID(pChannelInfo->GetTransportStreamID()))) {
							m_BSIndex = (int)i;
							fFound = true;
							break;
						}
					}
					// ���t����Ȃ��ꍇ�A�S�Ẵ`�����l���𒲂ׂ�
					if (!fFound) {
						for (size_t Space = 0; !fFound && Space < m_TuningSpaceList.GetSpaceCount(); Space++) {
							for (size_t i = 1; (pChannelInfo = m_TuningSpaceList.GetChannelInfo(Space, i)) != nullptr; i++) {
								if (IsBsNetworkID(pChannelInfo->GetNetworkID())
										|| IsBsTSID(pChannelInfo->GetTransportStreamID())) {
									m_BSIndex = (int)Space;
									fFound = true;
									break;
								}
							}
						}
					}
				}
				OutLog(LOG_INFO,
					   fFound ? L"BS�̃`���[�j���O��Ԃ��擾���܂����B(%d)":
					            L"BS�̃`���[�j���O��Ԃ��擾�ł��Ȃ����߁A%d �ɉ��肵�܂����B",
					   m_BSIndex);
			}

			// CS�̃`���[�j���O��Ԃ̎擾
			if (m_CSIndex >= 0) {
				fFound = true;
				if (!m_fCSIndexSpecified
						&& ((pChannelInfo = m_TuningSpaceList.GetChannelInfo(m_CSIndex, 0)) == nullptr
							|| (pChannelInfo->GetNetworkID() != 0
								&& !IsCsNetworkID(pChannelInfo->GetNetworkID()))
							|| !IsCsTSID(pChannelInfo->GetTransportStreamID()))) {
					// �܂��ŏ��̃`�����l���̂ݒ��ׂ�
					fFound = false;
					for (size_t i = 0; i < m_TuningSpaceList.GetSpaceCount(); i++) {
						pChannelInfo = m_TuningSpaceList.GetChannelInfo(i, 0);
						if (pChannelInfo != nullptr
								&& (IsCsNetworkID(pChannelInfo->GetNetworkID())
									|| IsCsTSID(pChannelInfo->GetTransportStreamID()))) {
							m_CSIndex = (int)i;
							fFound = true;
							break;
						}
					}
					// ���t����Ȃ��ꍇ�A�S�Ẵ`�����l���𒲂ׂ�
					if (!fFound) {
						for (size_t Space = 0; !fFound && Space < m_TuningSpaceList.GetSpaceCount(); Space++) {
							for (size_t i = 1; (pChannelInfo = m_TuningSpaceList.GetChannelInfo(Space, i)) != nullptr; i++) {
								if (IsCsNetworkID(pChannelInfo->GetNetworkID())
										|| IsCsTSID(pChannelInfo->GetTransportStreamID())) {
									m_CSIndex = (int)Space;
									fFound = true;
									break;
								}
							}
						}
					}
				}
				OutLog(LOG_INFO,
					   fFound ? L"CS�̃`���[�j���O��Ԃ��擾���܂����B(%d)":
					            L"CS�̃`���[�j���O��Ԃ��擾�ł��Ȃ����߁A%d �ɉ��肵�܂����B",
					   m_CSIndex);
			}

			m_fInitialized = true;
		}

		// ���g���ݒ�̓ǂݍ���
		bool LoadFrequencySettings(CSettings *pSettings, LPCTSTR pszSectionName, ChannelList *pChannelList)
		{
			CSettings::EntryList Entries;

			if (pSettings==nullptr
					|| !pSettings->SetSection(pszSectionName)
					|| !pSettings->GetEntries(&Entries)
					|| Entries.empty())
				return false;

			pChannelList->clear();

			for (auto i=Entries.begin();i!=Entries.end();i++) {
				FreqInfo Info;

				Info.TSID = (WORD)std::wcstoul(i->Name.c_str(), nullptr, 0);
				Info.Frequency = (WORD)std::wcstoul(i->Value.c_str(), nullptr, 0);
				if (Info.TSID != 0 && Info.Frequency != 0) {
					pChannelList->push_back(Info);
					OutLog(LOG_VERBOSE,L"���g���}�b�v[%s] : 0x%04x = %d MHz (%u)",
						   pszSectionName, Info.TSID, Info.Frequency, Info.GetTvRockChannel());
				} else {
					OutLog(LOG_ERROR,L"���g���̐ݒ肪�ُ�ł��B[%s] %s=%s",
						   pszSectionName, i->Name.c_str(), i->Value.c_str());
				}
			}

			OutLog(LOG_INFO,L"���g���̐ݒ�(%s)��%Iu�ǂݍ��݂܂����B",
				   pszSectionName, pChannelList->size());

			return !pChannelList->empty();
		}

		// ���g���͈̔͂��擾
		void GetFrequencyRange(const ChannelList &ChannelList, FreqRange *pRange)
		{
			WORD Min = 0xFFFF, Max = 0;

			for (size_t i = 0; i < ChannelList.size(); i++) {
				const WORD Frequency = ChannelList[i].Frequency;
				if (Frequency < Min)
					Min = Frequency;
				if (Frequency > Max)
					Max = Frequency;
			}
			pRange->Min = Min;
			pRange->Max = Max;
		}

		// �T�[�r�XID�̈�v����`�����l����T��
		int FindChannelByServiceID(int Space, WORD ServiceID)
		{
			const CChannelList *pChannelList=m_TuningSpaceList.GetChannelList(Space);
			if (pChannelList==nullptr)
				return -1;
			return pChannelList->FindByIDs(0, 0, ServiceID);
		}

		// TSID��ServiceID�̈�v����`�����l����T��
		int FindChannelByTSIDAndServiceID(int Space, WORD TSID, WORD ServiceID)
		{
			const CChannelList *pChannelList=m_TuningSpaceList.GetChannelList(Space);
			if (pChannelList==nullptr)
				return -1;
			return pChannelList->FindByIDs(0, TSID, ServiceID);
		}

	public:
		CChannelSearch(CTSTaskCore &Core)
			: m_Core(Core)
			, m_fInitialized(false)
			, m_TerrestrialIndex(-1)
			, m_TerrestrialIndex2(-1)
			, m_BSIndex(-1)
			, m_CSIndex(-1)
			, m_fTerraIndexSpecified(false)
			, m_fTerraIndex2Specified(false)
			, m_fBSIndexSpecified(false)
			, m_fCSIndexSpecified(false)
		{
		}

		void Initialize(CSettings *pSettings,LPCWSTR pszDevice)
		{
			// �ݒ�ǂݍ���
			String Keyword;

			if (pSettings!=nullptr) {
				StringUtility::Format(Keyword,L"TvRock.%s.TerrestrialIndex",pszDevice);
				m_fTerraIndexSpecified=
					pSettings->Read(Keyword.c_str(),&m_TerrestrialIndex) && m_TerrestrialIndex>=0;
				if (m_fTerraIndexSpecified) {
					StringUtility::Format(Keyword,L"TvRock.%s.TerrestrialIndex2",pszDevice);
					m_fTerraIndex2Specified=
						pSettings->Read(Keyword.c_str(),&m_TerrestrialIndex2) && m_TerrestrialIndex2>=0;
				} else {
					m_fTerraIndex2Specified=false;
				}
				StringUtility::Format(Keyword,L"TvRock.%s.BSIndex",pszDevice);
				m_fBSIndexSpecified=pSettings->Read(Keyword.c_str(),&m_BSIndex) && m_BSIndex>=0;
				StringUtility::Format(Keyword,L"TvRock.%s.CSIndex",pszDevice);
				m_fCSIndexSpecified=pSettings->Read(Keyword.c_str(),&m_CSIndex) && m_CSIndex>=0;
			} else {
				m_fTerraIndexSpecified=false;
				m_fTerraIndex2Specified=false;
				m_fBSIndexSpecified=false;
				m_fCSIndexSpecified=false;
			}

			// BS/CS�̎��g���ݒ�̓ǂݍ���
			if (pSettings==nullptr
					|| !LoadFrequencySettings(pSettings,L"BSFrequency",&m_BSChannelList)) {
				m_BSChannelList.resize(_countof(DefaultBSChannelList));
				for (size_t i = 0; i < _countof(DefaultBSChannelList) ; i++)
					m_BSChannelList[i] = DefaultBSChannelList[i];
			}
			GetFrequencyRange(m_BSChannelList, &m_BSFrequencyRange);
			if (pSettings==nullptr
					|| !LoadFrequencySettings(pSettings,L"CSFrequency",&m_CSChannelList)) {
				m_CSChannelList.resize(_countof(DefaultCSChannelList));
				for (size_t i = 0; i < _countof(DefaultCSChannelList) ; i++)
					m_CSChannelList[i] = DefaultCSChannelList[i];
			}
			GetFrequencyRange(m_CSChannelList, &m_CSFrequencyRange);
		}

		void SetProperty(RockProperty *pProp)
		{
			int i = 0;

			// �`���[�j���O��Ԃ̃C���f�b�N�X�����蓖�Ă�(���̒i�K�ł͂܂���)
			if (pProp->UseTS()) {
				if (!m_fTerraIndexSpecified)
					m_TerrestrialIndex = i++;
				else
					i = m_TerrestrialIndex + 1;
			} else {
				m_TerrestrialIndex = -1;
			}

			if (pProp->UseBS()) {
				if (!m_fBSIndexSpecified)
					m_BSIndex = i++;
				else
					i = m_BSIndex + 1;
			} else {
				m_BSIndex = -1;
			}

			if (pProp->UseCS()) {
				if (!m_fCSIndexSpecified)
					m_CSIndex = i++;
			} else {
				m_CSIndex = -1;
			}
		}

		// TvRock����̃`�����l���ύX�v���̏���
		bool SetChannel(const DWORD dwChannel, const DWORD dwService)
		{
			if (!m_fInitialized)
				GetSpaceIndex();

			const CChannelInfo *pChannelInfo;

			if (dwChannel <= TERRESTRIAL_CHANNEL_LAST) {
				// �n��
				int SpaceIndex[2];
				int NumSpaces = 0;

				if (m_TerrestrialIndex >= 0) {
					SpaceIndex[NumSpaces++] = m_TerrestrialIndex;
					if (m_TerrestrialIndex2 >= 0 && m_TerrestrialIndex2 != m_TerrestrialIndex)
						SpaceIndex[NumSpaces++] = m_TerrestrialIndex2;
				}

				// �`�����l���̃C���f�b�N�X/�T�[�r�XID���Ɉ�v����`�����l����T��
				for (int i = 0; i < NumSpaces; i++) {
					const int Space = SpaceIndex[i];
					const CChannelList *pChannelList=m_TuningSpaceList.GetChannelList(Space);
					if (pChannelList!=nullptr) {
						for (int j = 0; (pChannelInfo = pChannelList->GetChannelInfo(j)) != nullptr; j++) {
							if ((DWORD)pChannelInfo->GetChannel() == dwChannel - TERRESTRIAL_CHANNEL_FIRST
									&& (dwService == 0 || pChannelInfo->GetServiceID() == dwService))
								return m_Core.SetChannelByScanned(Space, j);
						}
					}
				}
				// �T�[�r�XID�̈�v����`�����l����T��
				if (dwService != 0) {
					for (int i = 0; i < NumSpaces; i++) {
						const int Space = SpaceIndex[i];
						const int Channel = FindChannelByServiceID(Space, (WORD)dwService);
						if (Channel >= 0)
							return m_Core.SetChannelByScanned(Space, Channel);
					}
				}
				// �`�����l���̃C���f�b�N�X����v����`�����l����T��
				for (int i = 0; i < NumSpaces; i++) {
					const int Space = SpaceIndex[i];
					const CChannelList *pChannelList=m_TuningSpaceList.GetChannelList(Space);
					if (pChannelList!=nullptr) {
						for (int j = 0; (pChannelInfo = pChannelList->GetChannelInfo(j)) != nullptr; j++) {
							if ((DWORD)pChannelInfo->GetChannel() == dwChannel - TERRESTRIAL_CHANNEL_FIRST)
								return m_Core.SetChannelByScanned(Space, j, (WORD)dwService);
						}
					}
				}
			} else if (m_BSIndex >= 0 || m_CSIndex >= 0) {
				// BS/CS
				const WORD Frequency = (WORD)(dwChannel >> 16), TSID = (WORD)(dwChannel & 0xFFFF);

				if (TSID != 0) {
					int Space, Channel;

					if (m_CSIndex < 0 || IsBsTSID(TSID)) {
						Space = m_BSIndex;
					} else {
						Space = m_CSIndex;
					}
					if (dwService != 0) {
						// TSID�ƃT�[�r�XID����v����`�����l����T��
						Channel = FindChannelByTSIDAndServiceID(Space, TSID, (WORD)dwService);
						if (Channel >= 0)
							return m_Core.SetChannelByScanned(Space, Channel);
						OutLog(LOG_WARNING,L"�w�肳�ꂽTSID�ƃT�[�r�XID�Ɉ�v����`�����l��������܂���B");
					}
					// TSID�����ŒT���Ă݂�
					Channel = FindChannelByTSIDAndServiceID(Space, TSID, 0);
					if (Channel >= 0)
						return m_Core.SetChannelByScanned(Space, Channel, (WORD)dwService);
				}

				if (dwService != 0) {
					enum {
						SPACE_UNKNOWN,
						SPACE_BS,
						SPACE_CS
					} SpaceType = SPACE_UNKNOWN;

					// ���g���ƃT�[�r�XID����v����`�����l����T��
					if (m_BSIndex >= 0) {
						for (size_t i = 0; i < m_BSChannelList.size(); i++) {
							if (m_BSChannelList[i].Frequency == Frequency) {
								const int Channel = FindChannelByTSIDAndServiceID(
									m_BSIndex, m_BSChannelList[i].TSID, (WORD)dwService);
								if (Channel >= 0)
									return m_Core.SetChannelByScanned(m_BSIndex, Channel);
								SpaceType = SPACE_BS;
							}
						}
					}
					if (m_CSIndex >= 0) {
						for (size_t i = 0; i < m_CSChannelList.size(); i++) {
							if (m_CSChannelList[i].Frequency == Frequency) {
								const int Channel = FindChannelByTSIDAndServiceID(
									m_CSIndex, m_CSChannelList[i].TSID, (WORD)dwService);
								if (Channel >= 0)
									return m_Core.SetChannelByScanned(m_CSIndex, Channel);
								SpaceType = SPACE_CS;
							}
						}
					}
					OutLog(LOG_WARNING,L"�w�肳�ꂽ���g���ƃT�[�r�XID�Ɉ�v����`�����l��������܂���B");
					if (SpaceType == SPACE_UNKNOWN) {
						if (m_BSIndex >= 0
								&& Frequency >= m_BSFrequencyRange.Min
								&& Frequency <= m_BSFrequencyRange.Max)
							SpaceType = SPACE_BS;
						else if (m_CSIndex >= 0
								&& Frequency >= m_CSFrequencyRange.Min
								&& Frequency <= m_CSFrequencyRange.Max)
							SpaceType = SPACE_CS;
					}
					// �T�[�r�XID�����ŒT���Ă݂�
					int Space = -1;
					if (SpaceType != SPACE_UNKNOWN) {
						if (SpaceType == SPACE_BS)
							Space = m_BSIndex;
						else
							Space = m_CSIndex;
						if (m_BSIndex == m_CSIndex) {
							for (int i = 0; (pChannelInfo = m_TuningSpaceList.GetChannelInfo(Space, i)) != nullptr; i++) {
								if (pChannelInfo->GetServiceID() == dwService
										&& ((SpaceType == SPACE_BS
											&& (IsBsNetworkID(pChannelInfo->GetNetworkID())
												|| IsBsTSID(pChannelInfo->GetTransportStreamID())))
										|| (SpaceType == SPACE_CS
											&& (IsCsNetworkID(pChannelInfo->GetNetworkID())
												|| IsCsTSID(pChannelInfo->GetTransportStreamID())))))
									return m_Core.SetChannelByScanned(Space, i);
							}
						} else {
							const int Channel = FindChannelByServiceID(Space, (WORD)dwService);
							if (Channel >= 0)
								return m_Core.SetChannelByScanned(Space, Channel);
						}
					}
					if (m_BSIndex >= 0 && m_BSIndex != Space) {
						const int Channel = FindChannelByServiceID(m_BSIndex, (WORD)dwService);
						if (Channel >= 0)
							return m_Core.SetChannelByScanned(m_BSIndex, Channel);
					}
					if (m_CSIndex >= 0 && m_CSIndex != Space) {
						const int Channel = FindChannelByServiceID(m_CSIndex, (WORD)dwService);
						if (Channel >= 0)
							return m_Core.SetChannelByScanned(m_CSIndex, Channel);
					}
				}
			}

			OutLog(LOG_ERROR,L"�Y������`�����l����������܂���B");
			return false;
		}

		// ���݂̃`�����l����TvRock�̌`���Ŏ擾����
		DWORD GetChannel()
		{
			if (!m_fInitialized)
				GetSpaceIndex();

			ChannelInfo ChannelInfo;
			if (!m_Core.GetChannel(&ChannelInfo) || ChannelInfo.Channel<0)
				return 0;
			if (ChannelInfo.Space == m_TerrestrialIndex
					|| ChannelInfo.Space == m_TerrestrialIndex2) {
				return ChannelInfo.Channel + TERRESTRIAL_CHANNEL_FIRST;
			}
			for (size_t i = 0; i < m_BSChannelList.size(); i++) {
				if (m_BSChannelList[i].TSID == ChannelInfo.TransportStreamID)
					return m_BSChannelList[i].GetTvRockChannel();
			}
			for (size_t i = 0; i < m_CSChannelList.size(); i++) {
				if (m_CSChannelList[i].TSID == ChannelInfo.TransportStreamID)
					return m_CSChannelList[i].GetTvRockChannel();
			}
			return 0;
		}

		int GetFrequencyByTSID(WORD TransportStreamID)
		{
			for (size_t i = 0; i < m_BSChannelList.size(); i++) {
				if (m_BSChannelList[i].TSID == TransportStreamID)
					return m_BSChannelList[i].Frequency;
			}
			for (size_t i = 0; i < m_CSChannelList.size(); i++) {
				if (m_CSChannelList[i].TSID == TransportStreamID)
					return m_CSChannelList[i].Frequency;
			}
			return 0;
		}
	};


	// TvRock����̗v�����󂯎��N���X
	class CTvRockRequest : public RockRequest
	{
		CTSTaskCore &m_Core;
		CTSTaskAppCore &m_AppCore;
		CChannelSearch &m_ChannelSearch;

	public:
		CTvRockRequest(CTSTaskCore &Core,CTSTaskAppCore &AppCore,CChannelSearch &ChannelSearch)
			: m_Core(Core)
			, m_AppCore(AppCore)
			, m_ChannelSearch(ChannelSearch)
		{
		}

		// �A�v���I���̗v��
		void PowerOff() override
		{
			OutLog(LOG_INFO,L"TvRock����I���v�����󂯂܂����B");

			m_AppCore.Quit();
		}

		// �`�����l���ύX�̗v��
		bool ChannelChange(const DWORD dwChannel,const DWORD dwService) override
		{
			OutLog(LOG_INFO,L"TvRock����`�����l���ύX�v�����󂯂܂����B(Ch %lu / Sv %lu)",dwChannel,dwService);

			return m_ChannelSearch.SetChannel(dwChannel,dwService);
		}

		// �^��J�n�̗v��
		bool RecordStart(LPWSTR lpFileName) override
		{
			OutLog(LOG_INFO,L"TvRock����^��J�n�v�����󂯂܂����B(%s)",lpFileName);

			RecordingSettings Settings;
			m_AppCore.GetCurSettings().Recording.GetRecordingSettings(&Settings);

			String DefaultDir;
			PathUtility::Split(String(lpFileName),&DefaultDir,&Settings.FileName);
			Settings.Directories.clear();
			Settings.Directories.push_back(DefaultDir);

			std::vector<String> Directories;
			if (m_AppCore.GetCurSettings().Recording.GetDirectories(&Directories)) {
				String Dir;
				PathUtility::AppendDelimiter(&DefaultDir);
				for (const auto &e:Directories) {
					Dir=e;
					PathUtility::AppendDelimiter(&Dir);
					if (StringUtility::CompareNoCase(Dir,DefaultDir)!=0)
						Settings.Directories.push_back(e);
				}
			}

			return m_Core.StartRecording(Settings);
		}

		// �N�C�b�N�^��J�n�̗v�� (Rock�o�[����̒��ژ^��̗v��)
		bool QuickRecord(LPWSTR lpFileName) override
		{
			return RecordStart(lpFileName);
		}

		// �^���~�̗v��
		bool RecordStop() override
		{
			OutLog(LOG_INFO,L"TvRock����^���~�v�����󂯂܂����B");

			return m_Core.StopRecording();
		}

		// �Î~��ۑ��̗v��
		void ScreenCapture() override
		{
			OutLog(LOG_VERBOSE,L"TvRock����Î~��ۑ��v�����󂯂܂����B");
		}

		// �I�v�V�����{�^���̗v��
		void OnOption() override
		{
			OutLog(LOG_VERBOSE,L"TvRock����I�v�V�����{�^���̗v�����󂯂܂����B");
		}

		// �r�f�I�E�B���h�E�n���h���̗v�� (Rock�o�[�E�^�b�v�E�E�B���h�E�T�C�Y�ύX)
		HWND GetVideoHandle() override
		{
			return m_AppCore.GetWindowHandle();
		}

		// �^�[�Q�b�g���̎擾
		void CollectSetting() override
		{
		}

		// ���b�Z�[�W����
		LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override
		{
			return 0;
		}
	};


	CTvRockDTVTarget::CTvRockDTVTarget(CTSTaskAppCore &AppCore)
		: m_AppCore(AppCore)
		, m_Core(AppCore.GetCore())
		, m_hDTVModule(nullptr)
		, m_pTvRock(nullptr)
		, m_pRequest(nullptr)
		, m_pChannel(nullptr)
		, m_DeviceID(-1)
		, m_StreamQueueMaxBlocks(MAX_QUEUE_BLOCKS_DEFAULT)
		, m_StreamSendBufferLength(DEFAULT_SEND_BUFFER_LENGTH)
	{
	}

	CTvRockDTVTarget::~CTvRockDTVTarget()
	{
		Finalize();
	}

	// ������
	bool CTvRockDTVTarget::Initialize(int DeviceID,CSettings *pSettings)
	{
		if (DeviceID>=0)
			InitializeTvRockDTV(DeviceID,pSettings);

		return true;
	}

	// �I��
	void CTvRockDTVTarget::Finalize()
	{
		m_Core.RemoveEventHandler(this);
		m_Core.RemoveTsGrabber(this);

		if (m_StreamThread.IsOpened()) {
			OutLog(LOG_VERBOSE,L"TvRock�X�g���[���X���b�h���I�����܂��B");
			m_StreamEndEvent.Set();
			if (m_StreamThread.Wait(5000)==WAIT_TIMEOUT) {
				OutLog(LOG_WARNING,L"TvRock�X�g���[���X���b�h�������I�������܂��B");
				m_StreamThread.Terminate();
			}
			m_StreamThread.Close();
			OutLog(LOG_VERBOSE,L"TvRock�X�g���[���X���b�h���I�����܂����B");
		}
		m_ProcessTSEvent.Close();
		m_StreamEndEvent.Close();
		m_StreamQueue.Clear();

		// TvRockDTVTarget��p��
		if (m_pTvRock!=nullptr) {
			OutLog(LOG_VERBOSE,L"TvRock DTV Target���I�����܂��B");
			m_pTvRock->Dispose();
			SafeDelete(m_pTvRock);
			SafeDelete(m_pRequest);
			OutLog(LOG_INFO,L"TvRock DTV Target���I�����܂����B");
		}

		if (m_hDTVModule!=nullptr) {
			OutLog(LOG_VERBOSE,L"TvRockDTV���C�u������������܂��B");
			::FreeLibrary(m_hDTVModule);
			m_hDTVModule=nullptr;
			OutLog(LOG_VERBOSE,L"TvRockDTV���C�u������������܂����B");
		}

		SafeDelete(m_pChannel);
		m_DeviceID=-1;
	}

	int CTvRockDTVTarget::GetFrequencyByTSID(WORD TransportStreamID)
	{
		if (m_pChannel==nullptr)
			return 0;

		return m_pChannel->GetFrequencyByTSID(TransportStreamID);
	}

	// TvRockDTV�̏�����
	bool CTvRockDTVTarget::InitializeTvRockDTV(int DeviceID,CSettings *pSettings)
	{
		if (DeviceID<0)
			return false;

		if (m_hDTVModule!=nullptr)
			return false;

		OutLog(LOG_IMPORTANT,L"TvRock DTV Target�����������܂��B(DID %c)",DeviceID+L'A');

		// TvRockDTV���C�u�����̓ǂݍ���
		m_hDTVModule=
#ifdef _WIN64
			LoadDTVModule64();
#else
			LoadDTVModule();
#endif
		if (m_hDTVModule==nullptr) {
			OutLog(LOG_ERROR,L"TvRockDTV���C�u���������[�h�ł��܂���B");
			return false;
		}

		OutLog(LOG_VERBOSE,L"TvRockDTV���C�u���������[�h���܂����B(%p)",m_hDTVModule);

		m_DeviceID = DeviceID;

		typedef TvRockDTV *(*CreateTvRockDTV)(HINSTANCE);
		CreateTvRockDTV pFunc=reinterpret_cast<CreateTvRockDTV>(::GetProcAddress(m_hDTVModule,"CreateTvRockDTV"));
		if (pFunc==nullptr) {
			OutLog(LOG_ERROR,L"TvRockDTV���C�u��������v���܂���B");
			Finalize();
			return false;
		}

		m_pTvRock=pFunc(m_hDTVModule);
		if (m_pTvRock==nullptr) {
			OutLog(LOG_ERROR,L"TvRockDTV���쐬�ł��܂���B");
			Finalize();
			return false;
		}

		m_pChannel=new CChannelSearch(m_Core);
		m_pRequest=new CTvRockRequest(m_Core,m_AppCore,*m_pChannel);
		if (!m_pTvRock->Initialize(DeviceID,m_pRequest)) {
			OutLog(LOG_ERROR,L"TvRockDTV�̏������G���[�ł��B");
			Finalize();
			return false;
		}

		// �ݒ�̓ǂݍ���
		if (pSettings!=nullptr) {
			unsigned int BufferLength;
			if (pSettings->Read(L"TvRock.MaxBufferLength",&BufferLength)
					&& BufferLength>=MAX_QUEUE_BLOCKS_MIN
					&& BufferLength<=MAX_QUEUE_BLOCKS_MAX) {
				m_StreamQueueMaxBlocks=BufferLength;
			}
		}

		WCHAR szDevice[8];
		FormatString(szDevice,_countof(szDevice),L"DID_%c",DeviceID+L'A');
		m_pChannel->Initialize(pSettings,szDevice);

		// �ݒ�̊m�F
		RockProperty *pProp=m_pTvRock->GetProperty();
		if (pProp!=nullptr) {
			// �^��t�@�C���g���q�̒���
			WCHAR szExtension[MAX_PATH];
			::ZeroMemory(szExtension,sizeof(szExtension));
			pProp->GetFileExtension(szExtension,_countof(szExtension));
			if (::lstrcmpiW(szExtension,L".ts")!=0) {
				pProp->SetFileExtension(L".ts");
				pProp->Notify();
			}
			m_pChannel->SetProperty(pProp);
		}

		m_StreamQueue.Initialize(QUEUE_BLOCK_SIZE,m_StreamQueueMaxBlocks);

		m_ProcessTSEvent.Create();
		m_StreamEndEvent.Create();

		CThread::CFunctorBase *pFunctor=
			new CThread::CFunctor<CTvRockDTVTarget>(this,&CTvRockDTVTarget::StreamThread);
		m_StreamThread.Begin(pFunctor);

		m_Core.AddEventHandler(this);
		m_Core.AddTsGrabber(this);

		return true;
	}

	// �X�g���[���̔j��
	void CTvRockDTVTarget::PurgeStream()
	{
		m_StreamQueue.Clear();

		if (m_pTvRock!=nullptr) {
			if (m_ProcessTSEvent.IsOpened()) {
				m_ProcessTSEvent.Reset();
				m_ProcessTSEvent.Wait(5000);
			}
		}
	}

	// �X�g���[���擾�X���b�h
	unsigned int CTvRockDTVTarget::StreamThread()
	{
		const size_t BufferSize=m_StreamSendBufferLength*188;
		BYTE *pBuffer=new BYTE[BufferSize];
		size_t ProcessSize;

		do {
			ProcessSize=BufferSize;
			if (m_StreamQueue.GetData(pBuffer,&ProcessSize)) {
				if (m_pTvRock!=nullptr)
					m_pTvRock->ProcessTS(pBuffer,DWORD(ProcessSize));
			} else {
				ProcessSize=0;
			}
			m_ProcessTSEvent.Set();

			if (m_pTvRock!=nullptr) {
				static DWORD SetStatusTime=0;
				const DWORD CurTime=::GetTickCount();
				if (CurTime-SetStatusTime>=1000UL) {
					RockStatus *pStatus=m_pTvRock->GetStatus();
					if (pStatus!=nullptr) {
						StreamStatistics Statistics;
						if (m_Core.GetStreamStatistics(&Statistics)) {
							pStatus->SetStatusSignalLevel(Statistics.SignalLevel);
							pStatus->SetStatusBitrate(Statistics.BitRate*8);
							pStatus->SetStatusErrorPacket((DWORD)(Statistics.ErrorPacketCount+
																  Statistics.DiscontinuityCount));
							pStatus->SetStatusScramblingPacket((DWORD)Statistics.ScramblePacketCount);
						}
					}
					SetStatusTime=CurTime;
				}
			}
		} while (m_StreamEndEvent.Wait(ProcessSize<BufferSize?50:0)==WAIT_TIMEOUT);

		delete [] pBuffer;

		return 0;
	}

	// �X�g���[���̕ύX
	void CTvRockDTVTarget::NotifyStreamChange()
	{
		if (m_pTvRock!=nullptr) {
			ChannelStreamIDs StreamIDs;
			DWORD Channel=m_pChannel!=nullptr ? m_pChannel->GetChannel() : 0;
			DWORD Service=m_Core.GetStreamIDs(&StreamIDs) ? StreamIDs.ServiceID : 0;

			OutLog(LOG_VERBOSE,L"TvRock �ɃX�g���[���̕ύX��ʒm���܂��B(Ch %u / Sv %u)",Channel,Service);
			m_pTvRock->OnStreamChange(Channel,Service);
		}
	}

	void CTvRockDTVTarget::OnTunerOpened()
	{
		NotifyStreamChange();
	}

	void CTvRockDTVTarget::OnChannelChanged(DWORD Space,DWORD Channel,WORD ServiceID)
	{
		NotifyStreamChange();
	}

	void CTvRockDTVTarget::OnStreamChanged()
	{
		PurgeStream();
		NotifyStreamChange();
	}

	void CTvRockDTVTarget::OnServiceChanged(WORD ServiceID)
	{
		NotifyStreamChange();
	}

	bool CTvRockDTVTarget::OnPacket(const ::CTsPacket *pPacket)
	{
		m_StreamQueue.AddData(pPacket->GetData(),pPacket->GetSize());

		return true;
	}

	void CTvRockDTVTarget::OnReset()
	{
		PurgeStream();
	}

}
