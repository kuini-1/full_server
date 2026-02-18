#include "stdafx.h"
#include "gm.h"
#include "GameServer.h"
#include "CPlayer.h"
#include "Monster.h"
#include "ItemManager.h"
#include "NtlPacketUG.h"
#include "NtlPacketGU.h"
#include "NtlPacketGQ.h"
#include "NtlPacketGT.h"
#include "NtlPacketGM.h"
#include "NtlPacketMG.h"
#include "NtlPacketUT.h"
#include "NtlResultCode.h"
#include "SubNeighborServerInfoManager.h"
#include "NtlTokenizer.h"
#include "TableContainerManager.h"
#include "ItemTable.h"
#include "ExpTable.h"
#include "GameProcessor.h"
#include "HLSItemTable.h"
#include "SystemEffectTable.h"
#include "TriggerObject.h"
#include "GameMain.h"
#include "DragonballScramble.h"
#include "HoneyBeeEvent.h"
#include "ItemDrop.h"
#include "NtlStringW.h"
#include "PortalTable.h"
#include "NeighborServerInfoManager.h"
#include "SummonPet.h"
#include "DungeonManager.h"
#include "NtlPacketTU.h"
#include "NtlPacketTQ.h"
#include "ItemRecipeTable.h"
#include "DynamicFieldSystemEvent.h"
#include "BattleRoyaleEvent.h"



enum EMPPTeleport
{
	MPP_TELE_YAHOI,             // ????? ????
	MPP_TELE_YUREKA,            // ????? ????
	MPP_TELE_DALPANG,           // ?????? ????
	MPP_TELE_DRAGON,            // ???? ?????
	MPP_TELE_BAEE,              // ?? ??????
	MPP_TELE_AJIRANG,           // ???????? ??
	MPP_TELE_KARINGA_1,         // ??????? ???? 1
	MPP_TELE_KARINGA_2,         // ??????? ???? 2
	MPP_TELE_GREAT_TREE,        // ?????? ???
	MPP_TELE_KARINGA_3,         // ??????? ????
	MPP_TELE_MERMAID,           // ?��??
	MPP_TELE_GANNET,            // ???? ?????
	MPP_TELE_EMERALD,           // ??????? ???
	MPP_TELE_TEMBARIN,          // ????? ???    
	MPP_TELE_CELL,              // ?��?
	MPP_TELE_BUU,               // ?��? ????
	MPP_TELE_CC,                // CC ???? ?????
	MPP_TELE_MUSHROOM,          // ??????

	MPP_TELE_PAPAYA,
};


void gm_read_command(sUG_SERVER_COMMAND* sPacket, CPlayer* pPlayer)
{
	CGameServer* app = (CGameServer*)g_pApp;

	char chBuffer[1024];
	WideCharToMultiByte(GetACP(), 0, sPacket->awchCommand, -1, chBuffer, 1024, NULL, NULL);

	CNtlTokenizer lexer(chBuffer);
	int iLine;
	int icmd;

	std::string strCommand = lexer.PeekNextToken(NULL, &iLine);

	for (icmd = 0; icmd < NTL_MAX_LENGTH_OF_CHAT_MESSAGE; icmd++)
	{
		if (!strcmp(cmd_info[icmd].command, "@qwasawedsad"))
			return;
		else if (!strcmp(cmd_info[icmd].command, strCommand.c_str()))
			break;
	}

	if (cmd_info[icmd].eAdminLevel > pPlayer->GetGMLevel()) //check if player gm level high enough to use command
		return;

	if (cmd_info[icmd].eAdminLevel > ADMIN_LEVEL_EARLY_ACCESS && pPlayer->IsGameMaster() == false)
		return;

	// log
	CNtlPacket packetQry(sizeof(sGQ_GM_LOG));
	sGQ_GM_LOG* resQry = (sGQ_GM_LOG*)packetQry.GetPacketData();
	resQry->wOpCode = GQ_GM_LOG;
	resQry->charId = pPlayer->GetCharID();
	resQry->byLogType = 0;
	strcpy(resQry->chBuffer, chBuffer);
	packetQry.SetPacketLen(sizeof(sGQ_GM_LOG));
	app->SendTo(app->GetQueryServerSession(), &packetQry);

	((*cmd_info[icmd].command_pointer) (pPlayer, &lexer, iLine));
}




ACMD(do_setspeed);
ACMD(do_addmob);
ACMD(do_addmobgroup);
ACMD(do_addnpc);
ACMD(do_additem);
ACMD(do_addmasteritem); // this function will give
ACMD(do_adddefaultbank); // if default bank (which is created when create an item) does not exist, then it will be created.
ACMD(do_addskill);
ACMD(do_addskill2); // this function add missing master class passive. Example.. if player is swordsman and dont have swordsman masterclass skill, then this can be used.
ACMD(do_r);
ACMD(do_addhtb);
ACMD(do_setzenny);
ACMD(do_setlevel);
ACMD(do_hide);
ACMD(do_notice);
ACMD(do_pm);
ACMD(do_teleport);
ACMD(do_world);
ACMD(do_warp);
ACMD(do_call);
ACMD(do_shutdown);
ACMD(do_setadult);
ACMD(do_setclass);
ACMD(do_dc);
ACMD(do_kill);
ACMD(do_delallitems);
ACMD(do_god);
ACMD(do_invincible);
ACMD(do_bann);
ACMD(do_dbann);
ACMD(do_purge);
ACMD(do_unstack);
ACMD(do_setnetpy);
ACMD(do_gethlsitem);
ACMD(do_skillspeed);
ACMD(do_attackspeed);
ACMD(do_warfog);
ACMD(do_test);
ACMD(do_upgrade);
ACMD(do_setitemrank);
ACMD(do_logout);
ACMD(do_mute);
ACMD(do_unmute);
ACMD(do_go);
ACMD(do_addtitle);
ACMD(do_deltitle);
ACMD(do_setitemduration);
ACMD(do_bind);
ACMD(do_exp);
ACMD(do_resetexp);
ACMD(do_starthoneybee);
ACMD(do_stophoneybee);
ACMD(do_deleteguild);
ACMD(do_cancelah);
ACMD(do_addmudosa);
ACMD(do_startgm);
ACMD(do_createloot);
ACMD(do_ud);
ACMD(do_tmq);
ACMD(do_bid);
ACMD(do_cc);
ACMD(do_switch);
ACMD(do_switch2);
ACMD(do_playercount);
ACMD(do_pet);
ACMD(do_relog);
ACMD(do_print_location);
ACMD(do_recipe_items);
ACMD(do_dungeon_loc);
ACMD(do_size);
ACMD(do_transform);
ACMD(do_skill_restore);
ACMD(do_skill_points);
ACMD(do_subclass);
ACMD(do_dd);
ACMD(do_itemrange);
ACMD(do_testitems);
ACMD(do_br);


struct command_info cmd_info[] =
{
	//lv0
	
	{ "@addmasteritem", do_addmasteritem, ADMIN_LEVEL_NONE },
	{ "@adddefaultbank", do_adddefaultbank, ADMIN_LEVEL_NONE },
	{ "@addskill2", do_addskill2, ADMIN_LEVEL_NONE },
	{ "@addhtb", do_addhtb, ADMIN_LEVEL_NONE },
	{ "@unstack", do_unstack, ADMIN_LEVEL_NONE },
	{ "@exp", do_exp, ADMIN_LEVEL_NONE },
	{ "@resetexp", do_resetexp, ADMIN_LEVEL_NONE },
	{ "@ud", do_ud, ADMIN_LEVEL_NONE },
	{ "@sub", do_subclass, ADMIN_LEVEL_NONE },
	//{ "@setlevel", do_setlevel, ADMIN_LEVEL_NONE },


		//lv2
	{ "@setspeed", do_setspeed, ADMIN_LEVEL_GAME_MASTER },
	{ "@teleport", do_teleport, ADMIN_LEVEL_GAME_MASTER },
	{ "@mute", do_mute, ADMIN_LEVEL_GAME_MASTER },
	{ "@unmute", do_unmute, ADMIN_LEVEL_GAME_MASTER },

		//lv3
	{ "@cc", do_cc, ADMIN_LEVEL_COMMUNITY_MANAGER },
	{ "@tmq", do_tmq, ADMIN_LEVEL_COMMUNITY_MANAGER },
	{ "@bid", do_bid, ADMIN_LEVEL_COMMUNITY_MANAGER },
	{ "@notice", do_notice, ADMIN_LEVEL_COMMUNITY_MANAGER },
	{ "@call", do_call, ADMIN_LEVEL_COMMUNITY_MANAGER },
	{ "@warp", do_warp, ADMIN_LEVEL_COMMUNITY_MANAGER },
	{ "@hide", do_hide, ADMIN_LEVEL_COMMUNITY_MANAGER },
	{ "@setzenny", do_setzenny, ADMIN_LEVEL_COMMUNITY_MANAGER },
	{ "@online", do_playercount, ADMIN_LEVEL_COMMUNITY_MANAGER },
	{ "@class", do_switch, ADMIN_LEVEL_COMMUNITY_MANAGER },
		//{ "@sp", do_skill_points, ADMIN_LEVEL_COMMUNITY_MANAGER },


		//lv4
	{ "@setadult", do_setadult, ADMIN_LEVEL_DEV_SERVER },

		//lv9
	{ "@addtitle", do_addtitle, ADMIN_LEVEL_TEAMLEADER },
	{ "@starthoneybee", do_starthoneybee, ADMIN_LEVEL_TEAMLEADER },
	{ "@stophoneybee", do_stophoneybee, ADMIN_LEVEL_TEAMLEADER },
	{ "@additem", do_additem, ADMIN_LEVEL_TEAMLEADER },
	{ "@setlevel", do_setlevel, ADMIN_LEVEL_TEAMLEADER },
	{ "@upgrade", do_upgrade, ADMIN_LEVEL_TEAMLEADER },
	{ "@setclass", do_setclass, ADMIN_LEVEL_TEAMLEADER },
	{ "@setitemrank", do_setitemrank, ADMIN_LEVEL_TEAMLEADER },
	{ "@addmudosa", do_addmudosa, ADMIN_LEVEL_TEAMLEADER },

		//lv10 
		//{ "@sp", do_skill_points, ADMIN_LEVEL_ADMIN },
	{ "@class", do_switch, ADMIN_LEVEL_ADMIN },
	{ "@purge", do_purge, ADMIN_LEVEL_ADMIN },
	{ "@createloot", do_createloot, ADMIN_LEVEL_ADMIN },
	{ "@addmob", do_addmob, ADMIN_LEVEL_ADMIN },
	{ "@setzenny", do_setzenny, ADMIN_LEVEL_ADMIN },
	{ "@shutdown", do_shutdown, ADMIN_LEVEL_ADMIN },
	{ "@kill", do_kill, ADMIN_LEVEL_ADMIN },
	{ "@setnetpy", do_setnetpy, ADMIN_LEVEL_ADMIN },
	{ "@skillspeed", do_skillspeed, ADMIN_LEVEL_ADMIN },
	{ "@attackspeed", do_attackspeed, ADMIN_LEVEL_ADMIN },
	{ "@test", do_test, ADMIN_LEVEL_ADMIN },
	{ "@setitemrank", do_setitemrank, ADMIN_LEVEL_ADMIN },
	{ "@logout", do_logout, ADMIN_LEVEL_ADMIN },
	{ "@startgm", do_startgm, ADMIN_LEVEL_ADMIN },



	{ "@addmob", do_addmob, ADMIN_LEVEL_ADMIN },
	{ "@addmobgroup", do_addmobgroup, ADMIN_LEVEL_ADMIN },
	{ "@addnpc", do_addnpc, ADMIN_LEVEL_ADMIN },
	{ "@hide", do_hide, ADMIN_LEVEL_ADMIN },
	{ "@notice", do_notice, ADMIN_LEVEL_ADMIN },
	{ "@pm", do_pm, ADMIN_LEVEL_ADMIN },
	{ "@world", do_world, ADMIN_LEVEL_ADMIN },
	{ "@warp", do_warp, ADMIN_LEVEL_ADMIN },
	{ "@call", do_call, ADMIN_LEVEL_ADMIN },
	{ "@dc", do_dc, ADMIN_LEVEL_ADMIN },
	{ "@delallitems", do_delallitems, ADMIN_LEVEL_ADMIN },
	{ "@god", do_god, ADMIN_LEVEL_ADMIN },
	{ "@invincible", do_invincible, ADMIN_LEVEL_ADMIN },
	{ "@bann", do_bann, ADMIN_LEVEL_ADMIN },
	{ "@dbann", do_dbann, ADMIN_LEVEL_ADMIN },
	{ "@warfog", do_warfog, ADMIN_LEVEL_ADMIN },
	{ "@go", do_go, ADMIN_LEVEL_ADMIN },
	{ "@addtitle", do_addtitle, ADMIN_LEVEL_ADMIN },
	{ "@deltitle", do_deltitle, ADMIN_LEVEL_ADMIN },
	{ "@bind", do_bind, ADMIN_LEVEL_ADMIN },
	{ "@cancelah", do_cancelah, ADMIN_LEVEL_ADMIN },
	{ "@purge", do_purge, ADMIN_LEVEL_ADMIN },
	{ "@starthoneybee", do_starthoneybee, ADMIN_LEVEL_ADMIN },
	{ "@stophoneybee", do_stophoneybee, ADMIN_LEVEL_ADMIN },
	{ "@deleteguild", do_deleteguild, ADMIN_LEVEL_ADMIN },
	{ "@createloot", do_createloot, ADMIN_LEVEL_ADMIN },
	{ "@additem", do_additem, ADMIN_LEVEL_ADMIN },
	{ "@r", do_r, ADMIN_LEVEL_ADMIN },
	{ "@setadult", do_setadult, ADMIN_LEVEL_ADMIN },
	{ "@gethlsitem", do_gethlsitem, ADMIN_LEVEL_ADMIN },
	{ "@upgrade", do_upgrade, ADMIN_LEVEL_ADMIN },
	{ "@setitemduration", do_setitemduration, ADMIN_LEVEL_ADMIN },
	{ "@addskill", do_addskill, ADMIN_LEVEL_ADMIN },
	{ "@setlevel", do_setlevel, ADMIN_LEVEL_ADMIN },
	{ "@setclass", do_setclass, ADMIN_LEVEL_ADMIN },
	{ "@addmudosa", do_addmudosa, ADMIN_LEVEL_ADMIN },
	{ "@online", do_playercount, ADMIN_LEVEL_ADMIN },
	{ "@loc", do_print_location, ADMIN_LEVEL_ADMIN },
	{ "@recipe", do_recipe_items, ADMIN_LEVEL_ADMIN },

		//new commands
	{ "@testitem", do_testitems, ADMIN_LEVEL_ADMIN },
	{ "@itemrange", do_itemrange, ADMIN_LEVEL_ADMIN },
	{ "@transform", do_transform, ADMIN_LEVEL_ADMIN },
	{ "@size", do_size, ADMIN_LEVEL_ADMIN },
	{ "@pet", do_pet, ADMIN_LEVEL_ADMIN },
	{ "@relog", do_relog, ADMIN_LEVEL_ADMIN },
	{ "@dun", do_dungeon_loc, ADMIN_LEVEL_ADMIN },
	{ "@dd", do_dd, ADMIN_LEVEL_ADMIN },
	{ "@br", do_br, ADMIN_LEVEL_ADMIN },
	//{ "@rs", do_skill_restore, ADMIN_LEVEL_ADMIN },

	{ "@qwasawedsad", NULL, ADMIN_LEVEL_ADMIN },
};

enum PASSIVES {
	POWER_UP = 0,
	GUARD,
	DASH,
	COUNTER,
	FLY
};

enum DRAGON_BUFF_TYPE {
	CON = 0,
	STR,
	SOL,
	FOC,
	DEX,
	ENG
};

ACMD(do_dd)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX count = (TBLIDX)atof(strToken.c_str());
	g_pDynamicFieldSystemEvent->UpdateDynamicFieldCount(count);
}

ACMD(do_skill_restore)
{
	/*
	CGameServer* app = (CGameServer*)g_pApp;

	CPlayer* pTarget = pPlayer;
	CPlayer* pTargetPC = g_pObjectManager->GetPC(pTarget->GetTargetHandle());

	if (pTargetPC && pTargetPC->IsInitialized() && pTargetPC->IsPC())
	{
		//pTargetPC->GetSkillManager()->InitSkills();
		//pTargetPC->GetSkillManager()->ResetSkills();
		pTargetPC->UpdateCharSP(((DWORD)pTargetPC->GetLevel() - 1) + pTargetPC->GetSkillPointsBought());

		CNtlPacket pQry(sizeof(sGQ_SKILL_INIT_REQ)); // reset skills
		sGQ_SKILL_INIT_REQ* rQry = (sGQ_SKILL_INIT_REQ*)pQry.GetPacketData();
		rQry->wOpCode = GQ_SKILL_INIT_REQ;
		rQry->handle = pTargetPC->GetID();
		rQry->charId = pTargetPC->GetCharID();
		rQry->dwSP = ((DWORD)pTargetPC->GetLevel() - 1) + pTargetPC->GetSkillPointsBought();
		rQry->dwZenny = 0;
		rQry->bySkillResetMethod = 0;
		pQry.SetPacketLen(sizeof(sGQ_SKILL_INIT_REQ));
		app->SendTo(app->GetQueryServerSession(), &pQry);

		CNtlPacket packetQry(sizeof(sGQ_PC_UPDATE_LEVEL_REQ));
		sGQ_PC_UPDATE_LEVEL_REQ* resQry = (sGQ_PC_UPDATE_LEVEL_REQ*)packetQry.GetPacketData();
		resQry->wOpCode = GQ_PC_UPDATE_LEVEL_REQ;
		resQry->handle = pTargetPC->GetID();
		resQry->charId = pTargetPC->GetCharID();
		resQry->dwEXP = 0;
		resQry->byLevel = pTargetPC->GetLevel();
		resQry->dwSP = pTargetPC->GetSkillPoints();
		packetQry.SetPacketLen(sizeof(sGQ_PC_UPDATE_LEVEL_REQ));
		app->SendTo(app->GetQueryServerSession(), &packetQry);
	}
	*/
	




	

	CPlayer* pTarget = pPlayer;

	CGameServer* app = (CGameServer*)g_pApp;
	CWorldCell* pWorldCell = pPlayer->GetCurWorldCell();
	if (!pWorldCell)
		return;

	sVECTOR3 vec;
	CPlayer* pNextPlayer = NULL;

	CWorldCell::QUADPAGE page = pWorldCell->GetCellQuadPage(pPlayer->GetCurLoc());
	for (int dir = CWorldCell::QUADPAGE_FIRST; dir < CWorldCell::QUADPAGE_COUNT; dir++)
	{
		CWorldCell* pWorldCellSibling = pWorldCell->GetQuadSibling(page, (CWorldCell::QUADDIR)dir);
		if (pWorldCellSibling)
		{
			CPlayer* pMobTarget = (CPlayer*)pWorldCellSibling->GetObjectList()->GetFirst(OBJTYPE_PC);
			while (pMobTarget)
			{
				pNextPlayer = (CPlayer*)pWorldCellSibling->GetObjectList()->GetNext(pMobTarget->GetWorldCellObjectLinker());

				if (pMobTarget->GetCurWorld())
				{
					
					pMobTarget->UpdateCharSP(((DWORD)pMobTarget->GetLevel() - 1) + pMobTarget->GetSkillPointsBought());

					CNtlPacket pQry(sizeof(sGQ_SKILL_INIT_REQ)); // reset skills
					sGQ_SKILL_INIT_REQ* rQry = (sGQ_SKILL_INIT_REQ*)pQry.GetPacketData();
					rQry->wOpCode = GQ_SKILL_INIT_REQ;
					rQry->handle = pMobTarget->GetID();
					rQry->charId = pMobTarget->GetCharID();
					rQry->dwSP = ((DWORD)pMobTarget->GetLevel() - 1) + pMobTarget->GetSkillPointsBought();
					rQry->dwZenny = 0;
					rQry->bySkillResetMethod = 0;
					pQry.SetPacketLen(sizeof(sGQ_SKILL_INIT_REQ));
					app->SendTo(app->GetQueryServerSession(), &pQry);

					CNtlPacket packetQry(sizeof(sGQ_PC_UPDATE_LEVEL_REQ));
					sGQ_PC_UPDATE_LEVEL_REQ* resQry = (sGQ_PC_UPDATE_LEVEL_REQ*)packetQry.GetPacketData();
					resQry->wOpCode = GQ_PC_UPDATE_LEVEL_REQ;
					resQry->handle = pMobTarget->GetID();
					resQry->charId = pMobTarget->GetCharID();
					resQry->dwEXP = 0;
					resQry->byLevel = pMobTarget->GetLevel();
					resQry->dwSP = pMobTarget->GetSkillPoints();
					packetQry.SetPacketLen(sizeof(sGQ_PC_UPDATE_LEVEL_REQ));
					app->SendTo(app->GetQueryServerSession(), &packetQry);

					/////////////////////////////////////////////////////////////

					TBLIDX transformation = INVALID_TBLIDX;
					TBLIDX masterPassive[2] = { INVALID_TBLIDX, INVALID_TBLIDX };
					TBLIDX htb[3] = { INVALID_TBLIDX, INVALID_TBLIDX, INVALID_TBLIDX };
					TBLIDX dragonBuffs[6] = { INVALID_TBLIDX, INVALID_TBLIDX, INVALID_TBLIDX, INVALID_TBLIDX, INVALID_TBLIDX, INVALID_TBLIDX };
					TBLIDX passives[5] = { INVALID_TBLIDX, INVALID_TBLIDX, INVALID_TBLIDX, INVALID_TBLIDX, INVALID_TBLIDX };
					WORD wTemp;

					switch (pMobTarget->GetClass())
					{
					case PC_CLASS_HUMAN_FIGHTER:
					case PC_CLASS_STREET_FIGHTER:
					case PC_CLASS_SWORD_MASTER:
					{ 
						transformation = 21011;

						masterPassive[0] = 729991;
						masterPassive[1] = 829991;

						htb[0] = 30611;
						htb[1] = 730831;
						htb[2] = 830841;

						dragonBuffs[CON] = 99110;
						dragonBuffs[STR] = 99100;
						dragonBuffs[SOL] = 99140;
						dragonBuffs[FOC] = 99120;
						dragonBuffs[DEX] = 99130;
						dragonBuffs[ENG] = 99150;

						passives[POWER_UP] = 20221;
						passives[GUARD] = 20201;
						passives[DASH] = 20211;
						passives[COUNTER] = 10021;
						passives[FLY] = 20911;
					} break;

					case PC_CLASS_HUMAN_MYSTIC:
					case PC_CLASS_CRANE_ROSHI:
					case PC_CLASS_TURTLE_ROSHI:
					{ 
						transformation = 121111;

						masterPassive[0] = 929991;
						masterPassive[1] = 1029991;

						htb[0] = 130411;
						htb[1] = 930531;
						htb[2] = 1030541;

						dragonBuffs[CON] = 99111;
						dragonBuffs[STR] = 99101;
						dragonBuffs[SOL] = 99141;
						dragonBuffs[FOC] = 99121;
						dragonBuffs[DEX] = 99131;
						dragonBuffs[ENG] = 99151;

						passives[POWER_UP] = 120501;
						passives[GUARD] = 120301;
						passives[DASH] = 120311;
						passives[COUNTER] = 110051;
						passives[FLY] = 120911;
					} break;

					case PC_CLASS_NAMEK_FIGHTER:
					case PC_CLASS_DARK_WARRIOR:
					case PC_CLASS_SHADOW_KNIGHT:
					{ 
						transformation = 320911;

						masterPassive[0] = 1329991;
						masterPassive[1] = 1429991;

						htb[0] = 330611;
						htb[1] = 1330751;
						htb[2] = 1430761;

						dragonBuffs[CON] = 99113;
						dragonBuffs[STR] = 99103;
						dragonBuffs[SOL] = 99143;
						dragonBuffs[FOC] = 99123;
						dragonBuffs[DEX] = 99133;
						dragonBuffs[ENG] = 99153;

						passives[POWER_UP] = 320321;
						passives[GUARD] = 320201;
						passives[DASH] = 320311;
						passives[COUNTER] = 310021;
						passives[FLY] = 320811;
					} break;

					case PC_CLASS_NAMEK_MYSTIC:
					case PC_CLASS_DENDEN_HEALER:
					case PC_CLASS_POCO_SUMMONER:
					{ 
						transformation = 421211;

						masterPassive[0] = 1529991;
						masterPassive[1] = 1629991;

						htb[0] = 430411;
						htb[1] = 1530531;
						htb[2] = 1630541;

						dragonBuffs[CON] = 99114;
						dragonBuffs[STR] = 99104;
						dragonBuffs[SOL] = 99144;
						dragonBuffs[FOC] = 99124;
						dragonBuffs[DEX] = 99134;
						dragonBuffs[ENG] = 99154;

						passives[POWER_UP] = 420421;
						passives[GUARD] = 420301;
						passives[DASH] = 420411;
						passives[COUNTER] = 410011;
						passives[FLY] = 421111;
					} break;

					case PC_CLASS_MIGHTY_MAJIN:
					case PC_CLASS_ULTI_MA:
					case PC_CLASS_GRAND_MA:
					{ 
						transformation = 520251;

						masterPassive[0] = 1729991;
						masterPassive[1] = 1829991;

						htb[0] = 532011;
						htb[1] = 1732021;
						htb[2] = 1832031;

						dragonBuffs[CON] = 99115;
						dragonBuffs[STR] = 99105;
						dragonBuffs[SOL] = 99145;
						dragonBuffs[FOC] = 99125;
						dragonBuffs[DEX] = 99135;
						dragonBuffs[ENG] = 99155;

						passives[POWER_UP] = 520161;
						passives[GUARD] = 520131;
						passives[DASH] = 520321;
						passives[COUNTER] = 510031;
						passives[FLY] = 520241;
					} break;

					case PC_CLASS_WONDER_MAJIN:
					case PC_CLASS_PLAS_MA:
					case PC_CLASS_KAR_MA:
					{ 
						transformation = 620241;

						masterPassive[0] = 1929991;
						masterPassive[1] = 2029991;

						htb[0] = 632011;
						htb[1] = 1932021;
						htb[2] = 2032031;

						dragonBuffs[CON] = 99116;
						dragonBuffs[STR] = 99106;
						dragonBuffs[SOL] = 99146;
						dragonBuffs[FOC] = 99126;
						dragonBuffs[DEX] = 99136;
						dragonBuffs[ENG] = 99156;

						passives[POWER_UP] = 620161;
						passives[GUARD] = 620131;
						passives[DASH] = 620151;
						passives[COUNTER] = 610021;
						passives[FLY] = 620231;
					} break;

					}

					if (pMobTarget->GetSkillManager())
					{
						for (BYTE COUNT = 0; COUNT < 3; COUNT++) pMobTarget->GetHtbSkillManager()->LearnHtbSkill(htb[COUNT], wTemp);
						for (BYTE COUNT = 0; COUNT < 2; COUNT++) pMobTarget->GetSkillManager()->LearnSkill(masterPassive[COUNT], wTemp, false);
						//for (BYTE COUNT = 0; COUNT < 6; COUNT++) pMobTarget->GetSkillManager()->LearnSkill(dragonBuffs[COUNT], wTemp, false);
						for (BYTE COUNT = 0; COUNT < 5; COUNT++) pMobTarget->GetSkillManager()->LearnSkill(passives[COUNT], wTemp, false);
						pMobTarget->GetSkillManager()->LearnSkill(transformation, wTemp, false);
						//pMobTarget->GetSkillManager()->LearnSkill(SkillId1, wTemp, false);
						//pMobTarget->GetSkillManager()->LearnSkill(SkillId2, wTemp, false);
					}
						

				}

				pMobTarget = pNextPlayer;
			}
		}
	}


}

ACMD(do_skill_points)
{
	CGameServer* app = (CGameServer*)g_pApp;

	CPlayer* pTarget = pPlayer;
	CPlayer* pTargetPC = g_pObjectManager->GetPC(pTarget->GetTargetHandle());

	if (pTargetPC && pTargetPC->IsInitialized() && pTargetPC->IsPC())
	{
		pTargetPC->UpdateCharSP(((DWORD)pTargetPC->GetLevel() - 1) + pTargetPC->GetSkillPointsBought());

		CNtlPacket pQry(sizeof(sGQ_SKILL_INIT_REQ)); // reset skills
		sGQ_SKILL_INIT_REQ* rQry = (sGQ_SKILL_INIT_REQ*)pQry.GetPacketData();
		rQry->wOpCode = GQ_SKILL_INIT_REQ;
		rQry->handle = pTargetPC->GetID();
		rQry->charId = pTargetPC->GetCharID();
		rQry->dwSP = ((DWORD)pTargetPC->GetLevel() - 1) + pTargetPC->GetSkillPointsBought();
		rQry->dwZenny = 0;
		rQry->bySkillResetMethod = 0;
		pQry.SetPacketLen(sizeof(sGQ_SKILL_INIT_REQ));
		app->SendTo(app->GetQueryServerSession(), &pQry);

		CNtlPacket packetQry(sizeof(sGQ_PC_UPDATE_LEVEL_REQ));
		sGQ_PC_UPDATE_LEVEL_REQ* resQry = (sGQ_PC_UPDATE_LEVEL_REQ*)packetQry.GetPacketData();
		resQry->wOpCode = GQ_PC_UPDATE_LEVEL_REQ;
		resQry->handle = pTargetPC->GetID();
		resQry->charId = pTargetPC->GetCharID();
		resQry->dwEXP = 0;
		resQry->byLevel = pTargetPC->GetLevel();
		resQry->dwSP = pTargetPC->GetSkillPoints();
		packetQry.SetPacketLen(sizeof(sGQ_PC_UPDATE_LEVEL_REQ));
		app->SendTo(app->GetQueryServerSession(), &packetQry);
	}
}

ACMD(do_skill_dragonbuff)
{
	CGameServer* app = (CGameServer*)g_pApp;

	CPlayer* pTarget = pPlayer;
	CPlayer* pTargetPC = g_pObjectManager->GetPC(pTarget->GetTargetHandle());

	if (pTargetPC && pTargetPC->IsInitialized() && pTargetPC->IsPC())
	{
		
	}

}

ACMD(do_transform)
{
	CGameServer* app = (CGameServer*)g_pApp;

	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	DWORD MonsterID = (DWORD)atof(strToken.c_str());

	sMOB_TBLDAT* pMOBTblData = (sMOB_TBLDAT*)g_pTableContainer->GetMobTable()->FindData(MonsterID);

	//pPlayer->SetTblidx(MonsterID);
	CNtlPacket packet(sizeof(sGU_MONSTER_TRANSFORMED_NFY));
	sGU_MONSTER_TRANSFORMED_NFY* res = (sGU_MONSTER_TRANSFORMED_NFY*)packet.GetPacketData();
	res->wOpCode = GU_MONSTER_TRANSFORMED_NFY;
	res->hSubject = pPlayer->GetID();
	res->newTblidx = MonsterID;
	packet.SetPacketLen(sizeof(sGU_MONSTER_TRANSFORMED_NFY));
	pPlayer->Broadcast(&packet);

	//WCHAR* msg = L"Receive EXP has been enabled";
	CNtlPacket packet2(sizeof(sTU_CHAR_NAME_CHANGED_NFY));
	sTU_CHAR_NAME_CHANGED_NFY* res2 = (sTU_CHAR_NAME_CHANGED_NFY*)packet2.GetPacketData();
	res2->wOpCode = TU_CHAR_NAME_CHANGED_NFY;
	res2->targetID = pPlayer->GetCharID();
	NTL_SAFE_WCSCPY(res2->wszCharName, pPlayer->GetCharName());
	NTL_SAFE_WCSCPY(res2->wszOldCharName, pPlayer->GetCharName());
	packet2.SetPacketLen(sizeof(sTU_CHAR_NAME_CHANGED_NFY));
	pPlayer->Broadcast(&packet2);

}

ACMD(do_size)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX recipeId = (TBLIDX)atof(strToken.c_str());
	
	pPlayer->UpdateSizeRate(20);

	CNtlPacket packet(sizeof(sGU_MONSTER_TRANSFORMED_NFY));
	sGU_MONSTER_TRANSFORMED_NFY* res = (sGU_MONSTER_TRANSFORMED_NFY*)packet.GetPacketData();
	res->wOpCode = GU_MONSTER_TRANSFORMED_NFY;
	res->hSubject = pPlayer->GetID();
	res->newTblidx = recipeId;
	packet.SetPacketLen(sizeof(sGU_MONSTER_TRANSFORMED_NFY));
	pPlayer->Broadcast(&packet);
}

ACMD(do_recipe_items)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX recipeId = (TBLIDX)atof(strToken.c_str());
	TBLIDX ItemId = INVALID_TBLIDX;

	sITEM_RECIPE_TBLDAT* recipeTbldat = (sITEM_RECIPE_TBLDAT*)g_pTableContainer->GetItemRecipeTable()->FindData(recipeId);

	for (int a = 0; a < DBO_MAX_COUNT_RECIPE_MATERIAL_ITEM; a++)
	{
		if (recipeTbldat->asMaterial[a].materialTblidx != INVALID_TBLIDX)
		{
			ItemId = recipeTbldat->asMaterial[a].materialTblidx;
			if (pPlayer->GetPlayerItemContainer()->CountEmptyInventory() >= 1)
			{
				sITEM_TBLDAT* pTblData = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(ItemId);
				if (pTblData)
				{
					if (pTblData->bValidity_Able == true)
					{
						BYTE amount = pTblData->byMax_Stack;

						g_pItemManager->CreateItem(pPlayer, ItemId, amount, INVALID_BYTE, INVALID_BYTE, pTblData->Item_Option_Tblidx == INVALID_TBLIDX);
					}
					else NTL_PRINT(PRINT_APP, "GmAddItem(TBLIDX itemid) item not bValidity_Able true ud", ItemId);
				}
				else NTL_PRINT(PRINT_APP, "GmAddItem(TBLIDX itemid) item not found %u", ItemId);
			}
		}
	}
}

ACMD(do_dungeon_loc)
{
	CNtlVector loc;
	loc.x = 4548.609863; loc.y = -59.180000; loc.z = 4194.390137;
	pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
}

ACMD(do_print_location)
{
	printf("%f;%f;%f\n", pPlayer->GetCurLoc().x, pPlayer->GetCurLoc().y, pPlayer->GetCurLoc().z);
	printf("%f;%f;%f\n", pPlayer->GetCurDir().x, pPlayer->GetCurDir().y, pPlayer->GetCurDir().z);

	printf("world idx%u\n", pPlayer->GetCurWorld()->GetIdx());
}

ACMD(do_pet)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX byPet = (TBLIDX)atof(strToken.c_str());

	if (((CPlayer*)pPlayer)->GetCurrentPetId() == INVALID_HOBJECT)
	{
		sNPC_TBLDAT* pNpcTbldat = (sNPC_TBLDAT*)g_pTableContainer->GetNpcTable()->FindData((TBLIDX)byPet);
		if (pNpcTbldat)
		{
			CSummonPet* pPet = (CSummonPet*)g_pObjectManager->CreateCharacter(OBJTYPE_SUMMON_PET);
			if (pPet)
			{
				if (pPet->CreateDataAndSpawn(pPlayer->GetID(), byPet, pNpcTbldat, pPlayer->GetWorldID(), pPlayer->GetCurLoc(), pPlayer->GetCurDir())) pPet->Spawn((CPlayer*)pPlayer);
				else g_pObjectManager->DestroyCharacter(pPet);
			}
		}
	}
	else
	{
		CSummonPet* pet = g_pObjectManager->GetSummonPet(pPlayer->GetCurrentPetId());
		pet->Despawn();

		sNPC_TBLDAT* pNpcTbldat = (sNPC_TBLDAT*)g_pTableContainer->GetNpcTable()->FindData((TBLIDX)byPet);
		if (pNpcTbldat)
		{
			CSummonPet* pPet = (CSummonPet*)g_pObjectManager->CreateCharacter(OBJTYPE_SUMMON_PET);
			if (pPet)
			{
				if (pPet->CreateDataAndSpawn(pPlayer->GetID(), byPet, pNpcTbldat, pPlayer->GetWorldID(), pPlayer->GetCurLoc(), pPlayer->GetCurDir())) pPet->Spawn((CPlayer*)pPlayer);
				else g_pObjectManager->DestroyCharacter(pPet);
			}
		}
	}
}

ACMD(do_relog)
{
	CGameServer* app = (CGameServer*)g_pApp;

	CPlayer* pTarget = pPlayer;
	CPlayer* pTargetPC = g_pObjectManager->GetPC(pTarget->GetTargetHandle());

	if (pTargetPC && pTargetPC->IsInitialized() && pTargetPC->IsPC())
	{
		CNtlPacket packet3(sizeof(sGQ_CLASS_CHANGE));
		sGQ_CLASS_CHANGE* res3 = (sGQ_CLASS_CHANGE*)packet3.GetPacketData();
		res3->wOpCode = GQ_CLASS_CHANGE;
		res3->classChange = true;
		res3->charId = pTargetPC->GetCharID();
		packet3.SetPacketLen(sizeof(sGQ_CLASS_CHANGE));
		app->SendTo(app->GetQueryServerSession(), &packet3);

		CNtlPacket packet(sizeof(sGM_MOVE_REQ));
		sGM_MOVE_REQ* res = (sGM_MOVE_REQ*)packet.GetPacketData();
		res->wOpCode = GM_MOVE_REQ;
		res->accountId = pTargetPC->GetAccountID();
		packet.SetPacketLen(sizeof(sGM_MOVE_REQ));
		app->SendTo(app->GetMasterServerSession(), &packet);
	}
}

ACMD(do_playercount)
{
	CNtlStringW msg;

	CNtlPacket packetMsg(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* resMsg = (sGU_SYSTEM_DISPLAY_TEXT*)packetMsg.GetPacketData();
	resMsg->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	resMsg->byDisplayType = SERVER_TEXT_SYSTEM;
	WCHAR wszFormatBuf[256];
	WCharTLiteralToWCHAR(L"%d Players Online", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
	resMsg->wMessageLengthInUnicode = (WORD)msg.Format(wszFormatBuf, g_pObjectManager->GetPlayerCount());
	NTL_SAFE_WCSCPY(resMsg->awchMessage, msg.c_str());
	pPlayer->SendPacket(&packetMsg);
}

ACMD(do_switch)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();

	CPlayer* pTarget = pPlayer;
	CPlayer* pTargetPC = g_pObjectManager->GetPC(pTarget->GetTargetHandle());

	if (pTargetPC && pTargetPC->IsInitialized() && pTargetPC->IsPC())
	{
		if (pTargetPC->GetLevel() <= 29)
			return;

		TBLIDX SkillId = INVALID_TBLIDX;
		BYTE ClassId = INVALID_BYTE;

		TBLIDX masterPassive = INVALID_TBLIDX;
		TBLIDX itemPassive = INVALID_TBLIDX;
		TBLIDX flightPassive = INVALID_TBLIDX;


		switch (pTargetPC->GetClass())
		{
		case PC_CLASS_HUMAN_FIGHTER:
		case PC_CLASS_STREET_FIGHTER:
		{
			ClassId = 8;
			masterPassive = 829991;
		}
		break;
		case PC_CLASS_SWORD_MASTER:
		{
			ClassId = 7;
			masterPassive = 729991;
		}
		break;
		case PC_CLASS_HUMAN_MYSTIC:
		case PC_CLASS_CRANE_ROSHI:
		{
			ClassId = 10;
			masterPassive = 1029991;
		}
		break;
		case PC_CLASS_TURTLE_ROSHI:
		{
			ClassId = 9;
			masterPassive = 929991;
		}
		break;
		case PC_CLASS_NAMEK_FIGHTER:
		case PC_CLASS_DARK_WARRIOR:
		{
			ClassId = 14;
			masterPassive = 1429991;
		}
		break;
		case PC_CLASS_SHADOW_KNIGHT:
		{
			ClassId = 13;
			masterPassive = 1329991;
		}
		break;
		case PC_CLASS_DENDEN_HEALER:
		{
			ClassId = 16;
			masterPassive = 1629991;
		}
		break;
		case PC_CLASS_NAMEK_MYSTIC:
		case PC_CLASS_POCO_SUMMONER:
		{
			ClassId = 15;
			masterPassive = 1529991;
		}
		break;
		case PC_CLASS_ULTI_MA:
		{
			ClassId = 18;
			masterPassive = 1829991;
		}
		break;
		case PC_CLASS_MIGHTY_MAJIN:
		case PC_CLASS_GRAND_MA:
		{
			ClassId = 17;
			masterPassive = 1729991;
		}
		break;
		case PC_CLASS_WONDER_MAJIN:
		case PC_CLASS_PLAS_MA:
		{
			ClassId = 20;
			masterPassive = 2029991;
		}
		break;
		case PC_CLASS_KAR_MA:
		{
			ClassId = 19;
			masterPassive = 1929991;
		}
		break;
		}

		WORD wTemp;

		pTargetPC->UpdateClass(ClassId);

		pTargetPC->GetSkillManager()->LearnSkill(masterPassive, wTemp, false);
	}
}

ACMD(do_switch2)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();

	CPlayer* pTarget = pPlayer;

	if (pTarget->GetLevel() <= 29)
		return;

	TBLIDX SkillId = INVALID_TBLIDX;
	BYTE ClassId = INVALID_BYTE;

	TBLIDX masterPassive = INVALID_TBLIDX;
	TBLIDX itemPassive = INVALID_TBLIDX;
	TBLIDX flightPassive = INVALID_TBLIDX;


	switch (pTarget->GetClass())
	{
	case PC_CLASS_HUMAN_FIGHTER:
	case PC_CLASS_STREET_FIGHTER:
	{
		ClassId = 8;
		masterPassive = 829991;
	}
	break;
	case PC_CLASS_SWORD_MASTER:
	{
		ClassId = 7;
		masterPassive = 729991;
	}
	break;
	case PC_CLASS_HUMAN_MYSTIC:
	case PC_CLASS_CRANE_ROSHI:
	{
		ClassId = 10;
		masterPassive = 1029991;
	}
	break;
	case PC_CLASS_TURTLE_ROSHI:
	{
		ClassId = 9;
		masterPassive = 929991;
	}
	break;
	case PC_CLASS_NAMEK_FIGHTER:
	case PC_CLASS_DARK_WARRIOR:
	{
		ClassId = 14;
		masterPassive = 1429991;
	}
	break;
	case PC_CLASS_SHADOW_KNIGHT:
	{
		ClassId = 13;
		masterPassive = 1329991;
	}
	break;
	case PC_CLASS_DENDEN_HEALER:
	{
		ClassId = 16;
		masterPassive = 1629991;
	}
	break;
	case PC_CLASS_NAMEK_MYSTIC:
	case PC_CLASS_POCO_SUMMONER:
	{
		ClassId = 15;
		masterPassive = 1529991;
	}
	break;
	case PC_CLASS_ULTI_MA:
	{
		ClassId = 18;
		masterPassive = 1829991;
	}
	break;
	case PC_CLASS_MIGHTY_MAJIN:
	case PC_CLASS_GRAND_MA:
	{
		ClassId = 17;
		masterPassive = 1729991;
	}
	break;
	case PC_CLASS_WONDER_MAJIN:
	case PC_CLASS_PLAS_MA:
	{
		ClassId = 20;
		masterPassive = 2029991;
	}
	break;
	case PC_CLASS_KAR_MA:
	{
		ClassId = 19;
		masterPassive = 1929991;
	}
	break;
	}

	WORD wTemp;

	pTarget->UpdateClass(ClassId);

	pTarget->GetSkillManager()->LearnSkill(masterPassive, wTemp, false);
}

ACMD(do_subclass)
{
	CGameServer* app = (CGameServer*)NtlSfxGetApp();

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE selectedClass = (BYTE)atof(strToken.c_str());


	if (pPlayer && pPlayer->IsInitialized() && pPlayer->IsPC())
	{
		if (pPlayer->GetLevel() < 29)
			return;

		BYTE ClassId1 = INVALID_BYTE;
		BYTE ClassId2 = INVALID_BYTE;

		TBLIDX masterPassive1 = INVALID_TBLIDX;
		TBLIDX masterPassive2 = INVALID_TBLIDX;


		switch (pPlayer->GetClass())
		{
		case PC_CLASS_HUMAN_FIGHTER:
		{
			ClassId1 = 7;
			masterPassive1 = 729991;

			ClassId2 = 8;
			masterPassive2 = 829991;
		}
		break;
		case PC_CLASS_HUMAN_MYSTIC:
		{
			ClassId1 = 9;
			masterPassive1 = 929991;

			ClassId2 = 10;
			masterPassive2 = 1029991;
		}
		break;
		case PC_CLASS_NAMEK_FIGHTER:
		{
			ClassId1 = 13;
			masterPassive1 = 1329991;

			ClassId2 = 14;
			masterPassive2 = 1429991;
		}
		break;
		case PC_CLASS_NAMEK_MYSTIC:
		{
			ClassId1 = 15;
			masterPassive1 = 1529991;

			ClassId2 = 16;
			masterPassive2 = 1629991;
		}
		break;
		case PC_CLASS_MIGHTY_MAJIN:
		{
			ClassId1 = 17;
			masterPassive1 = 1729991;

			ClassId2 = 18;
			masterPassive2 = 1829991;
		}
		break;
		case PC_CLASS_WONDER_MAJIN:
		{
			ClassId1 = 19;
			masterPassive1 = 1929991;

			ClassId2 = 20;
			masterPassive2 = 2029991;
		}
		break;
		}

		WORD wTemp;

		if (selectedClass == 1)
		{
			pPlayer->UpdateClass(ClassId1);
			pPlayer->GetSkillManager()->LearnSkill(masterPassive1, wTemp, false);
		}
		else if (selectedClass == 2)
		{
			pPlayer->UpdateClass(ClassId2);
			pPlayer->GetSkillManager()->LearnSkill(masterPassive2, wTemp, false);
		}
	}
}

ACMD(do_ud)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byMap = (BYTE)atof(strToken.c_str());
	CNtlVector loc;

	if (pPlayer->GetGMLevel() >= 3)
	{
		switch (byMap)
		{
		case 1:
			loc.x = 6351.0; loc.y = 0.0; loc.z = 704.0;
			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
			break;
		case 2:
			loc.x = 878.0; loc.y = 0.0; loc.z = 813.67;
			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
			break;
		case 3:
			loc.x = -690.974; loc.y = 0.0; loc.z = -360.768;
			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
			break;
		case 4:
			loc.x = -358.755; loc.y = 0.0; loc.z = -1818.843;
			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
			break;
		case 5:
			loc.x = 3984.41; loc.y = 0.0; loc.z = 5166.015;
			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
			break;
		case 6:
			loc.x = 3265.021; loc.y = -116.443; loc.z = -5310.495;
			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
			break;
		case 7:
			loc.x = 4548.609863; loc.y = -59.180000; loc.z = 4194.390137;
			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
			break;
		}
	}
	else
	{
		switch (byMap)
		{
		case 7:
			loc.x = 4548.609863; loc.y = -59.180000; loc.z = 4194.390137;
			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
			break;
		}
	}
}

ACMD(do_tmq)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byMap = (BYTE)atof(strToken.c_str());
	CNtlVector loc;

	if (pPlayer->GetGMLevel() == 10)
	{
		switch (byMap)
		{
		case 1:
			loc.x = 5855.0; loc.y = 0.0; loc.z = 1400.0;
			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
			break;
		case 2:
			loc.x = 4021.0; loc.y = 0.0; loc.z = 1630.0;
			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
			break;
		case 3:
			loc.x = 2963.0; loc.y = 0.0; loc.z = 3743.0;
			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
			break;
		case 4:
			loc.x = 320.0; loc.y = 0.0; loc.z = 1825.0;
			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
			break;
		case 5:
			loc.x = -2984.495; loc.y = -90.036; loc.z = -1870.735;
			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
			break;
		case 6:
			loc.x = -474.38; loc.y = -18.376; loc.z = -480.852;
			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 14, TELEPORT_TYPE_COMMAND);
			break;
		case 7:
			loc.x = 4905.741; loc.y = 27.658; loc.z = -3893.05;
			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
			break;
		}
	}
	else
	{

	}
}

ACMD(do_bid)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byMap = (BYTE)atof(strToken.c_str());
	CNtlVector loc;

	if (pPlayer->GetGMLevel() == 10)
	{
		switch (byMap)
		{
		case 1:
			loc.x = 447.929993; loc.y = -63.410000; loc.z = -4122.520020;

			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
			break;
		case 2:
			loc.x = -3662.359863; loc.y = -115.659996; loc.z = -5244.219727;

			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 15, TELEPORT_TYPE_COMMAND);
			break;
		case 3:
			loc.x = -4391.509766; loc.y = -115.329994; loc.z = -5253.569824;

			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 15, TELEPORT_TYPE_COMMAND);
			break;
		case 4:
			loc.x = -3700.469971; loc.y = -115.500000; loc.z = -7155.609863;

			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 15, TELEPORT_TYPE_COMMAND);
			break;
		}
	}
	else
	{
		switch (byMap)
		{
		case 1:
			loc.x = 447.929993; loc.y = -63.410000; loc.z = -4122.520020;

			pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 1, TELEPORT_TYPE_COMMAND);
			break;
		}
	}
}

ACMD(do_cc)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	CNtlVector loc;

	loc.x = 100.799995; loc.y = 0.230000; loc.z = -177.269989;

	pPlayer->StartTeleport(loc, pPlayer->GetCurDir(), 5, TELEPORT_TYPE_COMMAND);
}

ACMD(do_setspeed)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	float fSpeed = (float)atof(strToken.c_str());

	pPlayer->UpdateMoveSpeed(fSpeed, fSpeed);
}

ACMD(do_addmob)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX MobId = (TBLIDX)atof(strToken.c_str());

	sMOB_TBLDAT* pMOBTblData = (sMOB_TBLDAT*)g_pTableContainer->GetMobTable()->FindData(MobId);
	if (pMOBTblData)
	{
		sVECTOR3 spawnloc;
		spawnloc.x = pPlayer->GetCurLoc().x + rand() % 5;
		spawnloc.y = 0.0f;// pPlayer->GetCurLoc().y;
		spawnloc.z = pPlayer->GetCurLoc().z + rand() % 5;

		sVECTOR3 spawndir;
		spawndir.x = pPlayer->GetCurDir().x + rand() % 5;
		spawndir.y = 0.0f;// pPlayer->GetCurDir().y;
		spawndir.z = pPlayer->GetCurDir().z + rand() % 5;

		sSPAWN_TBLDAT sMobSpawn;
		sMobSpawn.vSpawn_Dir.CopyFrom(spawndir);
		sMobSpawn.vSpawn_Loc.CopyFrom(spawnloc);
		sMobSpawn.dwParty_Index = INVALID_DWORD;
		sMobSpawn.byMove_Range = 30;
		sMobSpawn.bySpawn_Move_Type = SPAWN_MOVE_WANDER;
		sMobSpawn.bySpawn_Loc_Range = 30;
		sMobSpawn.byWander_Range = 30;
		sMobSpawn.path_Table_Index = INVALID_TBLIDX;
		sMobSpawn.playScript = INVALID_TBLIDX;
		sMobSpawn.playScriptScene = INVALID_TBLIDX;
		sMobSpawn.aiScript = INVALID_TBLIDX;
		sMobSpawn.aiScriptScene = INVALID_TBLIDX;
		sMobSpawn.actionPatternTblidx = 1;

		CMonster* pMob = (CMonster*)g_pObjectManager->CreateCharacter(OBJTYPE_MOB);
		pMob->CreateDataAndSpawn(pPlayer->GetWorldID(), pMOBTblData, &sMobSpawn, false, 0);
	}
	else ERR_LOG(LOG_GENERAL, "mob not found %u. GM %u", MobId, pPlayer->GetCharID());
}

ACMD(do_addmobgroup)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX groupId = (TBLIDX)atof(strToken.c_str());

	DWORD spawnCount = g_pTableContainer->GetMobSpawnTable(pPlayer->GetCurWorld()->GetIdx())->GetSpawnGroupCount(groupId);
	sSPAWN_TBLDAT* spawnTbldat = g_pTableContainer->GetMobSpawnTable(pPlayer->GetCurWorld()->GetIdx())->GetSpawnGroupFirst(groupId);

	for (DWORD i = 0; i < spawnCount; i++)
	{
		if (spawnTbldat)
		{
			for (int ii = 0; ii < spawnTbldat->bySpawn_Quantity; ii++)
			{
				sMOB_DATA data;
				InitMobData(data);

				data.spawnGroupId = groupId;
				data.worldID = pPlayer->GetCurWorld()->GetID();
				data.worldtblidx = pPlayer->GetCurWorld()->GetIdx();
				data.tblidx = spawnTbldat->mob_Tblidx;
				spawnTbldat->vSpawn_Loc.CopyTo(data.vCurLoc);
				spawnTbldat->vSpawn_Loc.CopyTo(data.vSpawnLoc);
				spawnTbldat->vSpawn_Dir.CopyTo(data.vCurDir);
				spawnTbldat->vSpawn_Dir.CopyTo(data.vSpawnDir);
				data.bySpawnFuncFlag = 0;
				data.sScriptData.playScript = spawnTbldat->playScript;
				data.sScriptData.playScriptScene = spawnTbldat->playScriptScene;
				data.sScriptData.tblidxAiScript = spawnTbldat->aiScript;
				data.sScriptData.tblidxAiScriptScene = spawnTbldat->aiScriptScene;
				data.qwCharConditionFlag = 0;
				data.partyID = spawnTbldat->dwParty_Index;
				data.bPartyLeader = spawnTbldat->bParty_Leader;
				data.byImmortalMode = eIMMORTAL_MODE_OFF;
				data.actionpatternTblIdx = spawnTbldat->actionPatternTblidx;
				data.bySpawnRange = spawnTbldat->bySpawn_Loc_Range;
				data.wSpawnTime = spawnTbldat->wSpawn_Cool_Time;
				data.byMoveType = spawnTbldat->bySpawn_Move_Type;
				data.byWanderRange = spawnTbldat->byWander_Range;
				data.byMoveRange = spawnTbldat->byMove_Range;
				data.pathTblidx = spawnTbldat->path_Table_Index;
				data.hTargetFixedExecuter = INVALID_HOBJECT;
				data.sBotSubData.byNestRange = 20;
				data.sBotSubData.byNestType = NPC_NEST_TYPE_DEFAULT;

				sMOB_TBLDAT* pTbldat = (sMOB_TBLDAT*)g_pTableContainer->GetMobTable()->FindData(data.tblidx);
				if (pTbldat)
				{
					CMonster* pMob = (CMonster*)g_pObjectManager->CreateCharacter(OBJTYPE_MOB);
					if (pMob)
					{
						if (pMob->CreateDataAndSpawn(data, pTbldat))
						{
							pMob->SetStandAlone(false);
						}
						else
						{
							break;
						}
					}
					else
					{
						break;
					}
				}
				else
				{
					ERR_LOG(LOG_SCRIPT, "Could not find MOB-TBLDAT. Tblidx %u Grouptblidx %u", data.tblidx, groupId);
					break;
				}
			}
		}

		spawnTbldat = g_pTableContainer->GetMobSpawnTable(pPlayer->GetCurWorld()->GetIdx())->GetSpawnGroupNext(groupId);
	}
}

ACMD(do_addnpc)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX npcid = (TBLIDX)atof(strToken.c_str());

	sNPC_TBLDAT* pTblData = (sNPC_TBLDAT*)g_pTableContainer->GetNpcTable()->FindData(npcid);

	if (pTblData)
	{
		sVECTOR3 spawnloc;
		spawnloc.x = pPlayer->GetCurLoc().x + rand() % 5;
		spawnloc.y = pPlayer->GetCurLoc().y;
		spawnloc.z = pPlayer->GetCurLoc().z + rand() % 5;

		sVECTOR3 spawndir;
		spawndir.x = pPlayer->GetCurDir().x + rand() % 5;
		spawndir.y = pPlayer->GetCurDir().y;
		spawndir.z = pPlayer->GetCurDir().z + rand() % 5;

		sSPAWN_TBLDAT sSpawn;
		sSpawn.vSpawn_Dir.CopyFrom(spawndir);
		sSpawn.vSpawn_Loc.CopyFrom(spawnloc);
		sSpawn.dwParty_Index = INVALID_DWORD;
		sSpawn.byMove_Range = INVALID_BYTE;
		sSpawn.bySpawn_Move_Type = SPAWN_MOVE_UNKNOWN;
		sSpawn.bySpawn_Loc_Range = 10;
		sSpawn.byWander_Range = INVALID_BYTE;
		sSpawn.path_Table_Index = INVALID_TBLIDX;
		sSpawn.playScript = INVALID_TBLIDX;
		sSpawn.playScriptScene = INVALID_TBLIDX;
		sSpawn.aiScript = INVALID_TBLIDX;
		sSpawn.aiScriptScene = INVALID_TBLIDX;
		sSpawn.actionPatternTblidx = 1;

		CNpc* pNpc = (CNpc*)g_pObjectManager->CreateCharacter(OBJTYPE_NPC);
		pNpc->CreateDataAndSpawn(pPlayer->GetWorldID(), pTblData, &sSpawn, false, 0);
		pNpc->SetStandAlone(false);
		pNpc = pPlayer->GetCurWorld()->FindNpc(npcid);

		if (pNpc == NULL)
		{
			printf("npc not found in world\n");
		}
	}
	else ERR_LOG(LOG_GENERAL, "npc not found %u. GM %u", npcid, pPlayer->GetCharID());
}

ACMD(do_additem)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX ItemId = (TBLIDX)atof(strToken.c_str());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE amount = (BYTE)atof(strToken.c_str());

	if (amount == 0 || amount == INVALID_BYTE)
		amount = 1;

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring wstrName = std::wstring(strToken.begin(), strToken.end());

	CPlayer* pTarget = pPlayer;

	if (wcslen(wstrName.c_str()) > 0)
	{
		pTarget = g_pObjectManager->FindByName(wstrName.c_str());
		if (pTarget == NULL || pTarget->IsInitialized() == false)
			return;
	}

	if (pTarget->GetPlayerItemContainer()->CountEmptyInventory() >= 1)
	{
		sITEM_TBLDAT* pTblData = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(ItemId);
		if (pTblData)
		{
			if (pTblData->bValidity_Able == true)
			{
				if (amount > pTblData->byMax_Stack)
					amount = pTblData->byMax_Stack;

				g_pItemManager->CreateItem(pTarget, ItemId, amount, INVALID_BYTE, INVALID_BYTE, true);
			}
			else NTL_PRINT(PRINT_APP, "GmAddItem(TBLIDX itemid) item not bValidity_Able true ud", ItemId);
		}
		else NTL_PRINT(PRINT_APP, "GmAddItem(TBLIDX itemid) item not found %u", ItemId);
	}
}

ACMD(do_testitems)
{
	CGameServer* app = (CGameServer*)g_pApp;

	std::vector<TBLIDX> ITEMS{ 850636, 850641, 850706, 850711, 850716, 850721, 850726, 850727, 850736, 850737 };
	std::vector<TBLIDX> MATERIALS{ 850748, 850749, 850750, 850751, 850752, 850754 };
	TBLIDX CLASS_ITEM1 = INVALID_TBLIDX;
	TBLIDX CLASS_ITEM2 = INVALID_TBLIDX;

	CPlayer* pTarget = pPlayer;
	CPlayer* pTargetPC = g_pObjectManager->GetPC(pTarget->GetTargetHandle());

	if (pTargetPC && pTargetPC->IsInitialized() && pTargetPC->IsPC())
	{

		switch (pTargetPC->GetClass())
		{
			case PC_CLASS_STREET_FIGHTER:
			case PC_CLASS_SWORD_MASTER: 
				CLASS_ITEM1 = 850646;
				CLASS_ITEM2 = 850651;
				break;
			case PC_CLASS_CRANE_ROSHI:
			case PC_CLASS_TURTLE_ROSHI:
				CLASS_ITEM1 = 850656;
				CLASS_ITEM2 = 850661;
				break;
			case PC_CLASS_DARK_WARRIOR:
			case PC_CLASS_SHADOW_KNIGHT:
				CLASS_ITEM1 = 850666;
				CLASS_ITEM2 = 850671;
				break;
			case PC_CLASS_DENDEN_HEALER:
			case PC_CLASS_POCO_SUMMONER:
				CLASS_ITEM1 = 850676;
				CLASS_ITEM2 = 850681;
				break;
			case PC_CLASS_ULTI_MA:
			case PC_CLASS_GRAND_MA:
				CLASS_ITEM1 = 850686;
				CLASS_ITEM2 = 850691;
				break;
			case PC_CLASS_PLAS_MA:
			case PC_CLASS_KAR_MA:
				CLASS_ITEM1 = 850696;
				CLASS_ITEM2 = 850701;
				break;
		}

		if (pTargetPC->GetPlayerItemContainer()->CountEmptyInventory() >= 20)
		{
			for (BYTE i = 0; i < (int)ITEMS.size(); i++)
			{
				sITEM_TBLDAT* pTblData = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(ITEMS[i]);
				if (pTblData)
				{
					if (pTblData->bValidity_Able == true)
					{
						g_pItemManager->CreateItem(pTargetPC, ITEMS[i], 1, INVALID_BYTE, INVALID_BYTE, pTblData->Item_Option_Tblidx == INVALID_TBLIDX);

						CItem* pItem = pTargetPC->GetPlayerItemContainer()->GetItemByIdx(ITEMS[i]);
						if (pItem && pItem->GetDurationtype() == eDURATIONTYPE_FLATSUM)
						{
							pItem->SetUseEndTime(time(0) + 3800);

							CNtlPacket pQry(sizeof(sGQ_ITEM_CHANGE_DURATIONTIME_REQ));
							sGQ_ITEM_CHANGE_DURATIONTIME_REQ* rQry = (sGQ_ITEM_CHANGE_DURATIONTIME_REQ*)pQry.GetPacketData();
							rQry->wOpCode = GQ_ITEM_CHANGE_DURATIONTIME_REQ;
							rQry->charId = pTargetPC->GetCharID();
							rQry->handle = pTargetPC->GetID();
							memcpy(&rQry->sItem, &pItem->GetItemData(), sizeof(sITEM_DATA));
							pQry.SetPacketLen(sizeof(sGQ_ITEM_CHANGE_DURATIONTIME_REQ));
							app->SendTo(app->GetQueryServerSession(), &pQry);
						}
					}
				}
			}

			for (BYTE i = 0; i < (int)MATERIALS.size(); i++)
			{
				sITEM_TBLDAT* pTblData = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(MATERIALS[i]);
				if (pTblData)
				{
					if (pTblData->bValidity_Able == true)
					{
						g_pItemManager->CreateItem(pTargetPC, MATERIALS[i], 10, INVALID_BYTE, INVALID_BYTE, pTblData->Item_Option_Tblidx == INVALID_TBLIDX);
					}
				}
			}

			sITEM_TBLDAT* pTblData = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(CLASS_ITEM1);
			if (pTblData)
			{
				if (pTblData->bValidity_Able == true)
				{
					g_pItemManager->CreateItem(pTargetPC, CLASS_ITEM1, 1, INVALID_BYTE, INVALID_BYTE, pTblData->Item_Option_Tblidx == INVALID_TBLIDX);

					CItem* pItem = pTargetPC->GetPlayerItemContainer()->GetItemByIdx(CLASS_ITEM1);
					if (pItem && pItem->GetDurationtype() == eDURATIONTYPE_FLATSUM)
					{
						pItem->SetUseEndTime(time(0) + 3800);

						CNtlPacket pQry(sizeof(sGQ_ITEM_CHANGE_DURATIONTIME_REQ));
						sGQ_ITEM_CHANGE_DURATIONTIME_REQ* rQry = (sGQ_ITEM_CHANGE_DURATIONTIME_REQ*)pQry.GetPacketData();
						rQry->wOpCode = GQ_ITEM_CHANGE_DURATIONTIME_REQ;
						rQry->charId = pTargetPC->GetCharID();
						rQry->handle = pTargetPC->GetID();
						memcpy(&rQry->sItem, &pItem->GetItemData(), sizeof(sITEM_DATA));
						pQry.SetPacketLen(sizeof(sGQ_ITEM_CHANGE_DURATIONTIME_REQ));
						app->SendTo(app->GetQueryServerSession(), &pQry);
					}
				}
			}

			pTblData = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(CLASS_ITEM2);
			if (pTblData)
			{
				if (pTblData->bValidity_Able == true)
				{
					g_pItemManager->CreateItem(pTargetPC, CLASS_ITEM2, 1, INVALID_BYTE, INVALID_BYTE, pTblData->Item_Option_Tblidx == INVALID_TBLIDX);

					CItem* pItem = pTargetPC->GetPlayerItemContainer()->GetItemByIdx(CLASS_ITEM2);
					if (pItem && pItem->GetDurationtype() == eDURATIONTYPE_FLATSUM)
					{
						pItem->SetUseEndTime(time(0) + 3800);

						CNtlPacket pQry(sizeof(sGQ_ITEM_CHANGE_DURATIONTIME_REQ));
						sGQ_ITEM_CHANGE_DURATIONTIME_REQ* rQry = (sGQ_ITEM_CHANGE_DURATIONTIME_REQ*)pQry.GetPacketData();
						rQry->wOpCode = GQ_ITEM_CHANGE_DURATIONTIME_REQ;
						rQry->charId = pTargetPC->GetCharID();
						rQry->handle = pTargetPC->GetID();
						memcpy(&rQry->sItem, &pItem->GetItemData(), sizeof(sITEM_DATA));
						pQry.SetPacketLen(sizeof(sGQ_ITEM_CHANGE_DURATIONTIME_REQ));
						app->SendTo(app->GetQueryServerSession(), &pQry);
					}
				}
			}
		}
	}
}

ACMD(do_itemrange)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX ItemId = (TBLIDX)atof(strToken.c_str());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX ItemId2 = (TBLIDX)atof(strToken.c_str());

	BYTE amount = 1;

	CPlayer* pTarget = pPlayer;

	for (int i = ItemId; i < ItemId2 + 1; i++)
	{
		if (pTarget->GetPlayerItemContainer()->CountEmptyInventory() >= 1)
		{
			sITEM_TBLDAT* pTblData = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(i);
			if (pTblData)
			{
				if (pTblData->bValidity_Able == true)
				{
					if (amount > pTblData->byMax_Stack)
						amount = pTblData->byMax_Stack;

					g_pItemManager->CreateItem(pTarget, i, amount, INVALID_BYTE, INVALID_BYTE, pTblData->Item_Option_Tblidx == INVALID_TBLIDX);
				}
				else NTL_PRINT(PRINT_APP, "GmAddItem(TBLIDX itemid) item not bValidity_Able true ud", i);
			}
			else NTL_PRINT(PRINT_APP, "GmAddItem(TBLIDX itemid) item not found %u", i);
		}
	}
}

ACMD(do_addmasteritem)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE bySecondClass = (BYTE)atof(strToken.c_str());

	TBLIDX itemTblidx = INVALID_TBLIDX;

	if (pPlayer->GetClass() > PC_CLASS_1_LAST)
		return;

	switch (pPlayer->GetClass())
	{
		case PC_CLASS_HUMAN_FIGHTER:
		{
			switch (bySecondClass)
			{
				case PC_CLASS_STREET_FIGHTER: itemTblidx = 99078; break;
				case PC_CLASS_SWORD_MASTER: itemTblidx = 99079; break;

				default: return; break;
			}
		}
		break;
		case PC_CLASS_HUMAN_MYSTIC:
		{
			switch (bySecondClass)
			{
				case PC_CLASS_CRANE_ROSHI: itemTblidx = 99081; break;
				case PC_CLASS_TURTLE_ROSHI: itemTblidx = 99080; break;

				default: return; break;
			}
		}
		break;
		case PC_CLASS_NAMEK_FIGHTER:
		{
			switch (bySecondClass)
			{
				case PC_CLASS_DARK_WARRIOR: itemTblidx = 99082; break;
				case PC_CLASS_SHADOW_KNIGHT: itemTblidx = 99083; break;

				default: return; break;
			}
		}
		break;
		case PC_CLASS_NAMEK_MYSTIC:
		{
			switch (bySecondClass)
			{
				case PC_CLASS_DENDEN_HEALER: itemTblidx = 99084; break;
				case PC_CLASS_POCO_SUMMONER: itemTblidx = 99085; break;

				default: return; break;
			}
		}
		break;
		case PC_CLASS_MIGHTY_MAJIN:
		{
			switch (bySecondClass)
			{
				case PC_CLASS_ULTI_MA: itemTblidx = 99086; break;
				case PC_CLASS_GRAND_MA: itemTblidx = 99087; break;

				default: return; break;
			}
		}
		break;
		case PC_CLASS_WONDER_MAJIN:
		{
			switch (bySecondClass)
			{
				case PC_CLASS_PLAS_MA: itemTblidx = 99088; break;
				case PC_CLASS_KAR_MA: itemTblidx = 99089; break;

				default: return; break;
			}
		}
		break;

		default: return; break;
	}

	sITEM_TBLDAT* pTblData = (sITEM_TBLDAT*)g_pTableContainer->GetItemTable()->FindData(itemTblidx);
	if (pTblData)
	{
		if (pPlayer->GetPlayerItemContainer()->GetItemByIdx(itemTblidx) == NULL)
		{
			std::pair<BYTE, BYTE> inv = pPlayer->GetPlayerItemContainer()->GetEmptyInventory();
			if (inv.first != INVALID_BYTE && inv.second != INVALID_BYTE)
				g_pItemManager->CreateItem(pPlayer, itemTblidx, 1, inv.first, inv.second);
		}
	}
}

ACMD(do_adddefaultbank)
{
	CGameServer * app = (CGameServer*)g_pApp;

	if (pPlayer->IsUsingBank())	//check if has the bank open
		return;
	if (pPlayer->IsBankLoaded())
		return;

	pPlayer->GetPlayerItemContainer()->AddReservedInventory(CONTAINER_TYPE_BANKSLOT, 0);

	CNtlPacket packetQry(sizeof(sGQ_BANK_ADD_WITH_COMMAND_REQ));
	sGQ_BANK_ADD_WITH_COMMAND_REQ* res = (sGQ_BANK_ADD_WITH_COMMAND_REQ*)packetQry.GetPacketData();
	res->wOpCode = GQ_BANK_ADD_WITH_COMMAND_REQ;
	res->handle = pPlayer->GetID();
	res->charId = pPlayer->GetCharID();
	res->itemNo = 19991; //basic warehouse tblidx
	packetQry.SetPacketLen(sizeof(sGQ_BANK_ADD_WITH_COMMAND_REQ));
	app->SendTo(app->GetQueryServerSession(), &packetQry);
}

ACMD(do_addskill)
{
	CGameServer * app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX SkillId = (TBLIDX)atof(strToken.c_str());

	WORD wTemp;
	if (pPlayer->GetSkillManager())
		pPlayer->GetSkillManager()->LearnSkill(SkillId, wTemp, false);
}

ACMD(do_addskill2)
{
	CGameServer * app = (CGameServer*)NtlSfxGetApp();

	CPlayer* pTarget = pPlayer;
	CPlayer* pTargetPC = g_pObjectManager->GetPC(pTarget->GetTargetHandle());

	if (pTargetPC && pTargetPC->IsInitialized() && pTargetPC->IsPC())
	{
		TBLIDX SkillId = INVALID_TBLIDX;

		switch (pTargetPC->GetClass())
		{
		case PC_CLASS_STREET_FIGHTER: SkillId = 729991; break;
		case PC_CLASS_SWORD_MASTER: SkillId = 829991; break;
		case PC_CLASS_CRANE_ROSHI: SkillId = 929991; break;
		case PC_CLASS_TURTLE_ROSHI: SkillId = 1029991; break;
		case PC_CLASS_DARK_WARRIOR: SkillId = 1329991; break;
		case PC_CLASS_SHADOW_KNIGHT: SkillId = 1429991; break;
		case PC_CLASS_DENDEN_HEALER: SkillId = 1529991; break;
		case PC_CLASS_POCO_SUMMONER: SkillId = 1629991; break;
		case PC_CLASS_ULTI_MA: SkillId = 1729991; break;
		case PC_CLASS_GRAND_MA: SkillId = 1829991; break;
		case PC_CLASS_PLAS_MA: SkillId = 1929991; break;
		case PC_CLASS_KAR_MA: SkillId = 2029991; break;

		default: return; break;
		}

		WORD wTemp;


		if (pTargetPC->GetSkillManager())
			pTargetPC->GetSkillManager()->LearnSkill(SkillId, wTemp, false);
	}
}

ACMD(do_r)
{
	pPlayer->UpdateCurLpEp(pPlayer->GetLastMaxLP(), pPlayer->GetLastMaxEP(), true, false);
}

ACMD(do_addhtb)
{
	CGameServer * app = (CGameServer*)NtlSfxGetApp();

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX id = (TBLIDX)atof(strToken.c_str());
	WORD wTemp;

	if (pPlayer->IsGameMaster() == false)
	{
		switch (pPlayer->GetClass())
		{
		case PC_CLASS_STREET_FIGHTER:
		case PC_CLASS_SWORD_MASTER:
			id = 30611;
			break;

		case PC_CLASS_CRANE_ROSHI:
		case PC_CLASS_TURTLE_ROSHI:
			id = 130411;
			break;

		case PC_CLASS_DARK_WARRIOR:
		case PC_CLASS_SHADOW_KNIGHT:
			id = 330611;
			break;

		case PC_CLASS_DENDEN_HEALER:
		case PC_CLASS_POCO_SUMMONER:
			id = 430411;
			break;

		case PC_CLASS_ULTI_MA:
		case PC_CLASS_GRAND_MA:
			id = 532011;
			break;

		case PC_CLASS_PLAS_MA:
		case PC_CLASS_KAR_MA:
			id = 632011;
			break;
		}
	}

	pPlayer->GetHtbSkillManager()->LearnHtbSkill(id, wTemp);
}

ACMD(do_setzenny)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	DWORD zeni = (DWORD)atof(strToken.c_str());

	ERR_LOG(LOG_USER, "Player: %u receive %u zeni from gm command", pPlayer->GetCharID(), zeni);

	pPlayer->UpdateZeni(ZENNY_CHANGE_TYPE_CHEAT, zeni, true);
}

ACMD(do_setlevel)
{
	CGameServer * app = (CGameServer*)NtlSfxGetApp();

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE level = (BYTE)atof(strToken.c_str());

	CPlayer* pTarget = pPlayer;
	CPlayer* pTargetPC = g_pObjectManager->GetPC(pTarget->GetTargetHandle());

	if (pTargetPC && pTargetPC->IsInitialized() && pTargetPC->IsPC())
	{
		RwUInt32 curlv = pTarget->GetLevel();

		sEXP_TBLDAT* ExpData = (sEXP_TBLDAT*)g_pTableContainer->GetExpTable()->FindData(level);
		if (!ExpData)
			return;


		CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_LEVEL));
		sGU_UPDATE_CHAR_LEVEL* res = (sGU_UPDATE_CHAR_LEVEL*)packet.GetPacketData();
		res->wOpCode = GU_UPDATE_CHAR_LEVEL;
		res->byCurLevel = level;
		res->byPrevLevel = curlv;
		res->dwMaxExpInThisLevel = ExpData->dwNeed_Exp;
		res->handle = pTargetPC->GetID();
		packet.SetPacketLen(sizeof(sGU_UPDATE_CHAR_LEVEL));
		pTargetPC->Broadcast(&packet);

		DWORD getsp = Dbo_GetLevelUpGainSP(level, level - curlv);
		pTargetPC->SetLevel(level);
		pTargetPC->UpdateCharSP((pTargetPC->GetLevel() - 1) + pTargetPC->GetSkillPointsBought());
		pTargetPC->GetCharAtt()->CalculateAll();

		pTargetPC->UpdateCurLpEp(pTargetPC->GetLastMaxLP(), pTargetPC->GetLastMaxEP(), true, false);

		pTargetPC->UpdateMaxRpBalls();

		//send to chat server
		app->GetChatServerSession()->SendUpdatePcLevel(pTargetPC);

		//update party
		if (pTargetPC->GetPartyID() != INVALID_PARTYID && pTargetPC->GetParty())
			pTargetPC->GetParty()->UpdateMemberLevel(pTargetPC);

		//send to query server
		CNtlPacket packetQry(sizeof(sGQ_PC_UPDATE_LEVEL_REQ));
		sGQ_PC_UPDATE_LEVEL_REQ* resQry = (sGQ_PC_UPDATE_LEVEL_REQ*)packetQry.GetPacketData();
		resQry->wOpCode = GQ_PC_UPDATE_LEVEL_REQ;
		resQry->handle = pTargetPC->GetID();
		resQry->charId = pTargetPC->GetCharID();
		resQry->dwEXP = 0;
		resQry->byLevel = pTargetPC->GetLevel();
		resQry->dwSP = pTargetPC->GetSkillPoints();
		packetQry.SetPacketLen(sizeof(sGQ_PC_UPDATE_LEVEL_REQ));
		app->SendTo(app->GetQueryServerSession(), &packetQry);
	}
}

ACMD(do_hide)
{
	if (pPlayer->GetStateManager()->IsCharCondition(CHARCOND_TRANSPARENT))
		pPlayer->GetStateManager()->RemoveConditionState(CHARCOND_TRANSPARENT, NULL, true);
	else
		pPlayer->GetStateManager()->AddConditionState(CHARCOND_TRANSPARENT, NULL, true);
}

ACMD(do_notice)
{
	/*
		@notice TYPE(0-6) TEXT(MAX 256 CHARACTERS)
	*/
	
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byDisType = (BYTE)atof(strToken.c_str());

	pToken->PopToPeek();
	std::string text = "";

	while (text.length() < NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE)
	{
		std::string mtext = pToken->PeekNextToken(NULL, &iLine);
		if (mtext == ";" || mtext == "")
		{
			break;
		}
		else
		{
			text += mtext; text += " ";
		}
	}

	CGameServer* app = (CGameServer*)g_pApp;

	CNtlPacket packet(sizeof(sGT_SYSTEM_DISPLAY_TEXT));
	sGT_SYSTEM_DISPLAY_TEXT* res = (sGT_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GT_SYSTEM_DISPLAY_TEXT;
	res->serverChannelId = INVALID_SERVERCHANNELID;
	res->byDisplayType = byDisType;
	NTL_WCSCPY_S(res->wszMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE + 1, s2ws(text).c_str());
	packet.SetPacketLen(sizeof(sGT_SYSTEM_DISPLAY_TEXT));
	app->SendTo(app->GetChatServerSession(), &packet);
}

ACMD(do_pm)
{
	pToken->PopToPeek();
	std::string strName = pToken->PeekNextToken(NULL, &iLine);

	pToken->PopToPeek();
	std::string text = "";

	while (text.length() < NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE)
	{
		std::string mtext = pToken->PeekNextToken(NULL, &iLine);
		if (mtext == ";" || mtext == "")
		{
			break;
		}
		else
		{
			text += mtext; text += " ";
		}
	}

	if (CPlayer* pTarget = g_pObjectManager->FindByName(s2ws(strName).c_str()))
	{
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT * res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		res->wMessageLengthInUnicode = (WORD)text.length();
		res->byDisplayType = SERVER_TEXT_EMERGENCY;
		NTL_WCSCPY_S(res->awchMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE + 1, s2ws(text).c_str());
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pTarget->SendPacket(&packet);
	}
}

ACMD(do_teleport)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	int Portal = (BYTE)atof(strToken.c_str());
	sPORTAL_TBLDAT* pPortalTblData = (sPORTAL_TBLDAT*)g_pTableContainer->GetPortalTable()->FindData(Portal);
	if (pPortalTblData == NULL)
		return;

	pPlayer->StartTeleport(pPortalTblData->vLoc, pPortalTblData->vDir, pPortalTblData->worldId, TELEPORT_TYPE_COMMAND);
}

ACMD(do_world)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	WORLDID worldId = (WORLDID)atof(strToken.c_str());

	CGameServer* app = (CGameServer*)g_pApp;

	sWORLD_TBLDAT* pWorldTbldat = (sWORLD_TBLDAT*)g_pTableContainer->GetWorldTable()->FindData(worldId);
	if (pWorldTbldat == NULL)
	{
		return;
	}

	if (CWorld* pWorld = app->GetGameMain()->GetWorldManager()->FindWorld(worldId))
	{
		pPlayer->StartTeleport(pWorld->GetTbldat()->vDefaultLoc, pPlayer->GetCurDir(), worldId, TELEPORT_TYPE_COMMAND);
		pWorld->AddScriptToPlayer(pPlayer);
	}
	else
	{
		CWorld* pWorld2 = app->GetGameMain()->GetWorldManager()->CreateWorld(pWorldTbldat);
		if (pWorld2 == NULL)
			return;

		pPlayer->StartTeleport(pWorld2->GetTbldat()->vStart1Loc, pPlayer->GetCurDir(), pWorld2->GetID(), TELEPORT_TYPE_COMMAND);
		pWorld2->AddScriptToPlayer(pPlayer);
	}
}


ACMD(do_warp)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);

	std::wstring name = std::wstring(strToken.begin(), strToken.end());
	const wchar_t* wname = name.c_str();

	CCharacter* target = g_pObjectManager->FindByName(wname);
	if (target && target->GetCurWorld() && target->GetCurWorld()->GetTbldat()->bDynamic == false) //avoid teleporting by gm code into dungeon
		pPlayer->StartTeleport(target->GetCurLoc(), target->GetCurDir(), target->GetWorldID(), TELEPORT_TYPE_COMMAND);
}

ACMD(do_call)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);

	std::wstring name = std::wstring(strToken.begin(), strToken.end());
	const wchar_t* wname = name.c_str();

	CCharacter* target = g_pObjectManager->FindByName(wname);
	if (target && target->GetCurWorld() && target->GetCurWorld()->GetTbldat()->bDynamic == false) //avoid teleporting by gm code into dungeon
		target->StartTeleport(pPlayer->GetCurLoc(), pPlayer->GetCurDir(), pPlayer->GetWorldID(), TELEPORT_TYPE_COMMAND);
}

ACMD(do_shutdown)
{
	CGameServer * app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE min = (BYTE)atof(strToken.c_str());

	std::string text = "";

	text += "Server will shutdown in " + std::to_string(min); text += " minutes!";

	CNtlPacket packet(sizeof(sGT_SYSTEM_DISPLAY_TEXT));
	sGT_SYSTEM_DISPLAY_TEXT* res = (sGT_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GT_SYSTEM_DISPLAY_TEXT;
	res->serverChannelId = INVALID_SERVERCHANNELID;
	res->byDisplayType = 1;
	NTL_WCSCPY_S(res->wszMessage, NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE + 1, s2ws(text).c_str());
	packet.SetPacketLen(sizeof(sGT_SYSTEM_DISPLAY_TEXT));
	app->SendTo(app->GetChatServerSession(), &packet);

	DWORD dwCurTick = GetTickCount();
	DWORD tick = dwCurTick + 10000;//300000;
	bool shutdown = false;
	/*
	switch (min)
	{
	case 5: { tick = dwCurTick + 300000; } break;
	case 10: { tick = dwCurTick + 600000; } break;
	case 15: { tick = dwCurTick + 900000; } break;
	case 20: { tick = dwCurTick + 1200000; } break;
	case 25: { tick = dwCurTick + 1500000; } break;
	case 30: { tick = dwCurTick + 1800000; } break;

	default: { tick = dwCurTick + 300000; } break;
	}
	*/
	

	while (shutdown == false)
	{
		if (tick < dwCurTick)
		{
			shutdown == true;
			app->GetGameProcessor()->StartServerShutdownEvent();
		}
	}
}

ACMD(do_setadult)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	int n = (int)atof(strToken.c_str());
	bool bAdultSet = n != 0;

	pPlayer->UpdateAdult(bAdultSet);
}

ACMD(do_setclass)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byClass = (BYTE)atof(strToken.c_str());

	//if (pPlayer->GetClass() > PC_CLASS_1_LAST)
	//	return;
	/*
	switch (pPlayer->GetClass())
	{
		case PC_CLASS_HUMAN_FIGHTER:
		case PC_CLASS_STREET_FIGHTER:
		case PC_CLASS_SWORD_MASTER:
		{
			if (byClass != PC_CLASS_STREET_FIGHTER && byClass != PC_CLASS_SWORD_MASTER)
				return;
		}
		break;
		case PC_CLASS_HUMAN_MYSTIC:
		case PC_CLASS_CRANE_ROSHI:
		case PC_CLASS_TURTLE_ROSHI:
		{
			if (byClass != PC_CLASS_CRANE_ROSHI && byClass != PC_CLASS_TURTLE_ROSHI)
				return;
		}
		break;
		case PC_CLASS_NAMEK_FIGHTER:
		case PC_CLASS_DARK_WARRIOR:
		case PC_CLASS_SHADOW_KNIGHT:
		{
			if (byClass != PC_CLASS_DARK_WARRIOR && byClass != PC_CLASS_SHADOW_KNIGHT)
				return;
		}
		break;
		case PC_CLASS_NAMEK_MYSTIC:
		case PC_CLASS_DENDEN_HEALER:
		case PC_CLASS_POCO_SUMMONER:
		{
			if (byClass != PC_CLASS_DENDEN_HEALER && byClass != PC_CLASS_POCO_SUMMONER)
				return;
		}
		break;
		case PC_CLASS_MIGHTY_MAJIN:
		case PC_CLASS_ULTI_MA:
		case PC_CLASS_GRAND_MA:
		{
			if (byClass != PC_CLASS_ULTI_MA && byClass != PC_CLASS_GRAND_MA)
				return;
		}
		break;
		case PC_CLASS_WONDER_MAJIN:
		case PC_CLASS_PLAS_MA:
		case PC_CLASS_KAR_MA:
		{
			if (byClass != PC_CLASS_PLAS_MA && byClass != PC_CLASS_KAR_MA)
				return;
		}
		break;

		default: return;
	}
	*/
	

	pPlayer->UpdateClass(byClass);
}

ACMD(do_dc)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring name = std::wstring(strToken.begin(), strToken.end());
	const wchar_t* wname = name.c_str();

	CPlayer* target = g_pObjectManager->FindByName(wname);
	if (target && target->IsInitialized())
		target->GetClientSession()->Disconnect(false);
}

ACMD(do_kill)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring name = std::wstring(strToken.begin(), strToken.end());
	const wchar_t* wname = name.c_str();

	CPlayer* target = g_pObjectManager->FindByName(wname);
	if (target && target->IsInitialized())
		target->Faint(pPlayer, FAINT_REASON_COMMAND);
}

ACMD(do_delallitems)
{
	pPlayer->GetPlayerItemContainer()->DeleteAllItems();
}

ACMD(do_god)
{
	pPlayer->GetCharAtt()->SetLastPhysicalOffence(INVALID_WORD);
	pPlayer->GetCharAtt()->SetLastPhysicalDefence(INVALID_WORD);
	pPlayer->GetCharAtt()->SetLastEnergyOffence(INVALID_WORD);

	pPlayer->UpdateAttackSpeed(100);

	if (pPlayer->GetImmortalMode() == eIMMORTAL_MODE_OFF)
		pPlayer->SetImmortalMode(eIMMORTAL_MODE_NORMAL);
	else
		pPlayer->SetImmortalMode(eIMMORTAL_MODE_OFF);

	pPlayer->UpdateMoveSpeed(25.f, 25.f);
}


ACMD(do_invincible)
{
	/*
		@invincible DURATION(SECONDS)
	*/

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	int nSeconds = (int)atof(strToken.c_str());

	if (nSeconds == 0 || nSeconds > 3600)
		nSeconds = 3600;

	sDBO_BUFF_PARAMETER aBuffParameter[NTL_MAX_EFFECT_IN_SKILL];
	sSKILL_TBLDAT* pTempSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(2385);

	eSYSTEM_EFFECT_CODE aeEffectCode[NTL_MAX_EFFECT_IN_SKILL];
	aeEffectCode[0] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pTempSkillTbldat->skill_Effect[0]);
	aeEffectCode[1] = INVALID_SYSTEM_EFFECT_CODE;

	aBuffParameter[0].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DEFAULT;
	aBuffParameter[0].buffParameter.fParameter = 0;
	aBuffParameter[0].buffParameter.dwRemainValue = 0;

	DWORD dwDurationInMs = nSeconds * 1000;

	pPlayer->GetBuffManager()->RegisterBuff(dwDurationInMs, aeEffectCode, aBuffParameter, INVALID_HOBJECT, BUFF_TYPE_BLESS, pTempSkillTbldat);
}


ACMD(do_bann)
{
	CGameServer* app = (CGameServer*)g_pApp;

	/*
		@bann USERNAME DURATION(DAYS. 255 = PERMA) REASON(MAX 256 CHARACTERS)
	*/
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring name = std::wstring(strToken.begin(), strToken.end());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byDuration = (BYTE)atof(strToken.c_str());

	pToken->PopToPeek();
	std::string text = "";

	while (text.length() < NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE)
	{
		std::string mtext = pToken->PeekNextToken(NULL, &iLine);
		if (mtext == ";" || mtext == "")
		{
			break;
		}
		else
		{
			text += mtext;
			text += " ";
		}
	}

	CPlayer* target = g_pObjectManager->FindByName(name.c_str());
	if (target && target->IsInitialized())
	{
		target->Bann(text, byDuration, pPlayer->GetAccountID());
	}
}


ACMD(do_dbann)
{
	CGameServer* app = (CGameServer*)g_pApp;

	/*
		@dbann ACCOUNT_ID DURATION(DAYS. 255 = PERMA) REASON(MAX 256 CHARACTERS)
	*/
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	ACCOUNTID accid = (ACCOUNTID)atof(strToken.c_str());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byDuration = (BYTE)atof(strToken.c_str());

	pToken->PopToPeek();
	std::string text = "";

	while (text.length() < NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE)
	{
		std::string mtext = pToken->PeekNextToken(NULL, &iLine);
		if (mtext == ";" || mtext == "")
		{
			break;
		}
		else
		{
			text += mtext;
			text += " ";
		}
	}

	CNtlPacket pQry(sizeof(sGQ_ACCOUNT_BANN));
	sGQ_ACCOUNT_BANN * qRes = (sGQ_ACCOUNT_BANN *)pQry.GetPacketData();
	qRes->wOpCode = GQ_ACCOUNT_BANN;
	qRes->gmAccountID = pPlayer->GetAccountID();
	qRes->targetAccountID = accid;
	snprintf(qRes->szReason, NTL_MAX_LENGTH_OF_CHAT_MESSAGE_UNICODE + 1, "%s", text.c_str());
	qRes->byDuration = byDuration;
	pQry.SetPacketLen(sizeof(sGQ_ACCOUNT_BANN));
	app->SendTo(app->GetQueryServerSession(), &pQry);
}


ACMD(do_purge) //despawn all monster around player
{
	CWorldCell* pWorldCell = pPlayer->GetCurWorldCell();
	if (!pWorldCell)
		return;

	CMonster* pNextMob = NULL;
	CNpc* pNextNpc = NULL;


	CWorldCell::QUADPAGE page = pWorldCell->GetCellQuadPage(pPlayer->GetCurLoc());
	for (int dir = CWorldCell::QUADPAGE_FIRST; dir < CWorldCell::QUADPAGE_COUNT; dir++)
	{
		CWorldCell* pWorldCellSibling = pWorldCell->GetQuadSibling(page, (CWorldCell::QUADDIR)dir);
		if (pWorldCellSibling)
		{
			CMonster* pMobTarget = (CMonster*)pWorldCellSibling->GetObjectList()->GetFirst(OBJTYPE_MOB);
			while (pMobTarget)
			{
				pNextMob = (CMonster*)pWorldCellSibling->GetObjectList()->GetNext(pMobTarget->GetWorldCellObjectLinker());

				if (pMobTarget->GetCurWorld() && !pMobTarget->IsFainting())
					pMobTarget->Faint(pPlayer);

				pMobTarget = pNextMob;
			}

			CNpc* pNpcTarget = (CMonster*)pWorldCellSibling->GetObjectList()->GetFirst(OBJTYPE_NPC);
			while (pNpcTarget)
			{
				pNextNpc = (CMonster*)pWorldCellSibling->GetObjectList()->GetNext(pNpcTarget->GetWorldCellObjectLinker());

				if (pNpcTarget->GetCurWorld() && !pNpcTarget->IsFainting() && !pNpcTarget->GetStandAlone())
					pNpcTarget->Faint(pPlayer);

				pNpcTarget = pNextNpc;
			}
		}
	}
}

ACMD(do_unstack)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);

	if (!pPlayer->GetClientSession())
		return;

	if (pPlayer->IsFainting())
		return;
	if (pPlayer->GetFreeBattleID() != INVALID_DWORD)
		return;
	if (pPlayer->IsPvpZone() || pPlayer->GetFightMode())
		return;
	if (pPlayer->GetCurWorld() && pPlayer->GetCurWorld()->GetTbldat()->bDynamic)
		return;
	if (pPlayer->GetDragonballScrambleBallFlag() > 0)
		return;

	CGameServer* app = (CGameServer*)g_pApp;

	if (app->GetGsServerId() == DOJO_CHANNEL_INDEX)
		return;

	if (CSkill* pSkill = pPlayer->GetSkillManager()->FindSkillWithSystemEffectCode(ACTIVE_TELEPORT_BIND)) //dont allow to use unstack while call back skill is on cooldown
	{
		if (pSkill->GetCoolTimeRemaining() > 0)
			return;
	}

	if (pPlayer->GetCanUnstack() == true)
	{
		pPlayer->SetCanUnstack(false);
		CNtlVector vBindLoc(pPlayer->GetBindLoc());
		pPlayer->StartTeleport(vBindLoc, pPlayer->GetCurDir(), pPlayer->GetBindWorldID(), TELEPORT_TYPE_COMMAND);
	}
}

ACMD(do_setnetpy)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	DWORD dwNetpy = (DWORD)atof(strToken.c_str());

	pPlayer->UpdateNetpyToken(false, dwNetpy, true);
}


ACMD(do_gethlsitem) //used to test
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX itemTblidx = (TBLIDX)atof(strToken.c_str());

	sHLS_ITEM_TBLDAT* pTblData = (sHLS_ITEM_TBLDAT*)g_pTableContainer->GetHLSItemTable()->FindData(itemTblidx);
	if (pTblData)
	{
		CNtlPacket packetQry(sizeof(sGQ_CASHITEM_BUY_REQ));
		sGQ_CASHITEM_BUY_REQ* resQry = (sGQ_CASHITEM_BUY_REQ*)packetQry.GetPacketData();
		resQry->wOpCode = GQ_CASHITEM_BUY_REQ;
		resQry->accountId = pPlayer->GetAccountID();
		resQry->characterId = pPlayer->GetCharID();
		resQry->handle = pPlayer->GetID();
		resQry->byCount = pTblData->byStackCount;
		resQry->dwPrice = pPlayer->GetItemShopCash();
		resQry->HLSitemTblidx = pTblData->tblidx;
		packetQry.SetPacketLen(sizeof(sGQ_CASHITEM_BUY_REQ));
		app->SendTo(app->GetQueryServerSession(), &packetQry);
	}
}

ACMD(do_skillspeed)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	float fSpeed = (float)atof(strToken.c_str());

	pPlayer->SetLastSkillAnimationSpeedModifier(fSpeed);

	CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_SKILL_ANIMATION_SPEED_MODIFIER));
	sGU_UPDATE_CHAR_SKILL_ANIMATION_SPEED_MODIFIER * res = (sGU_UPDATE_CHAR_SKILL_ANIMATION_SPEED_MODIFIER *)packet.GetPacketData();
	res->wOpCode = GU_UPDATE_CHAR_SKILL_ANIMATION_SPEED_MODIFIER;
	res->hSubject = pPlayer->GetID();
	res->fSkillAnimationSpeedModifier = fSpeed;
	pPlayer->Broadcast(&packet);
}

ACMD(do_attackspeed)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	WORD wSpeed = (WORD)atof(strToken.c_str());

	pPlayer->SetLastAttackSpeedRate(wSpeed);

	CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_ATTACK_SPEEDRATE));
	sGU_UPDATE_CHAR_ATTACK_SPEEDRATE * res = (sGU_UPDATE_CHAR_ATTACK_SPEEDRATE *)packet.GetPacketData();
	res->wOpCode = GU_UPDATE_CHAR_ATTACK_SPEEDRATE;
	res->handle = pPlayer->GetID();
	res->wAttackSpeedRate = wSpeed;
	pPlayer->Broadcast(&packet);
}

ACMD(do_warfog)
{
	CGameServer* app = (CGameServer*)g_pApp;
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	
	CTriggerObject* pNextObj = NULL;
	CTriggerObject* pObj = (CTriggerObject*)pPlayer->GetCurWorld()->GetObjectList()->GetFirst(OBJTYPE_TOBJECT);
	while (pObj)
	{
		pNextObj = (CTriggerObject*)pPlayer->GetCurWorld()->GetObjectList()->GetNext(pObj->GetWorldObjectLinker());

		if (pObj->GetFunc() == eDBO_TRIGGER_OBJECT_FUNC_SELECTION + eDBO_TRIGGER_OBJECT_FUNC_NAMEKAN_SIGN)
		{
			if (pPlayer->CheckWarFog(pObj->GetContent()) == false)
			{
				if (pPlayer->AddWarFogFlag(pObj->GetContent()))
				{
					CNtlPacket packetQry(sizeof(sGQ_WAR_FOG_UPDATE_REQ));
					sGQ_WAR_FOG_UPDATE_REQ * resQry = (sGQ_WAR_FOG_UPDATE_REQ*)packetQry.GetPacketData();
					resQry->wOpCode = GQ_WAR_FOG_UPDATE_REQ;
					resQry->charID = pPlayer->GetCharID();
					resQry->contentsTblidx = pObj->GetContent();
					memcpy(resQry->sInfo.achWarFogFlag, pPlayer->GetWarFogFlag(), sizeof(resQry->sInfo.achWarFogFlag));
					packetQry.SetPacketLen(sizeof(sGQ_WAR_FOG_UPDATE_REQ));
					app->SendTo(app->GetQueryServerSession(), &packetQry);
				}
			}
		}

		pObj = pNextObj;
	}
}

ACMD(do_test)
{
	CGameServer* app = (CGameServer*)g_pApp;
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE stage = (BYTE)atof(strToken.c_str());
	strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE invincible = (BYTE)atof(strToken.c_str());

	if (!pPlayer || !pPlayer->IsInitialized())
		return;


	CNtlPacket packet(sizeof(sGU_PARTY_CREATE_RES));
	sGU_PARTY_CREATE_RES* res = (sGU_PARTY_CREATE_RES*)packet.GetPacketData();
	res->wOpCode = GU_PARTY_CREATE_RES;

	std::wstring charname = L"5555";

	if (pPlayer->GetParty() == NULL)
	{
		wcsncpy(res->wszPartyName, charname.c_str(), NTL_MAX_SIZE_PARTY_NAME + 1);

		CParty* party = g_pPartyManager->CreateParty(pPlayer, res->wszPartyName);
		if (party)
		{
			res->wResultCode = GAME_SUCCESS;
			res->partyID = party->GetPartyID();
		}
		else res->wResultCode = GAME_PARTY_NOT_CREATED_FOR_SOME_REASON;
	}
	else res->wResultCode = GAME_PARTY_ALREADY_IN_PARTY;

	packet.SetPacketLen(sizeof(sGU_PARTY_CREATE_RES));
	pPlayer->Broadcast(&packet);

	WORD wResultcode = GAME_SUCCESS;
	if (pPlayer->GetParty() && pPlayer->GetPartyID() != INVALID_PARTYID)
	{
		if (pPlayer->GetParty()->GetPartyLeaderID() == pPlayer->GetID())
		{
			if (pPlayer->GetParty()->IsEveryoneInLeaderRange(pPlayer, NTL_MAX_RADIUS_OF_VISIBLE_AREA))
			{
				BYTE byBeginStage = stage;
				CItem* pItem = NULL;

				CBattleDungeon* pDungeon = g_pDungeonManager->CreateBattleDungeon(pPlayer, wResultcode, byBeginStage);
				if (pDungeon == NULL)
					wResultcode = GAME_PARTY_DUNGEON_IS_NOT_CREATED;
				else
				{
					if (pItem && byBeginStage > 1)
						pItem->SetCount(pItem->GetCount() - 1, false, true);
				}
			}
			else wResultcode = GAME_PARTY_MEMBER_IS_TOO_FAR;
		}
		else wResultcode = GAME_COMMON_YOU_ARE_NOT_A_PARTY_LEADER;
	}
	
	if (invincible == 1)
	{
		sDBO_BUFF_PARAMETER aBuffParameter[NTL_MAX_EFFECT_IN_SKILL];
		sSKILL_TBLDAT* pTempSkillTbldat = (sSKILL_TBLDAT*)g_pTableContainer->GetSkillTable()->FindData(2385);

		eSYSTEM_EFFECT_CODE aeEffectCode[NTL_MAX_EFFECT_IN_SKILL];
		aeEffectCode[0] = g_pTableContainer->GetSystemEffectTable()->GetEffectCodeWithTblidx(pTempSkillTbldat->skill_Effect[0]);
		aeEffectCode[1] = INVALID_SYSTEM_EFFECT_CODE;

		aBuffParameter[0].byBuffParameterType = DBO_BUFF_PARAMETER_TYPE_DEFAULT;
		aBuffParameter[0].buffParameter.fParameter = 0;
		aBuffParameter[0].buffParameter.dwRemainValue = 0;

		DWORD dwDurationInMs = 3600 * 1000;

		pPlayer->GetBuffManager()->RegisterBuff(dwDurationInMs, aeEffectCode, aBuffParameter, INVALID_HOBJECT, BUFF_TYPE_BLESS, pTempSkillTbldat);
	}
	

	/*
	WORD wResultcode = GAME_SUCCESS;
	if (pPlayer->GetParty() && pPlayer->GetPartyID() != INVALID_PARTYID)
	{
		if (pPlayer->GetParty()->GetPartyLeaderID() == pPlayer->GetID())
		{
			if (pPlayer->GetParty()->IsEveryoneInLeaderRange(pPlayer, NTL_MAX_RADIUS_OF_VISIBLE_AREA))
			{
				BYTE byBeginStage = charid;
				CItem* pItem = NULL;

				CBattleDungeon* pDungeon = g_pDungeonManager->CreateBattleDungeon(pPlayer, wResultcode, byBeginStage);
				if (pDungeon == NULL)
					wResultcode = GAME_PARTY_DUNGEON_IS_NOT_CREATED;
				else
				{
					if (pItem && byBeginStage > 1)
						pItem->SetCount(pItem->GetCount() - 1, false, true);
				}
			}
			else wResultcode = GAME_PARTY_MEMBER_IS_TOO_FAR;
		}
		else wResultcode = GAME_COMMON_YOU_ARE_NOT_A_PARTY_LEADER;
	}
	*/

	
}

ACMD(do_upgrade)
{
	CGameServer* app = (CGameServer*)g_pApp;
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	int nGrade = (int)atof(strToken.c_str());

	nGrade = (nGrade > NTL_ITEM_MAX_GRADE) ? NTL_ITEM_MAX_GRADE : nGrade;

	int nCount = 0;

	for (int i = 0; i < EQUIP_SLOT_TYPE_SCOUTER; i++)
	{
		CItem* pItem = pPlayer->GetPlayerItemContainer()->GetItem(CONTAINER_TYPE_EQUIP, i);
		if (pItem)
		{
			if (pItem->GetGrade() != nGrade)
			{
				pItem->SetGrade(nGrade);

				CNtlPacket packet(sizeof(sGU_ITEM_UPDATE));
				sGU_ITEM_UPDATE * res = (sGU_ITEM_UPDATE*)packet.GetPacketData();
				res->wOpCode = GU_ITEM_UPDATE;
				res->handle = pItem->GetID();
				memcpy(&res->sItemData, &pItem->GetItemData(), sizeof(sITEM_DATA));
				packet.SetPacketLen(sizeof(sGU_ITEM_UPDATE));
				pPlayer->SendPacket(&packet);

				CNtlPacket pQry(sizeof(sGQ_ITEM_UPDATE_REQ));
				sGQ_ITEM_UPDATE_REQ * rQry = (sGQ_ITEM_UPDATE_REQ *)pQry.GetPacketData();
				rQry->wOpCode = GQ_ITEM_UPDATE_REQ;
				rQry->handle = pPlayer->GetID();
				rQry->charId = pPlayer->GetCharID();
				memcpy(&rQry->sItem, &pItem->GetItemData(), sizeof(sITEM_DATA));
				pQry.SetPacketLen(sizeof(sGQ_ITEM_UPDATE_REQ));
				app->SendTo(app->GetQueryServerSession(), &pQry);

				++nCount;
			}
		}
	}

	if(nCount > 0)
		pPlayer->GetCharAtt()->CalculateAll();
}

ACMD(do_setitemrank)
{
	CGameServer* app = (CGameServer*)g_pApp;
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	int nRank = (int)atof(strToken.c_str());

	nRank = (nRank > ITEM_RANK_LAST) ? ITEM_RANK_LAST : nRank;

	int nCount = 0;

	for (int i = 0; i < EQUIP_SLOT_TYPE_SCOUTER; i++)
	{
		CItem* pItem = pPlayer->GetPlayerItemContainer()->GetItem(CONTAINER_TYPE_EQUIP, i);
		if (pItem)
		{
			if (pItem->GetRank() != nRank)
			{
				pItem->SetRank(nRank);

				CNtlPacket packet(sizeof(sGU_ITEM_UPDATE));
				sGU_ITEM_UPDATE * res = (sGU_ITEM_UPDATE*)packet.GetPacketData();
				res->wOpCode = GU_ITEM_UPDATE;
				res->handle = pItem->GetID();
				memcpy(&res->sItemData, &pItem->GetItemData(), sizeof(sITEM_DATA));
				packet.SetPacketLen(sizeof(sGU_ITEM_UPDATE));
				pPlayer->SendPacket(&packet);

				CNtlPacket pQry(sizeof(sGQ_ITEM_UPDATE_REQ));
				sGQ_ITEM_UPDATE_REQ * rQry = (sGQ_ITEM_UPDATE_REQ *)pQry.GetPacketData();
				rQry->wOpCode = GQ_ITEM_UPDATE_REQ;
				rQry->handle = pPlayer->GetID();
				rQry->charId = pPlayer->GetCharID();
				memcpy(&rQry->sItem, &pItem->GetItemData(), sizeof(sITEM_DATA));
				pQry.SetPacketLen(sizeof(sGQ_ITEM_UPDATE_REQ));
				app->SendTo(app->GetQueryServerSession(), &pQry);

				++nCount;
			}
		}
	}

	if (nCount > 0)
		pPlayer->GetCharAtt()->CalculateAll();
}


ACMD(do_logout)
{
	CGameServer* app = (CGameServer*)g_pApp;

	CNtlPacket packet(sizeof(sGM_MOVE_REQ));
	sGM_MOVE_REQ* res = (sGM_MOVE_REQ*)packet.GetPacketData();
	res->wOpCode = GM_MOVE_REQ;
	res->accountId = pPlayer->GetAccountID();
	packet.SetPacketLen(sizeof(sGM_MOVE_REQ));
	app->SendTo(app->GetMasterServerSession(), &packet);
	
}

ACMD(do_mute)
{
	CGameServer* app = (CGameServer*)g_pApp;

	/*
		@mute CHARNAME DURATION(MINUTES) REASON(MAX 128 CHARACTERS)
	*/

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring name = std::wstring(strToken.begin(), strToken.end());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	DWORD dwDuration = (DWORD)atof(strToken.c_str());

	pToken->PopToPeek();
	std::string text = "";

	while (text.length() < NTL_MAX_LENGTH_OF_MAIL_MESSAGE - 10)
	{
		std::string mtext = pToken->PeekNextToken(NULL, &iLine);
		if (mtext == ";" || mtext == "")
		{
			break;
		}
		else
		{
			text += mtext;
			text += " ";
		}
	}

	CNtlPacket packet(sizeof(sGT_UPDATE_PUNISH));
	sGT_UPDATE_PUNISH * res = (sGT_UPDATE_PUNISH*)packet.GetPacketData();
	res->wOpCode = GT_UPDATE_PUNISH;
	res->accountId = pPlayer->GetAccountID();
	res->dwDurationInMinute = dwDuration;
	NTL_SAFE_WCSCPY(res->awchGmCharName, pPlayer->GetCharName());
	NTL_SAFE_WCSCPY(res->awchCharName, name.c_str());
	NTL_WCSCPY_S(res->wchReason, NTL_MAX_LENGTH_OF_MAIL_MESSAGE + 1, s2ws(text).c_str());
	packet.SetPacketLen(sizeof(sGT_UPDATE_PUNISH));
	app->SendTo(app->GetChatServerSession(), &packet);
}

ACMD(do_unmute)
{
	CGameServer* app = (CGameServer*)g_pApp;

	/*
		@unmute CHARNAME
	*/

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring name = std::wstring(strToken.begin(), strToken.end());

	CNtlPacket packet(sizeof(sGT_UPDATE_PUNISH));
	sGT_UPDATE_PUNISH * res = (sGT_UPDATE_PUNISH*)packet.GetPacketData();
	res->wOpCode = GT_UPDATE_PUNISH;
	res->accountId = pPlayer->GetAccountID();
	res->dwDurationInMinute = 0;
	NTL_SAFE_WCSCPY(res->awchCharName, name.c_str());
	packet.SetPacketLen(sizeof(sGT_UPDATE_PUNISH));
	app->SendTo(app->GetChatServerSession(), &packet);
}

ACMD(do_go)
{
	CGameServer* app = (CGameServer*)g_pApp;

	/*
		@go x y z
	*/

	CNtlVector vLoc;

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	vLoc.x = (float)atof(strToken.c_str());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	vLoc.y = (float)atof(strToken.c_str());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	vLoc.z = (float)atof(strToken.c_str());

	pPlayer->StartTeleport(vLoc, pPlayer->GetCurDir(), pPlayer->GetWorldID(), TELEPORT_TYPE_COMMAND);
}

ACMD(do_addtitle)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX titleIdx = (TBLIDX)atof(strToken.c_str());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring wstrName = std::wstring(strToken.begin(), strToken.end());

	CPlayer* pTarget = pPlayer;

	if (wcslen(wstrName.c_str()) > 0)
	{
		pTarget = g_pObjectManager->FindByName(wstrName.c_str());
		if (pTarget == NULL || pTarget->IsInitialized() == false)
			return;
	}

	/*
		@addtitle ID [CHARNAME]
	*/

	if ((titleIdx - 1) / NTL_MAX_CHAR_TITLE_FLAG_COUNT >= NTL_MAX_CHAR_TITLE_COUNT_IN_FLAG)
		return;

	if (pTarget->CheckCharTitle(titleIdx - 1) == false) // 303 is gm title
	{
		pTarget->AddCharTitle(titleIdx - 1);
	}
}

ACMD(do_deltitle)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX titleIdx = (TBLIDX)atof(strToken.c_str());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring wstrName = std::wstring(strToken.begin(), strToken.end());

	CPlayer* pTarget = pPlayer;

	if (wcslen(wstrName.c_str()) > 0)
	{
		pTarget = g_pObjectManager->FindByName(wstrName.c_str());
		if (pTarget == NULL || pTarget->IsInitialized() == false)
			return;
	}

	/*
		@deltitle ID [CHARNAME]
	*/

	if ((titleIdx - 1) / NTL_MAX_CHAR_TITLE_FLAG_COUNT >= NTL_MAX_CHAR_TITLE_COUNT_IN_FLAG)
		return;

	if (pTarget->CheckCharTitle(titleIdx - 1) == true) // 303 is gm title
	{
		pTarget->DelCharTitle(titleIdx - 1);
	}
}

ACMD(do_setitemduration)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	DWORD dwSeconds = (DWORD)atof(strToken.c_str());

	/*
		@setitemduration SECONDS
	*/

	CItem* pItem = pPlayer->GetPlayerItemContainer()->GetItem(1, 0); //get first item from inventory
	if (pItem && pItem->GetDurationtype() == eDURATIONTYPE_FLATSUM)
	{
		pItem->SetUseEndTime(time(0) + dwSeconds);

		CNtlPacket pQry(sizeof(sGQ_ITEM_CHANGE_DURATIONTIME_REQ));
		sGQ_ITEM_CHANGE_DURATIONTIME_REQ* rQry = (sGQ_ITEM_CHANGE_DURATIONTIME_REQ*)pQry.GetPacketData();
		rQry->wOpCode = GQ_ITEM_CHANGE_DURATIONTIME_REQ;
		rQry->charId = pPlayer->GetCharID();
		rQry->handle = pPlayer->GetID();
		memcpy(&rQry->sItem, &pItem->GetItemData(), sizeof(sITEM_DATA));
		pQry.SetPacketLen(sizeof(sGQ_ITEM_CHANGE_DURATIONTIME_REQ));
		app->SendTo(app->GetQueryServerSession(), &pQry);
	}
}


ACMD(do_bind)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();

	/*
		@bind
	*/

	if (pPlayer->GetCurWorld() && pPlayer->GetCurWorld()->GetTbldat()->bDynamic == false)
	{
		CNtlPacket packetQry(sizeof(sGQ_PC_UPDATE_BIND_REQ));
		sGQ_PC_UPDATE_BIND_REQ * resQry = (sGQ_PC_UPDATE_BIND_REQ *)packetQry.GetPacketData();
		resQry->wOpCode = GQ_PC_UPDATE_BIND_REQ;
		resQry->charId = pPlayer->GetCharID();
		resQry->handle = pPlayer->GetID();
		resQry->byBindType = DBO_BIND_TYPE_GM_TOOL;
		resQry->bindObjectTblidx = INVALID_TBLIDX;
		resQry->bindWorldId = pPlayer->GetWorldID();
		pPlayer->GetCurLoc().CopyTo(resQry->vBindLoc);
		pPlayer->GetCurDir().CopyTo(resQry->vBindDir);
		packetQry.SetPacketLen(sizeof(sGQ_PC_UPDATE_BIND_REQ));
		app->SendTo(app->GetQueryServerSession(), &packetQry);

		pPlayer->SetBindLoc(resQry->vBindLoc);
		pPlayer->SetBindDir(resQry->vBindDir);
		pPlayer->SetBindObjectTblidx(resQry->bindObjectTblidx);
		pPlayer->SetBindWorldID(resQry->bindWorldId);
		pPlayer->SetBindType(DBO_BIND_TYPE_GM_TOOL);
	}
}

ACMD(do_exp)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);

	if (pPlayer->IsReceiveExpDisabled() == false && strToken.compare("off") == 0)
	{
		pPlayer->SetExpReceiveDisabled(true);

		WCHAR wszMsgBuf[256];
		WCharTLiteralToWCHAR(L"Receive EXP has been disabled", wszMsgBuf, sizeof(wszMsgBuf)/sizeof(WCHAR));
		WCHAR* msg = wszMsgBuf;

		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT * res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		res->wMessageLengthInUnicode = WCHARLen(msg);
		res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, msg);
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
	}
	else if (pPlayer->IsReceiveExpDisabled() == true && strToken.compare("on") == 0)
	{
		pPlayer->SetExpReceiveDisabled(false);

		WCHAR wszMsgBuf[256];
		WCharTLiteralToWCHAR(L"Receive EXP has been enabled", wszMsgBuf, sizeof(wszMsgBuf)/sizeof(WCHAR));
		WCHAR* msg = wszMsgBuf;

		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT * res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		res->wMessageLengthInUnicode = WCHARLen(msg);
		res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, msg);
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
	}
}

ACMD(do_resetexp)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);

	if (pPlayer->GetExp() > 0)
	{
		CNtlPacket packet(sizeof(sGU_UPDATE_CHAR_EXP));
		sGU_UPDATE_CHAR_EXP * res = (sGU_UPDATE_CHAR_EXP*)packet.GetPacketData();
		res->handle = pPlayer->GetID();
		res->wOpCode = GU_UPDATE_CHAR_EXP;
		res->dwCurExp = 0;
		res->dwAcquisitionExp = 0;
		res->dwIncreasedExp = 0;
		res->dwBonusExp = 0;
		packet.SetPacketLen(sizeof(sGU_UPDATE_CHAR_EXP));
		pPlayer->SendPacket(&packet);

		pPlayer->SetExp(0);
	}
}

ACMD(do_starthoneybee)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byHours = (BYTE)atof(strToken.c_str());

	if (byHours > 12)
		byHours = 12;

	/*
		@starthoneybee HOURS
	*/

	g_pHoneyBeeEvent->StartEvent(byHours);
	NTL_PRINT(PRINT_APP, "HoneybeeEvent Started");
}

ACMD(do_stophoneybee)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	BYTE byHours = (BYTE)atof(strToken.c_str());

	/*
	@stophoneybee
	*/

	g_pHoneyBeeEvent->EndEvent();
	NTL_PRINT(PRINT_APP, "HoneybeeEvent Stopped");
}

ACMD(do_deleteguild)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	const char* chName = strToken.c_str();

	/*
		@deleteguild GUILDNAME
	*/

	WCHAR* wchName = Ntl_MB2WC((char*)chName);

	CNtlPacket cPacket(sizeof(sGT_GUILD_DELETE));
	sGT_GUILD_DELETE * cRes = (sGT_GUILD_DELETE *)cPacket.GetPacketData();
	cRes->wOpCode = GT_GUILD_DELETE;
	cRes->gmCharId = pPlayer->GetCharID();
	NTL_SAFE_WCSCPY(cRes->wszGuildName, wchName);
	cPacket.SetPacketLen(sizeof(sGT_GUILD_DELETE));
	app->SendTo(app->GetChatServerSession(), &cPacket); //Send to chat server

	//clean memory
	Ntl_CleanUpHeapStringW(wchName);

	ERR_LOG(LOG_USER, "GM %u deleted Guild %s", pPlayer->GetCharID(), chName);
}

ACMD(do_cancelah)
{
	CGameServer* app = (CGameServer*)g_pApp;

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	CHARACTERID charid = (CHARACTERID)atof(strToken.c_str());

	/*
		@cancelah charid
	*/

	CNtlPacket packet(sizeof(sGT_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ));
	sGT_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ * res = (sGT_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ *)packet.GetPacketData();
	res->wOpCode = GT_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ;
	res->charId = charid;
	res->itemId = INVALID_ITEMID;
	res->nItem = INVALID_ITEMID;
	packet.SetPacketLen(sizeof(sGT_TENKAICHIDAISIJYOU_SELL_CANCEL_REQ));
	app->SendTo(app->GetChatServerSession(), &packet);
}

ACMD(do_addmudosa)
{
	CGameServer * app = (CGameServer*)NtlSfxGetApp();

	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	DWORD dwMudosa = (DWORD)atof(strToken.c_str());

	pToken->PopToPeek();
	strToken = pToken->PeekNextToken(NULL, &iLine);
	std::wstring wstrName = std::wstring(strToken.begin(), strToken.end());

	CPlayer* pTarget = pPlayer;

	if (wcslen(wstrName.c_str()) > 0)
	{
		pTarget = g_pObjectManager->FindByName(wstrName.c_str());
		if (pTarget == NULL || pTarget->IsInitialized() == false)
			return;
	}

	DWORD dwFinalMudosa = pTarget->GetMudosaPoints() + dwMudosa;

	pTarget->UpdateMudosaPoints(dwFinalMudosa);
}

ACMD(do_startgm)
{
	CPlayer* pTarget = pPlayer;
	CPlayer* pTargetPC = g_pObjectManager->GetPC(pTarget->GetTargetHandle());

	if (pTargetPC && pTargetPC->IsInitialized() && pTargetPC->IsPC())
	{
		TBLIDX SkillId = INVALID_TBLIDX;
		BYTE ClassId = INVALID_BYTE;

		TBLIDX masterPassive = INVALID_TBLIDX;
		TBLIDX itemPassive = INVALID_TBLIDX;
		TBLIDX flightPassive = INVALID_TBLIDX;


		switch (pTargetPC->GetClass())
		{
		case PC_CLASS_HUMAN_FIGHTER:
		case PC_CLASS_STREET_FIGHTER:
		{
			ClassId = 8;
			masterPassive = 829991;
			itemPassive = 11140026;
			flightPassive = 11120142;
		}
		break;
		case PC_CLASS_SWORD_MASTER:
		{
			ClassId = 7;
			masterPassive = 729991;
			itemPassive = 11140026;
			flightPassive = 11120142;
		}
		break;
		case PC_CLASS_HUMAN_MYSTIC:
		case PC_CLASS_CRANE_ROSHI:
		{
			ClassId = 10;
			masterPassive = 1029991;
			itemPassive = 11140027;
			flightPassive = 11120143;
		}
		break;
		case PC_CLASS_TURTLE_ROSHI:
		{
			ClassId = 9;
			masterPassive = 929991;
			itemPassive = 11140027;
			flightPassive = 11120143;
		}
		break;
		case PC_CLASS_NAMEK_FIGHTER:
		case PC_CLASS_DARK_WARRIOR:
		{
			ClassId = 14;
			masterPassive = 1429991;
			itemPassive = 11140028;
			flightPassive = 11120144;
		}
		break;
		case PC_CLASS_SHADOW_KNIGHT:
		{
			ClassId = 13;
			masterPassive = 1329991;
			itemPassive = 11140028;
			flightPassive = 11120144;
		}
		break;
		case PC_CLASS_DENDEN_HEALER:
		{
			ClassId = 16;
			masterPassive = 1629991;
			itemPassive = 11140029;
			flightPassive = 11120145;
		}
		break;
		case PC_CLASS_NAMEK_MYSTIC:
		case PC_CLASS_POCO_SUMMONER:
		{
			ClassId = 15;
			masterPassive = 1529991;
			itemPassive = 11140029;
			flightPassive = 11120145;
		}
		break;
		case PC_CLASS_ULTI_MA:
		{
			ClassId = 18;
			masterPassive = 1829991;
			itemPassive = 11140030;
			flightPassive = 11120146;
		}
		break;
		case PC_CLASS_MIGHTY_MAJIN:
		case PC_CLASS_GRAND_MA:
		{
			ClassId = 17;
			masterPassive = 1729991;
			itemPassive = 11140030;
			flightPassive = 11120146;
		}
		break;
		case PC_CLASS_WONDER_MAJIN:
		case PC_CLASS_PLAS_MA:
		{
			ClassId = 20;
			masterPassive = 2029991;
			itemPassive = 11140031;
			flightPassive = 11120147;
		}
		break;
		case PC_CLASS_KAR_MA:
		{
			ClassId = 19;
			masterPassive = 1929991;
			itemPassive = 11140031;
			flightPassive = 11120147;
		}
		break;
		}

		WORD wTemp;

		pTargetPC->UpdateClass(ClassId);

		pTargetPC->GetSkillManager()->LearnSkill(masterPassive, wTemp, false);
	}
}

ACMD(do_createloot)
{
	pToken->PopToPeek();
	std::string strToken = pToken->PeekNextToken(NULL, &iLine);
	TBLIDX ItemId = (TBLIDX)atof(strToken.c_str());

	strToken = pToken->PeekNextToken(NULL, &iLine);
	int nCount = (int)atof(strToken.c_str());

	if (nCount > 100)
		nCount = 100;

	for (int i = 0; i < nCount; i++)
	{
		CNtlVector vPos(pPlayer->GetCurLoc());

		sVECTOR3 vec;
		vPos.CopyTo(vec.x, vec.y, vec.z);

		vec.x += RandomRangeF(-100.0f, 100.0f);
		vec.z += RandomRangeF(-100.0f, 100.0f);

		CItemDrop * pBall = g_pItemManager->CreateSingleDrop(100.f, ItemId);
		if (pBall)
		{
			pBall->AddToGround(pPlayer->GetWorldID(), vec);
		}
	}
}

ACMD(server_skill_reset)
{
	
}

ACMD(do_br)
{
	if (!g_pBattleRoyaleEvent)
	{
		ERR_LOG(LOG_SYSTEM, "BattleRoyaleEvent singleton not initialized!");
		CNtlStringW msg;
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		WCHAR wszFormatBuf[256];
		WCharTLiteralToWCHAR(L"Battle Royale system error: Singleton not initialized!", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
		res->wMessageLengthInUnicode = (WORD)msg.Format(wszFormatBuf);
		res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, msg.c_str());
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
		return;
	}

	if (g_pBattleRoyaleEvent->IsEventActive())
	{
		CNtlStringW msg;
		CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
		res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
		WCHAR wszFormatBuf[256];
		WCharTLiteralToWCHAR(L"Battle Royale event is already active!", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
		res->wMessageLengthInUnicode = (WORD)msg.Format(wszFormatBuf);
		res->byDisplayType = SERVER_TEXT_SYSTEM;
		NTL_SAFE_WCSCPY(res->awchMessage, msg.c_str());
		packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
		pPlayer->SendPacket(&packet);
		return;
	}

	ERR_LOG(LOG_SYSTEM, "GM %u starting Battle Royale event", pPlayer->GetCharID());
	g_pBattleRoyaleEvent->StartEvent();
	
	CNtlStringW msg;
	CNtlPacket packet(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	sGU_SYSTEM_DISPLAY_TEXT* res = (sGU_SYSTEM_DISPLAY_TEXT*)packet.GetPacketData();
	res->wOpCode = GU_SYSTEM_DISPLAY_TEXT;
	WCHAR wszFormatBuf[256];
	WCharTLiteralToWCHAR(L"Battle Royale event started!", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
	res->wMessageLengthInUnicode = (WORD)msg.Format(wszFormatBuf);
	res->byDisplayType = SERVER_TEXT_SYSTEM;
	NTL_SAFE_WCSCPY(res->awchMessage, msg.c_str());
	packet.SetPacketLen(sizeof(sGU_SYSTEM_DISPLAY_TEXT));
	pPlayer->SendPacket(&packet);
}