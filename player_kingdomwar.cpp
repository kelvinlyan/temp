#include "player_kingdomwar.h"
#include "kingdomwar_system.h"
#include "playerManager.h"

namespace gg
{
	namespace KingdomWar
	{
		mongo::BSONObj Position::toBSON() const
		{
			return BSON("t" << _type << "i" << _id << "tm" << _time << "f" << _from_city_id);
		}

		void Position::load(const mongo::BSONElement& obj)
		{
			_type = obj["t"].Int();
			_id = obj["i"].Int();
			_time = obj["tm"].Int();
			_from_city_id = obj["f"].Int();
		}

		Formation::Formation(int army_id, playerData* const own)
			: _auto_player(own), _army_id(army_id)
		{
			_fm_id = 0;
			_power = 0;
			_man_list.assign(9, playerManPtr());
		}

		void Formation::load(const mongo::BSONElement& obj)
		{
			_fm_id = obj["i"].Int();
			const std::vector<mongo::BSONElement> ele = obj["f"].Array();
			for (unsigned i = 0; i < 9; ++i)
				_man_list[i] = Own().Man->findMan(ele[i].Int());
			recalFM();
		}

		mongo::BSONArray Formation::manListBSON() const
		{
			mongo::BSONArrayBuilder b;
			ForEachC(ManList, it, _man_list)
				b.append((*it)? (*it)->mID() : -1);
			return b.arr();
		}

		mongo::BSONObj Formation::toBSON() const
		{
			return BSON("i" << _fm_id << "f" << manListBSON());
		}

		void Formation::getInfo(qValue& q) const
		{
			q.addMember("a", _army_id);
			q.addMember("i", _fm_id);
			q.addMember("v", _power);
			qValue tmp;
			ForEachC(ManList, it, _man_list)
				tmp.append((*it)? (*it)->mID() : -1);
			q.addMember("f", tmp);
		}

		int Formation::setFormation(int fm_id, const int fm[9])
		{
			const FMCFG::HOLEVEC& config = playerWarFM::getConfig(fm_id).HOLES;
			boost::unordered_set<int> SAMEMAN;
			for (unsigned i = 0; i < 9; ++i)
			{
				if (fm[i] < 0)continue;
				if (!SAMEMAN.insert(fm[i] / 100).second)return err_fomat_same_man;
				if (Own().KingDomWar->manUsed(_army_id, fm[i])) return err_fomat_same_man;
			}

			ManList tmpList;
			tmpList.assign(9, playerManPtr());
			for (unsigned i = 0; i < 9; ++i)
			{
				if (fm[i] < 0)continue;
				if (!config[i].Use)continue;
				if (Own().LV() < config[i].LV)return err_format_hole_limit;
				if (config[i].Process > 0 && !Own().War->isChallenge(config[i].Process))return err_format_hole_limit;
				playerManPtr man = Own().Man->findMan(fm[i]);//ÊÇ·ñÂú×ã¿ªÆôÌõ¼þ
				tmpList[i] = man;
			}

			_man_list = tmpList;
			recalFM();
			_sign_update();
			return res_sucess;
		}

		void Formation::recalFM()
		{
			_power = 0;
			unsigned num = 0;
			for (unsigned i = 0; i < 9; ++i)
			{
				playerManPtr& man = _man_list[i];
				if (!man)continue;
				++num;
				_power += man->battleValue();
			}
			const FMCFG& config = playerWarFM::getConfig(_fm_id);
			const int scLV = Own().Research->getForLV(config.techID);
			_power += (scLV * 7 * num);
			//Èç¹ûÔ½½ç//ÄÇÃ´10ÒÚ
			const static int MaxValue = 1000000000;//10ÒÚ
			if (_power < 0 || _power > MaxValue)
				_power = MaxValue;
		}

		bool Formation::_auto_save()
		{
			return true;
		}

		void Formation::_auto_update()
		{
			update();
		}

		void Formation::update()
		{
			qValue m;
			m.append(res_sucess);
			qValue q(qJson::qj_object);
			getInfo(q);
			m.append(q);
			Own().sendToClientFillMsg(gate_client::kingdom_war_formation_resp, m);
		}

		Army::Army(int id, playerData* const own)
		{
			_id = id;
			_hp = 100;
			_pos = Creator<Position>::Create();
			_fm = Creator<Formation>::Create(id, own);
		}

		mongo::BSONObj Army::toBSON() const
		{
			return BSON("i" << _id << "h" << _hp << "f" << _fm->toBSON() << "p" << _pos->toBSON());
		}

		void Army::load(const mongo::BSONElement& obj)
		{
			_hp = obj["h"].Int();
			_fm->load(obj["f"]);
			_pos->load(obj["p"]);
		}
	}

	playerKingdomWar::playerKingdomWar(playerData* const own)
		: _auto_player(own)
	{
		for (unsigned i = 0; i < 3; ++i)
			_armys.push_back(Creator<KingdomWar::Army>::Create(i, own));
	}

	void playerKingdomWar::loadDB()
	{
		mongo::BSONObj key = BSON(strPlayerID << Own().ID());
		mongo::BSONObj obj = db_mgr.FindOne(DBN::dbPlayerKingdomWar, key);
		if (obj.isEmpty())
			return;
		
		checkNotEoo(obj["mh"])
		{
			std::vector<mongo::BSONElement> ele = obj["mh"].Array();
			for (unsigned i = 0; i < ele.size(); ++i)
				_man_hp[ele[i]["i"].Int()] = ele[i]["h"].Int();
		}
		checkNotEoo(obj["am"])
		{
			std::vector<mongo::BSONElement> ele = obj["am"].Array();
			for (unsigned i = 0; i < ele.size(); ++i)
				_armys[i]->load(ele[i]);
		}
	}

	bool playerKingdomWar::_auto_save()
	{
		mongo::BSONObj key = BSON(strPlayerID << Own().ID());
		mongo::BSONObjBuilder obj;
		obj << strPlayerID << Own().ID();
		if (!_man_hp.empty())
		{
			mongo::BSONArrayBuilder b;
			ForEach(Man2HpMap, it, _man_hp)
				b.append(BSON("i" << it->first << "h" << it->second));
			obj << "mh" << b.arr();
		}
		
		{
			mongo::BSONArrayBuilder b;
			ForEach(KingdomWar::Armys, it, _armys)
				b.append((*it)->toBSON());
			obj << "am" << b.arr();
		}
		return db_mgr.SaveMongo(DBN::dbPlayerKingdomWar, key, obj.obj());
	}

	void playerKingdomWar::_auto_update()
	{
		
	}

	int playerKingdomWar::manHp(int man_id) const
	{
		Man2HpMap::const_iterator it = _man_hp.find(man_id);
		return it != _man_hp.end()? it->second : 0;
	}

	void playerKingdomWar::setManHp(int man_id, int hp)
	{
		_man_hp[man_id] = hp;
		_sign_save();
	}

	void playerKingdomWar::setPosition(int army_id, int type, int id, unsigned time, int from_city_id)
	{
		if (army_id >= _armys.size())
			return;

		KingdomWar::ArmyPtr& ptr = _armys[army_id];
		//KingdomWar::Position pos = *(ptr->_pos);
		ptr->_pos->_type = type;
		ptr->_pos->_id = id;
		ptr->_pos->_time = time;
		if (from_city_id != -1)
			ptr->_pos->_from_city_id = from_city_id;
		_sign_save();
	}

	bool playerKingdomWar::isDead(int army_id)
	{
		if (army_id < 0 || army_id >= _armys.size())
			return true;
		KingdomWar::ArmyPtr& ptr = _armys[army_id];
		if (ptr->_hp <= 0)
			return true;
		return false;
	}

	KingdomWar::PositionPtr playerKingdomWar::getPosition(int army_id)
	{
		if (army_id < 0 || army_id >= _armys.size())
			return KingdomWar::PositionPtr();
		return _armys[army_id]->_pos;
	}

	const KingdomWar::ManList& playerKingdomWar::getFM(int army_id) const
	{
		return _armys[army_id]->_fm->manList();
	}

	int playerKingdomWar::getBV(int army_id)
	{
		return 0;
	}

	bool playerKingdomWar::manUsed(int army_id, int man_id)
	{
		return false;
	}

	bool playerKingdomWar::inited() const
	{
		return _armys.size() >= KingdomWar::ArmyNum;
	}

	void playerKingdomWar::initData()
	{
		if (Own().Info->Nation() == Kingdom::null)
			return;
		unsigned i = _armys.size();
		for (; i < KingdomWar::ArmyNum; ++i)
		{
			_armys.push_back(Creator<KingdomWar::Army>::Create(i, _Own));
			kingdomwar_sys.goBackMainCity(0, Own().getOwnDataPtr(), i);
		}
	}
}
