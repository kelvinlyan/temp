#include "player_kingdomwar_shop.h"
#include "kingdomwar_shop.h"

namespace gg
{
	playerKingdomWarBox::playerKingdomWarBox(playerData* const own)
		: _auto_player(own)
	{
		_box_state.assign(5, 0);
	}

	void playerKingdomWarBox::load(const mongo::BSONElement& obj)
	{
		std::vector<mongo::BSONElement> ele = obj.Array();
		for (unsigned i = 0; i < ele.size(); ++i)
			_box_state[i] = ele[i].Int();
	}

	mongo::BSONArray playerKingdomWarBox::toBSON() const
	{
		mongo::BSONArrayBuilder b;
		ForEachC(std::vector<int>, it, _box_state)
			b.append(*it);
		return b.arr();
	}

	void playerKingdomWarBox::_auto_update()
	{
		update();
	}

	void playerKingdomWarBox::update()
	{
		qValue m;
		m.append(res_sucess);
		qValue q;
		ForEachC(std::vector<int>, it, _box_state)
			q.append(*it);
		m.append(q);
		Own().sendToClientFillMsg(gate_client::kingdom_war_box_reward_resp, m);
	}

	int playerKingdomWarBox::getReward(int id)
	{
		KingdomWar::BoxDataPtr ptr = kingdomwar_shop.getBoxData(id);
		if (!ptr)
			return err_illedge;

		if (Own().KingDomWar->getExploit() < ptr->_target
			|| _box_state[id] != 0)
			return err_illedge;

		int res = actionDoBox(Own().getOwnDataPtr(), ptr->_box, false);
		if (res == res_sucess)
		{
			_box_state[id] = 1;
			_sign_update();
		}

		return res_sucess;
	}

	playerKingdomWarShop::playerKingdomWarShop(playerData* const own)
		: _auto_player(own), _flush_times(0), _used_exploit(0)
	{
		_box_state = Creator<playerKingdomWarBox>::Create(own);
	}

	void playerKingdomWarShop::loadDB()
	{
		mongo::BSONObj key = BSON(strPlayerID << Own().ID());
		mongo::BSONObj obj = db_mgr.FindOne(DBN::dbPlayerKingdomWarShop, key);
		
		if (obj.isEmpty()) 
			return;

		checkNotEoo(obj["ep"])
			_used_exploit = obj["ep"].Int();
		checkNotEoo(obj["ft"])
			_flush_times = obj["ft"].Int();
		checkNotEoo(obj["sr"])
		{
			std::vector<mongo::BSONElement> ele = obj["sr"].Array();
			for (unsigned i = 0; i < ele.size(); ++i)
				_records.push_back(Record(ele[i]["id"].Int(), ele[i]["bt"].Int()));
		}
		checkNotEoo(obj["bs"])
			_box_state->load(obj["bs"]);
	}

	bool playerKingdomWarShop::_auto_save()
	{
		mongo::BSONObj key = BSON(strPlayerID << Own().ID());
		mongo::BSONObjBuilder obj;
		obj << strPlayerID << Own().ID() << "ft" << _flush_times << "ep" << _used_exploit
			<< "bs" << _box_state->toBSON();
		{
			mongo::BSONArrayBuilder b;
			ForEachC(std::vector<Record>, it, _records)
				b.append(BSON("id" << it->_id << "bt" << it->_buy_times));
			obj << "sr" << b.arr();
		}
		return db_mgr.SaveMongo(DBN::dbPlayerKingdomWarShop, key, obj.obj());
	}

	void playerKingdomWarShop::_auto_update()
	{
		update();
	}

	void playerKingdomWarShop::update()
	{
		checkShopRecord();
		
		qValue m;
		m.append(res_sucess);
		qValue sl;
		ForEachC(std::vector<Record>, it, _records)
		{
			qValue tmp;
			tmp.append(it->_id);
			tmp.append(it->_buy_times);
			sl.append(tmp);
		}
		qValue q(qJson::qj_object);
		q.addMember("sl", sl);
		q.addMember("ft", _flush_times);
		q.addMember("ep", Own().KingDomWar->getTotalExploit() - _used_exploit);
		m.append(q);
		Own().sendToClientFillMsg(gate_client::kingdom_war_shop_info_resp, m);
	}

	int playerKingdomWarShop::buy(int pos, int id)
	{
		if(pos <= 0 || pos > _records.size())
			return err_illedge;

		Record& rcd = _records[pos - 1];
		if (rcd._id != id)
			return err_illedge;

		KingdomWar::ShopDataPtr ptr = kingdomwar_shop.getShopData(id);
		if (!ptr)
			return err_illedge;

		if (rcd._buy_times >= ptr->_buy_times)
			return err_goods_sale_out;
		
		if (Own().KingDomWar->getTotalExploit() - _used_exploit < ptr->_consume_con)
			return err_exploit_coin_not_enough;

		int res = actionDoBox(Own().getOwnDataPtr(), ptr->_box, false);
		if (res == res_sucess)
		{
			//int tmp = Own().Res->getExploitCoin();
			++rcd._buy_times;
			//Own().Res->alterExploitCoin(0 - ptr->_consume_con);
			_used_exploit += ptr->_consume_con;
			_sign_auto();
			//Log(DBLOG::strLogSearch, Own().getOwnDataPtr(), 2, tmp, Own().Res->getSearchPoints(), id, 1);
		}

		return res;
	}

	int playerKingdomWarShop::flush()
	{
		int cost = kingdomwar_shop.getFlushCost(_flush_times);
		if (Own().Res->getCash() < cost)
			return err_gold_not_enough;
		Own().Res->alterCash(0 - cost);
		++_flush_times;
		_records.clear();
		checkShopRecord();
		_sign_auto();
		return res_sucess;
	}

	void playerKingdomWarShop::checkShopRecord()
	{
		if (_records.empty())
		{
			KingdomWar::ShopDataList shop_list = kingdomwar_shop.getShopList(Own().Info->Nation());
			ForEachC(KingdomWar::ShopDataList, it, shop_list)
				_records.push_back(Record((*it)->_id, 0));
			_sign_auto();
		}
	}

	void playerKingdomWarShop::dailyTick()
	{
		_records.clear();
		checkShopRecord();
		_flush_times = 0;
	}

	void playerKingdomWarShop::updateBox()
	{
		_box_state->update();
	}

	int playerKingdomWarShop::getBoxReward(int id)
	{
		int res = _box_state->getReward(id);
		if (res == res_sucess)
			_sign_save();
		return res;
	}
}
