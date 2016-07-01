#include "player_kingdomwar.h"
#include "kingdomwar_system.h"
#include "playerManager.h"
#include "man_system.h"

namespace gg
{
	namespace KingdomWar
	{
		enum{
			ReportSize = 20,
		};

		Report::Report(const mongo::BSONElement& obj)
		{
			_army_id = obj["a"].Int();
			_city_id = obj["c"].Int();
			_state = obj["s"].Int();
			_target_nation = obj["n"].Int();
			_target_name = obj["m"].String();
			_exploit = obj["e"].Int();
			_rep_id = obj["r"].String();
		}

		mongo::BSONObj Report::toBSON() const
		{
			return BSON("a" << _army_id << "c" << _city_id << "s" << _state << "n" << _target_nation
				<< "m" << _target_name << "e" << _exploit << "r" << _rep_id);
		}

		void Report::getInfo(qValue& q)
		{	
			q.append(_army_id);
			q.append(_city_id);
			q.append(_state);
			q.append(_target_nation);
			q.append(_target_name);
			q.append(_exploit);
			q.append(_rep_id);
		}
	}

	playerKingdomWar::playerKingdomWar(playerData* const own)
		: _auto_player(own), _report_id(0), _exploit(0), _total_exploit(0)
	{
		std::string path = "./report/kingdomwar/" + Common::toString(Own().ID());
		Common::createDirectories(path);
		_army_hp.assign(KingdomWar::ArmyNum, 100);
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
		checkNotEoo(obj["ah"])
		{
			std::vector<mongo::BSONElement> ele = obj["ah"].Array();
			for (unsigned i = 0; i < ele.size(); ++i)
				_army_hp[i] = ele[i].Int();
		}
		checkNotEoo(obj["rl"])
		{
			std::vector<mongo::BSONElement> ele = obj["rl"].Array();
			for (unsigned i = 0; i < ele.size(); ++i)
			{
				KingdomWar::ReportPtr ptr = Creator<KingdomWar::Report>::Create(ele[i]);
				_report_list.push_back(ptr);
			}
		}
		checkNotEoo(obj["ri"])
			_report_id = obj["ri"].Int();
		checkNotEoo(obj["ep"])
			_exploit = obj["ep"].Int();
		checkNotEoo(obj["te"])
			_total_exploit = obj["te"].Int();
	}

	bool playerKingdomWar::_auto_save()
	{
		mongo::BSONObj key = BSON(strPlayerID << Own().ID());
		mongo::BSONObjBuilder obj;
		obj << strPlayerID << Own().ID() << "ri" << _report_id << "ep" << _exploit
			<< "te" << _total_exploit;
		if (!_man_hp.empty())
		{
			mongo::BSONArrayBuilder b;
			ForEach(Man2HpMap, it, _man_hp)
				b.append(BSON("i" << it->first << "h" << it->second));
			obj << "mh" << b.arr();
		}
		
		{
			mongo::BSONArrayBuilder b;
			ForEach(ArmyHp, it, _army_hp)
				b.append((*it));
			obj << "ah" << b.arr();
		}
		if (!_report_list.empty())
		{
			mongo::BSONArrayBuilder b;
			ForEach(ReportList, it, _report_list)
				b.append((*it)->toBSON());
			obj << "rl" << b.arr();
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

	bool playerKingdomWar::isDead(int army_id)
	{
		if (_army_hp[army_id] < 1)
			return true;
		return false;
	}

	std::string playerKingdomWar::addReport(int army_id, int city_id, int state, int target_nation, const std::string& target_name, unsigned exploit)
	{
		++_report_id;
		if (_report_id > KingdomWar::ReportSize)
			_report_id = 1;
			
		std::string rep = Common::toString(_report_id);
		KingdomWar::ReportPtr ptr = Creator<KingdomWar::Report>::Create(army_id, city_id, state, target_nation, target_name, exploit, rep);
		_report_list.push_front(ptr);
		while (_report_list.size() > KingdomWar::ReportSize)
			_report_list.pop_back();
		std::string path = "kingdomwar/" + Common::toString(Own().ID()) + "/" + rep;
		return path;
	}

	void playerKingdomWar::updateReport()
	{
		qValue m;
		m.append(res_sucess);
		qValue q;
		ForEachC(ReportList, it, _report_list)
		{
			qValue tmp;
			(*it)->getInfo(tmp);
			q.append(tmp);
		}
		m.append(q);
		Own().sendToClientFillMsg(gate_client::kingdom_war_report_info_resp, m);
	}

	int playerKingdomWar::getExploit()
	{
		return _exploit;
	}

	int playerKingdomWar::alterExploit(int num)
	{
		if (num < 0)
			return _exploit;
		int tmp = _exploit;
		_exploit += num;
		_total_exploit += num;
		_sign_auto();
		return tmp;
	}

	int playerKingdomWar::alterArmyHp(int army_id, int num)
	{
		int tmp = _army_hp[army_id];
		_army_hp[army_id] += num;
		if (_army_hp[army_id] < 0)
			_army_hp[army_id] = 0;
		_sign_save();
		return _army_hp[army_id] - tmp;
	}

	int playerKingdomWar::useHpItem(int army_id, int num)
	{
		return res_sucess;
	}

	int playerKingdomWar::armyHp(int army_id)
	{
		return _army_hp[army_id];
	}
}
