#include "player_kingdomwar_fm.h"
#include "playerManager.h"
#include "man_system.h"
#include "kingdomwar_def.h"

namespace gg
{
	namespace KingdomWar
	{
		enum{
			ArmyMaxHp = 100,
		};

		int getManMaxHp(playerDataPtr player, int army_id, playerManPtr man)
		{
			mBattlePtr mbattle = Creator<manBattle>::Create();
			cfgManPtr config = man_sys.getConfig(man->mID());
			if (!config) return 0;
			mbattle->manID = man->mID();
			mbattle->battleValue = man->battleValue();
			mbattle->holdMorale = config->holdMorale;
			mbattle->set_skill_1(config->skill_1);
			mbattle->set_skill_2(config->skill_2);
			mbattle->armsType = config->armsType;
			mbattle->manLevel = man->LV();
			memcpy(mbattle->armsModule, config->armsModules, sizeof(mbattle->armsModule));
			man->toInitialAttri(mbattle->initialAttri);
			man->toBattleAttri(player->KingDomWarFM->getFMId(army_id), mbattle->battleAttri);
			return mbattle->getTotalAttri(idx_hp);
		}
	}

	playerKingdomWarSGFM::playerKingdomWarSGFM(int army_id, playerData* const own)
		: _auto_player(own), _army_id(army_id)
	{
		_fm_id = 0;
		_power = -1;
		_man_list.assign(9, playerManPtr());
	}

	void playerKingdomWarSGFM::load(const mongo::BSONObj& obj)
	{
		_fm_id = obj["i"].Int();
		const std::vector<mongo::BSONElement> ele = obj["f"].Array();
		for (unsigned i = 0; i < 9; ++i)
			_man_list[i] = Own().Man->findMan(ele[i].Int());
	}

	bool playerKingdomWarSGFM::_auto_save()
	{
		mongo::BSONObj key = BSON(strPlayerID << Own().ID() << "a" << _army_id);
		mongo::BSONObjBuilder obj;
		obj << strPlayerID << Own().ID() << "a" << _army_id << "i" << _fm_id;
		{
			mongo::BSONArrayBuilder b;
			ForEachC(ManList, it, _man_list)
				b.append((*it)? (*it)->mID() : -1);
			obj << "f" << b.arr();
		}
		return db_mgr.SaveMongo(DBN::dbPlayerKingdomWarFM, key, obj.obj());
	}

	void playerKingdomWarSGFM::getInfo(qValue& q)
	{
		if (_power == -1)
			recalFM();
		q.addMember("a", _army_id);
		q.addMember("i", _fm_id);
		q.addMember("v", _power);
		qValue f;
		ForEachC(ManList, it, _man_list)
			f.append((*it)? (*it)->mID() : -1);
		q.addMember("f", f);
	}
	
	void playerKingdomWarSGFM::_auto_update()
	{
		qValue m;
		m.append(res_sucess);
		qValue q(qJson::qj_object);
		getInfo(q);
		m.append(q);
		Own().sendToClientFillMsg(gate_client::kingdom_war_single_formation_resp, m);
	} 

	int playerKingdomWarSGFM::setFormation(int fm_id, const int fm[9])
	{
		const FMCFG::HOLEVEC& config = playerWarFM::getConfig(fm_id).HOLES;
		boost::unordered_set<int> SAMEMAN;
		for (unsigned i = 0; i < 9; ++i)
		{
			if (fm[i] < 0)continue;
			if (!SAMEMAN.insert(fm[i] / 100).second)return err_fomat_same_man;
			if (Own().KingDomWarFM->manUsed(_army_id, fm[i])) return err_fomat_same_man;
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
		_sign_auto();
		return res_sucess;
	}

	void playerKingdomWarSGFM::recalFM()
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

	void playerKingdomWarSGFM::useHpItem()
	{
		ForEach(ManList, it, _man_list)
		{
			if (!*it) continue;
			Own().KingDomWar->setManHp((*it)->mID(), KingdomWar::getManMaxHp(Own().getOwnDataPtr(), _army_id, *it));
		}
	}

	playerKingdomWarFM::playerKingdomWarFM(playerData* const own)
		: _auto_player(own)
	{
		for (int i = 0; i < KingdomWar::ArmyNum; ++i)
			_fm_list.push_back(Creator<playerKingdomWarSGFM>::Create(i, own));
	}

	void playerKingdomWarFM::loadDB()
	{
		mongo::BSONObj key = BSON(strPlayerID << Own().ID());
		objCollection objs = db_mgr.Query(DBN::dbPlayerKingdomWarFM, key);
		for (unsigned i = 0; i < objs.size(); ++i)
		{
			int army_id = objs[i]["a"].Int();
			_fm_list[army_id]->load(objs[i]);
		}
	}

	bool playerKingdomWarFM::_auto_save()
	{
		return true;
	}

	void playerKingdomWarFM::_auto_update()
	{
	}

	void playerKingdomWarFM::update()
	{
		qValue m;
		m.append(res_sucess);
		qValue q;
		for (unsigned i = 0; i < _fm_list.size(); ++i)
		{
			qValue tmp(qJson::qj_object);
			_fm_list[i]->getInfo(tmp);
			q.append(tmp);
		}
		m.append(q);
		Own().sendToClientFillMsg(gate_client::kingdom_war_all_formation_resp, m);
	}

	int playerKingdomWarFM::setFormation(int army_id, int fm_id, const int fm[9])
	{
		if (army_id < 0 || army_id >= _fm_list.size())
			return err_illedge;

		return _fm_list[army_id]->setFormation(fm_id, fm);
	}

	bool playerKingdomWarFM::manUsed(int army_id, int man_id) const
	{
		for (unsigned i = 0; i < _fm_list.size(); ++i)
		{
			if (i == army_id)
				continue;
			const ManList& ml = _fm_list[i]->manList();
			ForEachC(ManList, it, ml)
			{
				if ((*it) && (*it)->mID() == man_id)
					return true;
			}
		}
		return false;
	}
}
