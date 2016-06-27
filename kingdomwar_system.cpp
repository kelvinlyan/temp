#include "kingdomwar_system.h"
#include "playerManager.h"
#include "man_system.h"

namespace gg
{
	kingdomwar_system* const kingdomwar_system::_Instance = new kingdomwar_system();

	void kingdomwar_system::initData()
	{
		loadFile();
	}

	void kingdomwar_system::timerTick()
	{
		static unsigned next_tick_time = 0;
		unsigned cur_time = Common::gameTime();
		if (cur_time >= next_tick_time)
		{
			next_tick_time = cur_time + 1;
			_timer.check(cur_time);
		}
	}
	
	void kingdomwar_system::loadFile()
	{
		FileJsonSeq vec;	
		vec = Common::loadFileJsonFromDir("../instance/kingdom_war/");
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
				int path_id = left_city + 100 * right_city;
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

			vector<KingdomWar::Lines> lines_list;
			for (unsigned i = 0; i < vec.size(); ++i)
			{
				KingdomWar::Lines lines;
				shortestPath_DIJ(graph, lines, i);
				lines_list.push_back(lines);
			}
			_shortest_path.load(lines_list);
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
		sb->battleValue = player->KingDomWar->getBV(army_id);
		manList& ml = sb->battleMan;
		ml.clear();
		vector<playerManPtr> ownMan = player->KingDomWar->getFM(army_id);
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
			man->toBattleAttri(player->WarFM->currentID(), mbattle->battleAttri);
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
			detach(d);
			Return(r, res_sucess);
		}
		KingdomWar::CityPtr ptr = getCity(id);
		if (!ptr)
			Return(r, err_illedge);
		ptr->detach(d);
		Return(r, res_sucess);
	}

	void kingdomwar_system::mainInfoReq(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);
		if (!d || d->Info->Nation() == Kingdom::null) 
			Return(r, err_illedge);

		attach(d);
		mainInfo(d);
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

	void kingdomwar_system::addTimer(unsigned tick_time, const KingdomWar::Timer::TickFunc& func)
	{
		_timer.add(tick_time, func);
	}

	void kingdomwar_system::goBackMainCity(unsigned time, playerDataPtr d, int army_id)
	{
		KingdomWar::CityPtr ptr = getMainCity(d->Info->Nation());
		if (!ptr)
			return;
		ptr->enter(time, d, army_id);
	}
}
