#include "precomp_trigger.h"
#include "DboTSItemEquip.h"


NTL_TS_IMPLEMENT_RTTI( CDboTSItemEquip, CNtlTSEvent )


CDboTSItemEquip::CDboTSItemEquip( void )
: m_uiItemIdx( 0xffffffff )
{
}

void CDboTSItemEquip::SetItemIdx( unsigned int uiItemIdx )
{
	m_uiItemIdx = uiItemIdx;
}

void CDboTSItemEquip::ApplyScriptDataForScript( const CNtlTSScrProperty& clProperty )
{
	CNtlTSEvent::ApplyScriptDataForScript( clProperty );

	if ( clProperty.IsExist( "idx" ) )
	{
		SetItemIdx( clProperty.GetValueAsInt( "idx" ) );
	}
}

void CDboTSItemEquip::TakeScriptDataForScript( CNtlTSScrProperty& clProperty )
{
	CNtlTSEvent::TakeScriptDataForScript( clProperty );

	NTL_SNPRINTF( g_NtlTSString, sizeof(g_NtlTSString), "%d", GetItemIdx() );
	clProperty.m_defProperty["idx"] = g_NtlTSString;
}
