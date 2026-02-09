#include "precomp_trigger.h"
#include "DboTSActTMQTimeBonus.h"


NTL_TS_IMPLEMENT_RTTI( CDboTSActTMQTimeBonus, CNtlTSAction )


CDboTSActTMQTimeBonus::CDboTSActTMQTimeBonus( void )
: m_nBonusTime( 0 )
{
}

void CDboTSActTMQTimeBonus::ApplyScriptDataForScript( const CNtlTSScrProperty& clProperty )
{
	CNtlTSAction::ApplyScriptDataForScript( clProperty );

	if ( clProperty.IsExist( "bt" ) )
	{
		SetBonusTime( clProperty.GetValueAsInt( "bt" ) );
	}
}

void CDboTSActTMQTimeBonus::TakeScriptDataForScript( CNtlTSScrProperty& clProperty )
{
	CNtlTSAction::TakeScriptDataForScript( clProperty );

	NTL_SNPRINTF( g_NtlTSString, sizeof(g_NtlTSString), "%d", GetBonusTime() );
	clProperty.m_defProperty["bt"] = g_NtlTSString;
}
