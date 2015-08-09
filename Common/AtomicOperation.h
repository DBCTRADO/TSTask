#ifndef TSTASK_ATOMIC_OPERATION_H
#define TSTASK_ATOMIC_OPREATION_H


#include <intrin.h>


namespace TSTask
{

	template<typename T> inline void AtomicAnd32(T *pDest,T Operand) {
		static_assert(sizeof(T)==4,"");
#ifdef _M_IX86
		_InterlockedAnd((long volatile*)pDest,(long)Operand);
#else
		::InterlockedAnd((LONG volatile*)pDest,(LONG)Operand);
#endif
	}

	template<typename T> inline void AtomicOr32(T *pDest,T Operand) {
		static_assert(sizeof(T)==4,"");
#ifdef _M_IX86
		_InterlockedOr((long volatile*)pDest,(long)Operand);
#else
		::InterlockedOr((LONG volatile*)pDest,(LONG)Operand);
#endif
	}

	template<typename T> inline void AtomicXor32(T *pDest,T Operand) {
		static_assert(sizeof(T)==4,"");
#ifdef _M_IX86
		_InterlockedXor((long volatile*)pDest,(long)Operand);
#else
		::InterlockedXor((LONG volatile*)pDest,(LONG)Operand);
#endif
	}

	template<typename T> inline void AtomicSet64(T *pDest,T Value) {
		static_assert(sizeof(T)==8,"");
#ifdef _WIN64
		*pDest=Value;
#else
		::InterlockedExchange64((LONGLONG volatile*)pDest,(LONGLONG)Value);
#endif
	}

	template<typename T> inline T AtomicGet64(T *pValue) {
		static_assert(sizeof(T)==8,"");
#ifdef _WIN64
		return *pValue;
#else
		return ::InterlockedCompareExchange64((LONGLONG volatile*)pValue,0,0);
#endif
	}

}


#endif
