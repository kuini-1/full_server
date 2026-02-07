#if defined(_WIN32)
#include <Windows.h>
#include <oleauto.h>
#else
#include "../NtlSharedCommon.h"
#include <cstring>
#endif
#include "NtlLuaState.h"

extern "C" {
#include "lua/src/lua.h"
#include "lua/src/lualib.h"
#include "lua/src/lauxlib.h"
}

#define NTL_MAX_LUA_ERR		2048

CNtlLuaState::CNtlLuaState()
{
	m_pScriptContext = luaL_newstate();
//	luaopen_base(m_pScriptContext);
//	luaopen_io(m_pScriptContext);
//	luaopen_string(m_pScriptContext);
//	luaopen_math(m_pScriptContext);
//	luaopen_debug(m_pScriptContext);
//	luaopen_table(m_pScriptContext);
//	luaopen_loadlib(m_pScriptContext);
	luaL_openlibs(m_pScriptContext);
}

CNtlLuaState::~CNtlLuaState()
{
	if(m_pScriptContext)
		lua_close(m_pScriptContext);
}


void CNtlLuaState::CallErrorHandler(const char *pError)
{
	ListErrorHandler::iterator it;
	for(it = m_listErrorHandler.begin(); it != m_listErrorHandler.end(); ++it)
	{
		(*it)(pError);
	}
}

void CNtlLuaState::CallSuccessHandler(const char *pSuccess)
{
	ListSuccessHandler::iterator it;
	for(it = m_listSuccessHandler.begin(); it != m_listSuccessHandler.end(); ++it)
	{
		(*it)(pSuccess);
	}
}

bool CNtlLuaState::RunScript(const char *pFileName)
{
	if(luaL_loadfile(m_pScriptContext, pFileName) != 0)
	{
		CallErrorHandler(luaL_checkstring(m_pScriptContext, -1));
		return false;
	}

	if(lua_pcall(m_pScriptContext, 0, LUA_MULTRET, 0) != 0)
	{
		CallErrorHandler(luaL_checkstring(m_pScriptContext, -1));
		return false;
	}

	char chSuccess[NTL_MAX_LUA_ERR];
	sprintf_s(chSuccess, NTL_MAX_LUA_ERR, "RunScript(lua_pcall) Success - Script Name : %s\n", pFileName);
	CallSuccessHandler(chSuccess);

	return true;
}

bool CNtlLuaState::RunString(const char *pCmd)
{
	if(luaL_loadbuffer(m_pScriptContext, pCmd, strlen(pCmd), NULL) != 0)
	{
		char chError[ NTL_MAX_LUA_ERR ] = { '\0', };
		sprintf_s(chError, NTL_MAX_LUA_ERR, "RunString(luaL_loadbuffer) Error - Error Message:%s", luaL_checkstring(m_pScriptContext, -1));

		CallErrorHandler(chError);

		return false;
	}

	if(lua_pcall(m_pScriptContext, 0, LUA_MULTRET, 0) != 0)
	{
		char chError[ NTL_MAX_LUA_ERR ] = { '\0', };
		sprintf_s(chError, NTL_MAX_LUA_ERR, "RunString(lua_pcall) Error - Error Message:%s", luaL_checkstring(m_pScriptContext, -1));

		CallErrorHandler(chError);

		return false;
	}

	char chSuccess[NTL_MAX_LUA_ERR] = { '\0', };
	sprintf_s(chSuccess, NTL_MAX_LUA_ERR, "RunString(lua_pcall) Success\n");
	CallSuccessHandler(chSuccess);

	return true;
}

bool CNtlLuaState::AddFunction(const char *pFuncName, LuaFunctionType pFunc)
{
	lua_register(m_pScriptContext, pFuncName, pFunc);
	return true;
}

const char* CNtlLuaState::GetStringArgument(int num, const char *pDefault)
{
	return luaL_optstring(m_pScriptContext, num, pDefault);

}

double CNtlLuaState::GetNumberArgument(int num, double dDefault)
{
	return luaL_optnumber(m_pScriptContext, num, dDefault);
}

bool CNtlLuaState::IsStringArgument(int num)
{
	if ( !lua_isstring( m_pScriptContext, num) )
	{
		return false;
	}

	return true;
}

bool CNtlLuaState::IsNumberArgument(int num)
{
	if ( !lua_isnumber( m_pScriptContext, num) )
	{
		return false;
	}

	return true;
}

void CNtlLuaState::PushNumber(int iValue)
{
	lua_pushnumber(m_pScriptContext, iValue);
}

void CNtlLuaState::PushNumber(unsigned int uiValue)
{
	lua_pushnumber(m_pScriptContext, uiValue);
}

void CNtlLuaState::PushNumber(float fValue)
{
	lua_pushnumber(m_pScriptContext, fValue);
}

void CNtlLuaState::SetNumberVariable(const char *pVariable, int iValue)
{
	lua_pushnumber ( m_pScriptContext, iValue );
	lua_setglobal ( m_pScriptContext, pVariable );
}

void CNtlLuaState::SetNumberVariable(const char *pVariable, unsigned int uiValue)
{
	lua_pushnumber ( m_pScriptContext, uiValue );
	lua_setglobal ( m_pScriptContext, pVariable );
}

void CNtlLuaState::SetNumberVariable(const char *pVariable, float fValue)
{
	lua_pushnumber ( m_pScriptContext, fValue );
	lua_setglobal ( m_pScriptContext, pVariable );
}


void CNtlLuaState::LinkErrorHandler(fpFuncErrorHandler fpFunc)
{
	m_listErrorHandler.push_back(fpFunc);
}

void CNtlLuaState::LinkSuccessHandler(fpFuncSuccessHandler fpFunc)
{
	m_listSuccessHandler.push_back(fpFunc);
}

