#include "player_kingdomwar_pos.h"
#include "playerManager.h"
#include "kingdomwar_system.h"
#include "kingdomwar_def.h"

namespace gg
{
	namespace KingdomWar
	{
		Position::Position()
			: _type(PosEmpty), _from_city_id(-1)
		{
		}
	}

	playerKingdomWarSGPos::playerKingdomWarSGPos(int army_id, playerData* const own)
		: _auto_player(own), _army_id(army_id)
	{
	}

	void playerKingdomWarSGPos::load(const mongo::BSONObj& obj)
	{
		_pos._type = obj["t"].Int();
		_pos._id = obj["i"].Int();
		_pos._time = obj["tm"].Int();
		_pos._from_city_id = obj["f"].Int();
		std::vector<mongo::BSONElement> ele = obj["p"].Array();
		for (unsigned i = 0; i < ele.size(); ++i)
			_pos._line.push_back(ele[i].Int());
	}

	bool playerKingdomWarSGPos::_auto_save()
	{
		mongo::BSONObj key = BSON(strPlayerID << Own().ID() << "a" << _army_id);
		mongo::BSONObjBuilder obj;
		obj << strPlayerID << Own().ID() << "a" << _army_id << "t" << _pos._type
			<< "i" << _pos._id << "tm" << _pos._time << "f" << _pos._from_city_id;
		{
			mongo::BSONArrayBuilder b;
			ForEachC(KingdomWar::Position::IdList, it, _pos._line)
				b.append(*it);
			obj << "p" << b.arr();
		}
		return db_mgr.SaveMongo(DBN::dbPlayerKingdomWarPos, key, obj.obj());
	}

	void playerKingdomWarSGPos::_auto_update()
	{
		qValue m;
		m.append(res_sucess);
		qValue q(qJson::qj_object);
		getInfo(q);
		m.append(q);
		Own().sendToClientFillMsg(gate_client::kingdom_war_player_update_resp, m);
	}

	void playerKingdomWarSGPos::getInfo(qValue& q)
	{
		if (_pos._type == KingdomWar::PosEmpty)
			kingdomwar_sys.goBackMainCity(0, Own().getOwnDataPtr(), _army_id);

		q.addMember("i", _army_id);
		q.addMember("h", Own().KingDomWar->armyHp(_army_id));
		qValue p(qJson::qj_object);
		p.addMember("t", _pos._type);
		p.addMember("i", _pos._id);
		p.addMember("tm", _pos._time);
		p.addMember("f", _pos._from_city_id);
		qValue l;
		ForEachC(KingdomWar::Position::IdList, it, _pos._line)
			l.append(*it);
		p.addMember("p", l);
		q.addMember("p", p);
	}

	void playerKingdomWarSGPos::setLine(const KingdomWar::Line& line)
	{
		for (unsigned i = 1; i < line.size(); ++i)
			_pos._line.push_back(line[i]);
		_sign_auto();
	}

	void playerKingdomWarSGPos::setPosition(int type, int id, unsigned time)
	{
		if (_pos._type != KingdomWar::PosPath)
			_pos._from_city_id = _pos._id;
		_pos._type = type;
		_pos._id = id;
		_pos._time = time;
		if (_pos._type == KingdomWar::PosCity
			&& !_pos._line.empty())
		{
			_pos._line.pop_front();
			if (!_pos._line.empty())
			{
				_auto_update();
				move(time, _pos._line.front(), false);
				return;
			}
		}
		_sign_auto();
	}

	int playerKingdomWarSGPos::move(unsigned time, int to_city_id, bool start)
	{
		if (_pos._type == KingdomWar::PosPath)
			return err_illedge;

		KingdomWar::CityPtr ptr = kingdomwar_sys.getCity(_pos._id);	
		if (!ptr)
			return err_illedge;
		
		if (ptr->onFight(Own().getOwnDataPtr(), _army_id))
			return err_illedge;
		
		if (start)
		{
			const KingdomWar::LineInfo& line = kingdomwar_sys.getLine(_pos._id, to_city_id);
			if (line.empty())
				return err_illedge;
			setLine(line.get());
			to_city_id = _pos._line.front();	
		}
		
		return ptr->leave(time, Own().getOwnDataPtr(), _army_id, to_city_id);
	}

	int playerKingdomWarSGPos::retreat()
	{
		if (_pos._type == KingdomWar::PosPath)
			return err_illedge;

		KingdomWar::CityPtr ptr = kingdomwar_sys.getCity(_pos._id);	
		if (!ptr)
			return err_illedge;
		
		if (ptr->onFight(Own().getOwnDataPtr(), _army_id))
			return err_illedge;
		
		return ptr->leave(Common::gameTime(), Own().getOwnDataPtr(), _army_id, _pos._from_city_id);
	}

	playerKingdomWarPos::playerKingdomWarPos(playerData* const own)
		: _auto_player(own)
	{
		for (unsigned i = 0; i < KingdomWar::ArmyNum; ++i)
			_pos_list.push_back(Creator<playerKingdomWarSGPos>::Create(i, own));
	}

	void playerKingdomWarPos::loadDB()
	{
		mongo::BSONObj key = BSON(strPlayerID << Own().ID());
		objCollection objs = db_mgr.Query(DBN::dbPlayerKingdomWarPos, key);
		for (unsigned i = 0; i < objs.size(); ++i)
		{
			int army_id = objs[i]["a"].Int();
			_pos_list[army_id]->load(objs[i]);
		}
	}

	void playerKingdomWarPos::_auto_update()
	{
	}

	bool playerKingdomWarPos::_auto_save()
	{
		return true;
	}

	void playerKingdomWarPos::update()
	{
		qValue m;
		m.append(res_sucess);
		qValue q;
		for (unsigned i = 0; i < _pos_list.size(); ++i)
		{
			qValue tmp(qJson::qj_object);
			_pos_list[i]->getInfo(tmp);
			q.append(tmp);
		}
		m.append(q);
		Own().sendToClientFillMsg(gate_client::kingdom_war_player_info_resp, m);
	}

	void playerKingdomWarPos::setPosition(int army_id, int type, int id, unsigned time)
	{
		_pos_list[army_id]->setPosition(type, id, time);
	}

	void playerKingdomWarPos::setLine(int army_id, const KingdomWar::Line& l)
	{
		_pos_list[army_id]->setLine(l);
	}

	int playerKingdomWarPos::move(unsigned time, int army_id, int to_city_id, bool start)
	{
		if (army_id < 0 || army_id >= KingdomWar::ArmyNum)
			return err_illedge;

		return _pos_list[army_id]->move(time, to_city_id, start);
	}

	int playerKingdomWarPos::retreat(int army_id)
	{
		if (army_id < 0 || army_id >= KingdomWar::ArmyNum)
			return err_illedge;

		return _pos_list[army_id]->retreat();
	}
}
