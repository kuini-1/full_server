#ifndef _DBO_TSCLICKSIDEICON_H_
#define _DBO_TSCLICKSIDEICON_H_


#include "DboTSCoreDefine.h"


/**
	Click side icon
*/


class CDboTSClickSideIcon : public CNtlTSEvent
{
	NTL_TS_DECLARE_RTTI


// Member variables
protected:
	unsigned char						m_bySideIconType;


// Constructions
public:
	CDboTSClickSideIcon( void );

// Methods
public:
	virtual unsigned int				GetEntityType( void ) const { return DBO_EVENT_TYPE_ID_CLICK_SIDEICON; }

	unsigned char						GetSideIconType( void ) const;



// Implementations
protected:

	virtual	void						ApplyScriptDataForScript( const CNtlTSScrProperty& clProperty );
	virtual	void						TakeScriptDataForScript( CNtlTSScrProperty& clProperty );

};


inline unsigned char CDboTSClickSideIcon::GetSideIconType( void ) const
{
	return m_bySideIconType;
}




#endif