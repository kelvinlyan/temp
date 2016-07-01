#include "player_kingdomwar_output.h"
#include "kingdomwar_system.h"

namespace gg
{
	playerKingdomWarOutput::playerKingdomWarOutput(playerData* const own)
		: _auto_player(own), _next_output_time(0)
	{
	}

	void playerKingdomWarOutput::loadDB()
	{
		mongo::BSONObj key = BSON(strPlayerID << Own().ID());
		mongo::BSONObj obj = db_mgr.FindOne(DBN::dbPlayerKingdomWarOutput, key);
		
		if (obj.isEmpty()) 
			return;

		checkNotEoo(obj["o"])
		{
			std::vector<mongo::BSONElement> ele = obj["o"].Array();
			for (unsigned i = 0; i < ele.size(); ++i)
				_output.push_back(ele[i].Int());
		}
		checkNotEoo(obj["t"])
			_next_output_time = obj["t"].Int();
	}

	bool playerKingdomWarOutput::_auto_save()
	{
		mongo::BSONObj key = BSON(strPlayerID << Own().ID());
		mongo::BSONObjBuilder obj;
		obj << strPlayerID << Own().ID() << "t" << _next_output_time;
		{
			mongo::BSONArrayBuilder b;
			ForEachC(Output, it, _output)
				b.append(*it);
			obj << "o" << b.arr();
		}
		return db_mgr.SaveMongo(DBN::dbPlayerKingdomWarOutput, key, obj.obj());
	}

	void playerKingdomWarOutput::_auto_update()
	{
		update();
	}

	void playerKingdomWarOutput::init()
	{
		if (_next_output_time == 0)
			_next_output_time = kingdomwar_sys.next5MinTime();

		while (_output.size() < kingdomwar_sys.citySize())
		{
			KingdomWar::CityPtr ptr = kingdomwar_sys.getCity(_output.size());
			_output.push_back(ptr->getOutput(Own().Info->Nation()));
			_sign_save();
		}
	}

	void playerKingdomWarOutput::update()
	{
		init();

		qValue m;
		m.append(res_sucess);
		qValue q(qJson::qj_object);
		q.addMember("t", kingdomwar_sys.next5MinTime());
		if (!outputEmpty())
		{
			qValue tmp;
			for (unsigned i = 0; i < _output.size(); ++i)
				tmp.append(kingdomwar_sys.getCity(i)->getOutput(Own().Info->Nation()) > _output[i]? 1 : 0);
			q.addMember("l", tmp);
		}
		m.append(q);
		Own().sendToClientFillMsg(gate_client::kingdom_war_output_info_resp, m);
	}

	inline bool playerKingdomWarOutput::outputEmpty() const
	{
		return _next_output_time >= kingdomwar_sys.next5MinTime();
	}

	int playerKingdomWarOutput::getOutput(int id)
	{
		if (outputEmpty())
			return err_illedge;
		
		if (id >= 0)
		{
			KingdomWar::CityPtr ptr = kingdomwar_sys.getCity(id);
			if (!ptr) 
				return err_illedge;
			if (id >= _output.size())
				return err_illedge;
			
			int val = ptr->getOutput(Own().Info->Nation()) - _output[id];
			if (val <= 0)
				return err_illedge;
			if (val >= ptr->maxOutput())
				val = ptr->maxOutput();

			doGetOutput(ptr->outputType(), val);
			_output[id] = ptr->getOutput(Own().Info->Nation());
			kingdomwar_sys.outputRet()[ptr->outputType()] = 
				kingdomwar_sys.outputRet()[ptr->outputType()].asInt() + val;
		}
		else
		{
			for (unsigned i = 0; i < _output.size(); ++i)
				getOutput(i);

			_next_output_time = kingdomwar_sys.next5MinTime();
		}

		_sign_auto();
		return res_sucess;
	}

	void playerKingdomWarOutput::doGetOutput(int type, int val)
	{
		switch(type)
		{
			case 0:
			{
				Own().Res->alterGold(val);
				break;
			}
			case 1:
			{
				Own().Res->alterSilver(val);
				break;
			}
			case 2:
			{
				Own().Res->alterFame(val);
				break;
			}
			case 3:
			{
				Own().Res->alterMerit(val);
				break;
			}
			default:
				break;
		}
	}
}
