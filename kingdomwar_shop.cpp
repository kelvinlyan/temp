#include "kingdomwar_shop.h"

namespace gg
{
	namespace KingdomWar
	{
		ShopData::ShopData(const Json::Value& info)
		{
			_id = info["id"].asInt();
			_consume_con = info["consumeCon"].asInt();
			_buy_times = info["buynum"].asInt();
			_weight = info["weight"].asInt();
			_kingdom_id = (Kingdom::NATION)info["nation"].asInt();
			Json::Value box = info["box"];
			_box = actionFormatBox(box);
		}

		BoxData::BoxData(const Json::Value& info)
		{
			_target = info["target"].asInt();
			Json::Value box = info["reward"];
			_box = actionFormatBox(box);
		}
	}

	kingdomwar_shop_system* const kingdomwar_shop_system::_Instance = new kingdomwar_shop_system();

	void kingdomwar_shop_system::initData()
	{
		loadFile();
	}

	void kingdomwar_shop_system::loadFile()
	{
		Json::Value json;
		json = Common::loadJsonFile("./instance/kingdom_war/shop.json");
		ForEach(Json::Value, it, json)
		{
			KingdomWar::ShopDataPtr ptr = Creator<KingdomWar::ShopData>::Create(*it);
			_shop_map.insert(make_pair(ptr->_id, ptr));
			if (ptr->_weight == 0)
			{
				if (ptr->_kingdom_id != -1)
					_shopFix[ptr->_kingdom_id].push_back(ptr);
				else
				{
					for (unsigned j = 0; j < Kingdom::nation_num; ++j)
						_shopFix[j].push_back(ptr);
				}
			}
			else
			{
				if (ptr->_kingdom_id != -1)
				{
					_shopRd[ptr->_kingdom_id].push_back(ptr);
					_weight[ptr->_kingdom_id] += ptr->_weight;
				}
				else
				{
					for (int j = 0; j < Kingdom::nation_num; ++j)
					{
						_shopRd[j].push_back(ptr);
						_weight[j] += ptr->_weight;
					}
				}
			}
		}
		
		json = Common::loadJsonFile("./instance/kingdom_war/shop_cost.json");
		ForEach(Json::Value, it, json)
			_flush_costs.push_back((*it).asInt());

		json = Common::loadJsonFile("./instance/kingdom_war/box_reward.json");
		ForEach(Json::Value, it, json)
			_box_map.push_back(Creator<KingdomWar::BoxData>::Create(*it));
	}

	KingdomWar::ShopDataPtr kingdomwar_shop_system::getShopData(int id)
	{
		KingdomWar::ShopDataMap::iterator it = _shop_map.find(id);
		return it != _shop_map.end()? it->second : KingdomWar::ShopDataPtr();
	}

	KingdomWar::BoxDataPtr kingdomwar_shop_system::getBoxData(int id)
	{
		if (id < 0 || id > 4)
			return KingdomWar::BoxDataPtr();
		return _box_map[id];
	}

	KingdomWar::ShopDataList kingdomwar_shop_system::getShopList(int nation)
	{
		KingdomWar::ShopDataList shop_list;
		if (nation == Kingdom::null)
			return shop_list;

		KingdomWar::ShopDataList fix = _shopFix[nation];
		KingdomWar::ShopDataList rd = _shopRd[nation];

		unsigned rdNum = 0;
		if (fix.size() < 8)
			rdNum = 8 - fix.size();
		shop_list = fix;

		unsigned tmp_total = _weight[nation];
		for (unsigned i = 0; i < rdNum; i++)
		{
			const unsigned rd_num = Common::randomBetween(0, tmp_total - 1);
			unsigned real_num = 0;
			for (unsigned idx = 0; idx < rd.size(); idx++)
			{
				real_num += rd[idx]->_weight;
				if (real_num > rd_num)
				{
					shop_list.push_back(rd[idx]);
					tmp_total -= rd[idx]->_weight;
					rd.erase(rd.begin() + idx);
					break;
				}
			}
		}
		return shop_list;
	}

	int kingdomwar_shop_system::getFlushCost(unsigned times) const
	{
		if (times >= _flush_costs.size())
			times = _flush_costs.size() - 1;
		return _flush_costs[times];
	}

	void kingdomwar_shop_system::shopInfo(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);
		if (!d || d->Info->Nation() == Kingdom::null) 
			Return(r, err_illedge);

		d->KingDomWarShop->update();
	}

	void kingdomwar_shop_system::buy(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);
		if (!d || d->Info->Nation() == Kingdom::null) 
			Return(r, err_illedge);

		ReadJsonArray;
		int pos = js_msg[0u].asInt();
		int id = js_msg[1u].asInt();

		int res = d->KingDomWarShop->buy(pos, id);
		Return(r, res);
	}

	void kingdomwar_shop_system::flush(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);
		if (!d || d->Info->Nation() == Kingdom::null) 
			Return(r, err_illedge);

		int res = d->KingDomWarShop->flush();
		Return(r, res);
	}

	void kingdomwar_shop_system::boxRewardReq(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);
		if (!d || d->Info->Nation() == Kingdom::null)
			Return(r, err_illedge);
		
		d->KingDomWarShop->updateBox();
	}

	void kingdomwar_shop_system::getBoxRewardReq(net::Msg& m, Json::Value& r)
	{
		playerDataPtr d = player_mgr.getPlayer(m.playerID);
		if (!d || d->Info->Nation() == Kingdom::null)
			Return(r, err_illedge);
		
		ReadJsonArray;
		int id = js_msg[0u].asInt();
		int res = d->KingDomWarShop->getBoxReward(id);
		Return(r, res);
	}
}
