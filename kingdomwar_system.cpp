#include "kingdomwar_system.h"
#include "playerManager.h"
#include "man_system.h"
#include "heroparty_system.h"
#include "map_war.h"
#include "net_helper.hpp"
#include "timer2.h"

namespace gg
{
	kingdomwar_system* const kingdomwar_system::_Instance = new kingdomwar_system();

	kingdomwar_system::kingdomwar_system()
	{
	}

	void kingdomwar_system::initData()
	{
		_output_ret = Json::arrayValue;

		loadFile();
		loadDB();
		_city_list.update(true);
		_path_list.update(true);
		addUpdater(boostBind(kingdomwar_system::tick, this));
		addTimer();
		startTimer();
	}

	void kingdomwar_system::timerTick()
	{
		update();
		startTimer();
	}

	void kingdomwar_system::tick()
	{
		_city_list.update();
		_path_list.update();
		if (!Observer::empty())
		{
			qValue state;
			qValue nation;
			qValue path;
			_city_list.getUpStateInfo(state);
			_city_list.getUpNationInfo(nation);
			_path_list.getUpdateInfo(path);
			if (state.isEmpty() && nation.isEmpty() && path.isEmpty())
				return;
			qValue q(qJson::qj_object);
			q.addMember("s", state);
			q.addMember("c", nation);
			q.addMember("p", path);
			qValue m;
			m.append(res_sucess);
			m.append(q);
			detail::batchOnline(getObserver(), m, gate_client::kingdom_war_main_update_resp);
		}
	}
	
	void kingdomwar_system::loadFile()
	{
		FileJsonSeq vec;	
		vec = Common::loadFileJsonFromDir("./instance/kingdom_war/city");
		ForEachC(FileJsonSeq, it, vec)
			_city_list.push(Creator<KingdomWar::City>::Create(*it));

		KingdomWar::Graph graph(vec.size());
		ForEachC(FileJsonSeq, it, vec)
		{
			const Json::Value& json = *it;
			int city_id = json["id"].asInt();
			const Json::Value& link_citys = json["relationship"];
			const Json::Value& path_types = json["relationship_type"];
			const Json::Value& cost_times = json["distance"];
			for (unsigned i = 0; i < link_citys.size(); ++i)
			{
				int link_city = link_citys[i].asInt();
				int left_city = city_id > link_city ? link_city : city_id;
				int right_city = city_id > link_city ? city_id : link_city;
				int path_id = left_city * 100 + right_city;
				KingdomWar::PathPtr ptr = getPath(path_id);
				if (!ptr)
				{
					ptr = Creator<KingdomWar::Path>::Create(path_id, city_id, link_city);
					_path_list.push(ptr);
					getCity(left_city)->addPath(ptr);
					getCity(right_city)->addPath(ptr);
				}
				ptr->setPathType(city_id, link_city, path_types[i].asInt(), cost_times[i].asInt());
				if (ptr->access(city_id, link_city))
					graph.set(city_id, link_city, cost_times[i].asInt());
			}

		}
		vector<KingdomWar::Lines> lines_list;
		for (unsigned i = 0; i < vec.size(); ++i)
		{
			KingdomWar::Lines lines;
			shortestPath_DIJ(graph, lines, i);
			lines_list.push_back(lines);
		}
		_shortest_path.load(lines_list);

		vec = Common::loadFileJsonFromDir("./instance/kingdom_war/npc");
		ForEach(FileJsonSeq, it, vec)
		{
			Json::Value& chapterMap = *it;

			MapDataCfgPtr mapDatePtr = Creator<mapDataConfig>::Create();

			mapDatePtr->mapId = chapterMap["mapId"].asInt();
			mapDatePtr->frontId = chapterMap["frontId"].asInt();
			mapDatePtr->background = chapterMap.isMember("background") ? chapterMap["background"].asInt() : 1;
			mapDatePtr->faceID = chapterMap["faceID"].asInt();
			mapDatePtr->mapName = chapterMap["mapName"].asString();
			mapDatePtr->mapLevel = chapterMap["mapLevel"].asInt();
			mapDatePtr->battleValue = chapterMap["battleValue"].asInt();
			mapDatePtr->needFood = chapterMap["needfood"].asInt();
			mapDatePtr->needAction = chapterMap["needAction"].asInt();
			mapDatePtr->chanllengeTimes = chapterMap["challengeTimes"].asInt();
			mapDatePtr->manExp = chapterMap["manExp"].asUInt();
			mapDatePtr->levelLimit = chapterMap["levelLimit"].asUInt();
			mapDatePtr->robotIDX = chapterMap["robotIDX"].asInt();
			// 					Json::Value& rwJson = chapterMap["winBox"];
			// 					mapDatePtr->winBox = actionFormat(rwJson);
			const unsigned rwJsonID = chapterMap["winBox"].asUInt();
			mapDatePtr->winBox = actionFormat(rwJsonID);
			for (unsigned nn = 0; nn < chapterMap["army"].size(); ++nn)
			{
				Json::Value& npcJson = chapterMap["army"][nn];
				armyNPC npc;
				npc.npcID = npcJson["npcID"].asInt();
				npc.holdMorale = npcJson["holdMorale"].asBool();
				for (unsigned cn = 0; cn < characterNum; ++cn)
				{
					npc.initAttri[cn] = npcJson["initAttri"][cn].asInt();
					//							npc.addAttri[cn] = npcJson["addAttri"][cn].asInt();
				}
				npc.armsType = npcJson["armsType"].asInt();
				for (unsigned cn = 0; cn < armsModulesNum; ++cn)
				{
					npc.armsModule[cn] = npcJson["armsModule"][cn].asDouble();
				}
				npc.npcLevel = npcJson["npcLevel"].asInt();
				npc.npcPos = npcJson["npcPos"].asUInt() % 9;
				npc.battleValue = npcJson["battleValue"].asInt();
				npc.skill_1 = npcJson["skill_1"].asInt();
				npc.skill_2 = npcJson["skill_2"].asInt();
				for (unsigned eq_idx = 0; eq_idx < npcJson["equip"].size(); ++eq_idx)
				{
					npc.equipList.push_back(BattleEquip(npcJson["equip"][eq_idx][0u].asUInt(),
								npcJson["equip"][eq_idx][1u].asInt(),
								npcJson["equip"][eq_idx][2u].asUInt()));
				}
				mapDatePtr->npcList.push_back(npc);
			}
			_npc_map[mapDatePtr->mapId] = mapDatePtr;
		}

		Json::Value json;
		json = Common::loadJsonFile("./instance/kingdom_war/npc_rule.json");
		ForEach(Json::Value, it, json)
			_npc_rule.push_back(KingdomWar::NpcRule(*it));
	}

	void kingdomwar_system::loadDB()
	{
		_city_list.loadDB();
		objCollection objs = db_mgr.Query(DBN::dbPlayerKingdomWar);
		ForEachC(objCollection, it, objs)
		{
			int pid = (*it)[strPlayerID].Int();
			playerDataPtr d = player_mgr.getPlayer(pid);
			if (!d) continue;
			if (d->KingDomWar->getExploit() > 0)
				_rank_mgr.update(d, 0);
			for (unsigned i = 0; i < 3; ++i)
			{
				const KingdomWar::Position& pos = d->KingDomWarPos->position(i);
				if (pos._type == KingdomWar::PosPath)
				{
					KingdomWar::PathPtr ptr = getPath(pos._id);
					if (!ptr) continue;
					int to_city_id = ptr->id() / 100;
					if (to_city_id == pos._from_city_id)
						to_city_id = ptr->id() % 100;
					ptr->enter(pos._time, d, i, to_city_id);
				}
				else
				{
					KingdomWar::CityPtr ptr = getCity(pos._id);
					if (!ptr) continue;
					ptr->tryLoadPlayer(d, i);
				}
			}
		}
	}

	sBattlePtr kingdomwar_system::getBattlePtr(playerDataPtr player, int army_id)
	{
		sBattlePtr sb = Creator<sideBattle>::Create();
		sb->playerID = player->ID();
		sb->playerName = player->Name();
		sb->isPlayer = true;
		sb->playerLevel = player->LV();
		sb->playerNation = player->Info->Nation();
		sb->playerFace = player->Info->Face();
		sb->battleValue = player->KingDomWarFM->getBV(army_id);
		manList& ml = sb->battleMan;
		ml.clear();
		vector<playerManPtr> ownMan = player->KingDomWarFM->getFM(army_id);
		for (unsigned i = 0; i < ownMan.size(); i++)
		{
			playerManPtr man = ownMan[i];
			if (!man)continue;
			mBattlePtr mbattle = Creator<manBattle>::Create();
			cfgManPtr config = man_sys.getConfig(man->mID());
			if (!config)continue;
			mbattle->manID = man->mID();
			mbattle->battleValue = man->battleValue();
			mbattle->holdMorale = config->holdMorale;
			mbattle->set_skill_1(config->skill_1);
			mbattle->set_skill_2(config->skill_2);
			mbattle->armsType = config->armsType;
			mbattle->manLevel = man->LV();
			mbattle->currentIdx = i % 9;
			memcpy(mbattle->armsModule, config->armsModules, sizeof(mbattle->armsModule));
			man->toInitialAttri(mbattle->initialAttri);
			man->toBattleAttri(player->KingDomWarFM->getFMId(army_id), mbattle->battleAttri);
			mbattle->currentHP = mbattle->getTotalAttri(idx_hp);
			int man_hp = player->KingDomWar->manHp(man->mID());
			if (mbattle->currentHP >= man_hp)
				mbattle->currentHP = man_hp;
			else
				player->KingDomWar->setManHp(man->mID(), mbattle->currentHP);
			mbattle->currentHP = player->KingDomWar->manHp(man->mID());
			std::vector<itemPtr> equipList = man->getEquipList();
			for (unsigned pos = 0; pos < equipList.size(); ++pos)
			{
				itemPtr item = equipList[pos];
				if (item)
				{
					mbattle->equipList.push_back(BattleEquip(pos, item->itemID(), item->getLv()));
				}
			}
			if (0 == i)
			{
				sb->leaderMan = mbattle;
			}
			ml.push_back(mbattle);
		}
		return sb;
	}

	sBattlePtr kingdomwar_system::getNpcBattlePtr(int id)
	{
		MapDataMap::iterator it = _npc_map.find(id);	
		if (it != _npc_map.end())
			return map_sys.npcSide(it->second);
		return map_sys.npcSide(_npc_map.begin()->second);
	}

	KingdomWar::CityPtr kingdomwar_system::getCity(int id)
	{
		return _city_list.getCity(id);
	}
	
	KingdomWar::PathPtr kingdomwar_system::getPath(int id)
	{
		return _path_list.getPath(id);
	}

	KingdomWar::CityPtr kingdomwar_system::getMainCity(int nation)
	{
		if (nation < 0 || nation >= Kingdom::nation_num)
			return KingdomWar::CityPtr();

		return getCity(KingdomWar::MainCity[nation]);
	}

	void kingdomwar_system::quitReq(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);
		if (!d || d->Info->Nation() == Kingdom::null) 
			Return(r, err_illedge);
		ReadJsonArray;
		int id = js_msg[0u].asInt();
		if (id == -1)
		{
			detach(d->ID());
			Return(r, res_sucess);
		}
		KingdomWar::CityPtr ptr = getCity(id);
		if (!ptr)
			Return(r, err_illedge);
		ptr->detach(d->ID());
		Return(r, res_sucess);
	}

	void kingdomwar_system::mainInfoReq(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);
		if (!d || d->Info->Nation() == Kingdom::null) 
			Return(r, err_illedge);

		attach(d->ID());
		mainInfo(d);
	}

	void kingdomwar_system::moveReq(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);
		if (!d || d->Info->Nation() == Kingdom::null) 
			Return(r, err_illedge);

		ReadJsonArray;
		int army_id = js_msg[0u].asInt();
		int to_city_id = js_msg[1u].asInt();

		int res = d->KingDomWarPos->move(Common::gameTime(), army_id, to_city_id, true);
		Return(r, res);
	}

	void kingdomwar_system::playerInfoReq(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);	
		if (!d || d->Info->Nation() == Kingdom::null)
			Return(r, err_illedge);

		d->KingDomWarPos->update();
	}

	void kingdomwar_system::formationReq(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);	
		if (!d || d->Info->Nation() == Kingdom::null)
			Return(r, err_illedge);
		
		d->KingDomWarFM->update();
	}

	void kingdomwar_system::setFormationReq(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);	
		if (!d || d->Info->Nation() == Kingdom::null)
			Return(r, err_illedge);
		
		ReadJsonArray;
		int army_id = js_msg[0u].asInt();
		int fm_id = js_msg[1u].asInt();
		int fm[9];
		for (unsigned i = 0; i < 9; ++i)
			fm[i] = js_msg[2u][i].asInt();

		int res = d->KingDomWarFM->setFormation(army_id, fm_id, fm);
		Return(r, res);
	}

	void kingdomwar_system::cityBattleInfoReq(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);	
		if (!d || d->Info->Nation() == Kingdom::null)
			Return(r, err_illedge);
		
		ReadJsonArray;
		int city_id = js_msg[0u].asInt();
		KingdomWar::CityPtr ptr = getCity(city_id);
		if (!ptr)
			Return(r, err_illedge);

		ptr->attach(d->ID());
		qValue rm;
		rm.append(res_sucess);
		qValue q;
		ptr->getMainInfo(q);
		rm.append(q);
		d->sendToClientFillMsg(gate_client::kingdom_war_city_battle_info_resp, rm);
	}

	void kingdomwar_system::retreatReq(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);	
		if (!d || d->Info->Nation() == Kingdom::null)
			Return(r, err_illedge);

		ReadJsonArray;
		int army_id = js_msg[0u].asInt();

		int res = d->KingDomWarPos->retreat(army_id);
		Return(r, res);
	}

	void kingdomwar_system::reportInfoReq(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);	
		if (!d || d->Info->Nation() == Kingdom::null)
			Return(r, err_illedge);

		d->KingDomWar->updateReport();
	}

	void kingdomwar_system::rankInfoReq(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);	
		if (!d || d->Info->Nation() == Kingdom::null)
			Return(r, err_illedge);
		
		ReadJsonArray;
		int begin = js_msg[0u].asInt();
		int end = js_msg[1u].asInt();
		qValue mq;
		mq.append(res_sucess);
		qValue q(qJson::qj_object);
		_rank_mgr.getInfo(d, begin, end, q);
		mq.append(q);
		d->sendToClientFillMsg(gate_client::kingdom_war_rank_info_resp, mq);
	}

	void kingdomwar_system::buyHpItemReq(net::Msg& m, Json::Value& r)
	{
	
	}

	void kingdomwar_system::useHpItemReq(net::Msg& m, Json::Value& r)
	{
	
	}

	void kingdomwar_system::outputInfoReq(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);	
		if (!d || d->Info->Nation() == Kingdom::null)
			Return(r, err_illedge);
		
		d->KingDomWarOutput->update();
	}
	void kingdomwar_system::getOutputReq(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);	
		if (!d || d->Info->Nation() == Kingdom::null)
			Return(r, err_illedge);
		
		ReadJsonArray;
		int id = js_msg[0u].asInt();
		int res = d->KingDomWarOutput->getOutput(id);
		if (res == res_sucess)
		{
			r[strMsg][1u] = _output_ret;
			_output_ret = Json::arrayValue;
		}
		Return(r, res);
	}

	void kingdomwar_system::mainInfo(playerDataPtr d)
	{
		qValue m;
		m.append(res_sucess);
		qValue q(qJson::qj_object);
		qValue state_info;
		_city_list.getStateInfo(state_info);
		q.addMember("s", state_info);
		qValue nation_info;
		_city_list.getNationInfo(nation_info);
		q.addMember("c", nation_info);
		qValue path_info;
		_path_list.getMainInfo(path_info);
		q.addMember("p", path_info);
		m.append(q);
		d->sendToClientFillMsg(gate_client::kingdom_war_main_info_resp, m);
	}


	void kingdomwar_system::addTimer(unsigned tick_time, const Timer2::TickFunc& func)
	{
		Timer2::add(tick_time, func);
	}

	void kingdomwar_system::goBackMainCity(unsigned time, playerDataPtr d, int army_id)
	{
		KingdomWar::CityPtr ptr = getMainCity(d->Info->Nation());
		if (!ptr)
			return;
		ptr->enter(time, d, army_id);
	}

	void kingdomwar_system::startTimer()
	{
		Timer2::add(Common::gameTime() + 2, boostBind(kingdomwar_system::timerTick, this));
	}

	int kingdomwar_system::getCostTime(int from_id, int to_id)
	{
		return 0;
	}

	const KingdomWar::LineInfo& kingdomwar_system::getLine(int from_id, int to_id) const
	{
		return _shortest_path.getLine(from_id, to_id);
	}

	void kingdomwar_system::updateRank(playerDataPtr d, int old_value)
	{
		_rank_mgr.update(d, old_value);
	}

	void kingdomwar_system::update()
	{
		ForEach(FuncList, it, _updaters)
			(*it)();
	}

	void kingdomwar_system::outputTick()
	{
		ForEach(Observer::IdList, it, getObserver())
		{
			playerDataPtr d = player_mgr.getOnlinePlayer(*it);
			if (!d) continue;
			d->KingDomWarOutput->update();
		}
	}

	void kingdomwar_system::tickEvery5Min()
	{
		_every_5_min->resetTime();
		addTimer(_every_5_min->nextTime(), boostBind(kingdomwar_system::tickEvery5Min, this));

		resetRandomRange();
		ForEach(FuncList, it, _5_min_tickers)
			(*it)();
		outputTick();
	}

	void kingdomwar_system::addTimer()
	{
		_every_5_min = Creator<KingdomWar::Every5Min>::Create();
		_every_5_min->loadDB();
		addTimer(_every_5_min->nextTime(), boostBind(kingdomwar_system::tickEvery5Min, this));
	}

	void kingdomwar_system::addUpdater(const Func& func)
	{
		_updaters.push_back(func);
	}

	void kingdomwar_system::add5MinTicker(const Func& func)
	{
		_5_min_tickers.push_back(func);
	}

	void kingdomwar_system::resetRandomRange()
	{
		int power = heroparty_sys.getKingdomWarNpcBV();
		unsigned i = 0;
		for (; i < _npc_rule.size(); ++i)
		{
			if (power >= _npc_rule[i]._power_begin &&
				power <= _npc_rule[i]._power_end)
				break;
		}
		if (i == _npc_rule.size())
			i = _npc_rule.size() - 1;
		_rand_begin = _npc_rule[i]._npc_begin;
		_rand_end = _npc_rule[i]._npc_end;
	}

	int kingdomwar_system::randomNpcID() const
	{
		return Common::randomBetween(_rand_begin, _rand_end);
	}
}
