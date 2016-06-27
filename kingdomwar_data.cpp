#include "kingdomwar_data.h"
#include "playerManager.h"
#include "kingdomwar_system.h"

namespace gg
{
	namespace KingdomWar
	{
		enum
		{
			// param
			QueueMemNum = 8,
			QueueNum = 3,
			DefSpeed = 10,
			AttackerNum = 5,
			DefenderNum = 5,
			AttackInterval = 5,
			AttackerWaitTime = 10,
			DefenderWaitTime = 10,

			// queue state
			Free = 0,
			Wait,
			Busy,

			// fighter type
			TypePlayer = 0,
			TypeNpc,

			// update type
			Enter = 0,
			Leave,
			Defeated,

			// battle field state
			Closed = 0,
			Opened,
			AttackerWait,
			DefenderWait,
		};

		PathItem::PathItem(unsigned time, int side, playerDataPtr d, int army_id)
		{
			_start_time = time;		
			_side = side;
			_pid = d->ID();
			_army_id = army_id;
			_name = d->Name();
			_nation = d->Info->Nation();
			_trigger_time = -1;
		}

		inline void PathItem::getInfo(qValue& q) const
		{
			q.append(_pid);
			q.append(_army_id);
			q.append(_nation);
			q.append(_start_time);
		}

		inline void UpdateItem::getInfo(qValue& q) const
		{
			q.append(_type);
			q.append(_ptr->pid());
			q.append(_ptr->armyId());
			if (_type == Enter)
			{
				q.append(_ptr->nation());
				q.append(_ptr->startTime());
			}
		}

		bool PathItemSorter::getMin(PLIter& iter)
		{
			if (_sort_list.empty())
				return false;
			iter = _sort_list.begin()->_target;
			return true;
		}

		inline void PathItemSorter::push(const PLIter& iter)
		{
			_sort_list.insert(SortItem(iter));
		}

		void PathItemSorter::pop(const PLIter& iter)
		{
			typedef SortList::iterator SortIter;
			SortItem target(iter);
			std::pair<SortIter, SortIter> pair_iter = _sort_list.equal_range(target);
			SortIter item = pair_iter.first;
			for(; item != pair_iter.second; ++item)
			{
				if (*item == target)
				{
					_sort_list.erase(item);
					break;
				}
			}
		}

		PathItemMgr::PathItemMgr(const int& path_id)
			: _path_id(path_id), _distance(-1)
		{
			_item_list = getInitedPathItemList();
			_left_end = _item_list.begin();
			_right_end = ++_item_list.begin();
		}

		void PathItemMgr::setPathType(int side, int path_type, int cost_time)
		{
			_type[side] = path_type;
			if (_distance == -1)
			{
				_distance = DefSpeed * cost_time;
				_speed[side] = DefSpeed;
			}
			else
			{
				_speed[side] = divide32(_distance, cost_time);
			}
		}

		PathItemList PathItemMgr::getInitedPathItemList()
		{
			PathItemList p;
			p.push_back(PathItemPtr());
			p.push_back(PathItemPtr());
			return p;
		}

		bool PathItemMgr::empty() const
		{
			return _item_list.size() < 3;
		}

		void PathItemMgr::getMainInfo(qValue& q) const
		{
			if (empty())
				return;
			q.append(_path_id);
			qValue lhs;
			qValue rhs;
			PLIter iter = _left_end;
			++iter;
			for (; iter != _right_end; ++iter)
			{
				qValue tmp;
				(*iter)->getInfo(tmp);
				if ((*iter)->side() == Left)
					lhs.append(tmp);
				else
					rhs.append(tmp);
			}
			q.append(lhs);
			q.append(rhs);
		}

		void PathItemMgr::getUpdateInfo(qValue& q) const
		{
			if (_updates.empty())
				return;
			q.append(_path_id);
			qValue lhs;
			qValue rhs;
			ForEachC(UpdateItems, it, _updates)
			{
				qValue tmp;
				it->getInfo(tmp);
				if (it->side() == Left)
					lhs.append(tmp);
				else
					rhs.append(tmp);
			}
			q.append(lhs);
			q.append(rhs);
			_updates.clear();
		}

		void PathItemMgr::push(const PathItemPtr& item)
		{
			PLIter iter;
			if (item->side() == Left)
			{
				iter = _left_end;
				++iter;
			}
			else
				iter = _right_end;

			iter = _item_list.insert(iter, item);
			PLIter n_iter = getNext(iter);
			int trigger_time = getTriggerTime(iter, n_iter);
			setTriggerTime(iter, trigger_time);
			if (n_iter != _left_end && 
				n_iter != _right_end &&
				(*n_iter)->side() != (*iter)->side())
				setTriggerTime(n_iter, trigger_time);

			_updates.push_back(UpdateItem(Enter, item));
		}

		void PathItemMgr::pop(const PLIter& iter, int type)
		{
			_sorter.pop(iter);
			PLIter l_iter = iter;
			--l_iter;
			PLIter r_iter = iter;
			++r_iter;
			_updates.push_back(UpdateItem(type, *iter));
			_item_list.erase(iter);
			if (l_iter != _left_end && (*l_iter)->side() == Left)
				setTriggerTime(l_iter, getTriggerTime(l_iter, r_iter));
			if (r_iter != _right_end && (*r_iter)->side() == Right)
				setTriggerTime(r_iter, getTriggerTime(r_iter, l_iter));
		}

		void PathItemMgr::swap(PLIter& lhs, PLIter& rhs)
		{
			PathItemPtr ptr = *lhs;
			*lhs = *rhs;
			*rhs = ptr;
			PLIter iter = lhs;
			--iter;
			if (iter != _left_end && (*iter)->side() == Left)
				setTriggerTime(iter, getTriggerTime(iter, lhs));
			setTriggerTime(lhs, getTriggerTime(lhs, iter));
			iter = rhs;
			++iter;
			setTriggerTime(rhs, getTriggerTime(rhs, iter));
			if (iter != _right_end && (*iter)->side() == Right)
				setTriggerTime(iter, getTriggerTime(iter, rhs));
		}

		bool PathItemMgr::getMin(PLIter& iter)
		{
			return _sorter.getMin(iter);
		}

		void PathItemMgr::run(const Handler& h)
		{
			PLIter iter = _left_end;
			++iter;
			for(; iter != _right_end; ++iter)
				h(*iter);
		}

		void PathItemMgr::clear()
		{
			_updates.clear();
			_sorter.clear();
			_item_list = getInitedPathItemList();
			_left_end = _item_list.begin();
			_right_end = ++_item_list.begin();
		}

		PLIter PathItemMgr::getNext(const PLIter& iter)
		{
			PLIter temp = iter;
			return (*temp)->side() == Left? ++temp : --temp;
		}

		int PathItemMgr::getTriggerTime(const PLIter& lhs, const PLIter& rhs)
		{
			if (rhs == _left_end || rhs == _right_end)
				return (*lhs)->startTime() + divide32(_distance, _speed[(*lhs)->side()]);
			else
			{
				if ((*lhs)->side() == (*rhs)->side())
					return -1;
				else
				{
					int pass_distance = 0;
					int start_time = 0;
					if ((*lhs)->startTime() > (*rhs)->startTime())
					{
						pass_distance = ((*lhs)->startTime() - (*rhs)->startTime()) * _speed[Right];
						start_time = (*rhs)->startTime();
					}
					else
					{
						pass_distance = ((*rhs)->startTime() - (*lhs)->startTime()) * _speed[Left];
						start_time = (*lhs)->startTime();
					}

					I64 total = (I64)_distance - pass_distance;
					I64 speed = _speed[Left] + _speed[Right];
					return (int)divide64(total, speed) + start_time;
				}
			}
		}

		void PathItemMgr::setTriggerTime(PLIter& iter, int trigger_time)
		{
			if ((*iter)->triggerTime() != trigger_time)
			{
				if ((*iter)->triggerTime() != -1)
					_sorter.pop(iter);
				(*iter)->setTriggerTime(trigger_time);
				if ((*iter)->triggerTime() != -1)
					_sorter.push(iter);
			}
		}

		Path::Path(int path_id, int from_id, int to_id)
			: _manager(path_id)
		{
			_id = path_id;
			if (from_id < to_id)
			{
				_linked_city[Left] = kingdomwar_sys.getCity(from_id);
				_linked_city[Right] = kingdomwar_sys.getCity(to_id);
			}
			else
			{
				_linked_city[Left] = kingdomwar_sys.getCity(to_id);
				_linked_city[Right] = kingdomwar_sys.getCity(from_id);
			}
		}

		void Path::setPathType(int from_id, int to_id, int path_type, unsigned time)
		{
			int side = from_id == _linked_city[Left]->id()? Left : Right;
			_manager.setPathType(side, path_type, time);
		}

		bool Path::access(int from_id, int to_id) const
		{
			if (from_id == to_id)
				return false;
			if (from_id != _linked_city[Left]->id() && to_id != _linked_city[Left]->id())
				return false;
			if (from_id != _linked_city[Right]->id() && to_id != _linked_city[Right]->id())
				return false;
			int side = from_id == _linked_city[Left]->id()? Left : Right;
			return _manager.access(side);
		}

		int Path::enter(unsigned time, playerDataPtr d, int army_id, int to_city_id)
		{
			int side = to_city_id == _linked_city[Left]->id()? Right : Left;
			int from_city_id = to_city_id == _linked_city[Left]->id()? _linked_city[Right]->id() :  _linked_city[Left]->id();
			_manager.push(Creator<PathItem>::Create(time, side, d, army_id));
			d->KingDomWar->setPosition(army_id, PosPath, _id, time, from_city_id);
			resetTimer();
			return res_sucess;
		}

		int Path::arrive(int timer_id, PLIter& iter)
		{
			if (timerOverTime(timer_id))
				return res_sucess;
			CityPtr ptr = (*iter)->side() == Left?
				_linked_city[Right] : _linked_city[Left];
			unsigned time = (*iter)->triggerTime();
			playerDataPtr d = player_mgr.getPlayer((*iter)->pid());
			if (!d)  return ServerError;
			ptr->enter(time, d, (*iter)->armyId());
			_manager.pop(iter, Leave);
			resetTimer();
			return res_sucess;
		}
		
		int Path::meet(int timer_id, PLIter& lhs, PLIter& rhs)
		{
			if (timerOverTime(timer_id))
				return res_sucess;
			_manager.swap(lhs, rhs);
			resetTimer();
			return res_sucess;
		}

		int Path::attack(int timer_id, PLIter& lhs, PLIter& rhs)
		{
			if (timerOverTime(timer_id))
				return res_sucess;
			
			unsigned time = (*lhs)->triggerTime();
			playerDataPtr d = player_mgr.getPlayer((*lhs)->pid());
			playerDataPtr target = player_mgr.getPlayer((*rhs)->pid());
			if (!d || !target)
			{
				// error
				return ServerError;
			}
			sBattlePtr atk = kingdomwar_sys.getBattlePtr(d, (*lhs)->armyId());
			sBattlePtr def = kingdomwar_sys.getBattlePtr(target, (*rhs)->armyId());
			O2ORes resultB = battle_sys.One2One(atk, def, typeBattle::kingdomwar);
			if (resultB.res == resBattle::atk_win)
			{
				kingdomwar_sys.goBackMainCity(time, target, (*rhs)->armyId());
				_manager.pop(rhs, Defeated);
			}
			else
			{
				kingdomwar_sys.goBackMainCity(time, d, (*lhs)->armyId());
				_manager.pop(lhs, Defeated);
			}
			resetTimer();
			return res_sucess;
		}

		inline bool Path::timerOverTime(int timer_id)
		{
			return timer_id != _timer_id;
		}

		void Path::resetTimer()
		{
			++_timer_id;
			PLIter iter;
			if(!_manager.getMin(iter))
				return;
			setTimer(iter);
		}

		void Path::setTimer(const PLIter& iter)
		{
			PLIter n_iter = _manager.getNext(iter);
			if (n_iter == _manager.leftEnd() || n_iter == _manager.rightEnd())
				kingdomwar_sys.addTimer((*iter)->triggerTime(), boostBind(Path::arrive, this, _timer_id, iter));
			else if ((*iter)->nation() == (*n_iter)->nation())
				kingdomwar_sys.addTimer((*iter)->triggerTime(), boostBind(Path::meet, this, _timer_id, iter, n_iter));
			else
				kingdomwar_sys.addTimer((*iter)->triggerTime(), boostBind(Path::attack, this, _timer_id, iter, n_iter));
		}

		PlayerFighter::PlayerFighter(const mongo::BSONElement& obj)
		{
			_pid = obj["p"].Int();
			_army_id = obj["a"].Int();
			playerDataPtr d = player_mgr.getPlayer(_pid);
			_name = d->Name();
		}

		PlayerFighter::PlayerFighter(playerDataPtr d, int army_id)
		{
			_pid = d->ID();
			_name = d->Name();
			_army_id = army_id;
		}

		mongo::BSONObj PlayerFighter::toBSON() const
		{
			return BSON("t" << (int)TypePlayer << "p" << _pid << "a" << _army_id);
		}

		void PlayerFighter::getInfo(qValue& q) const
		{
			q.append(_name);
		}

		int PlayerFighter::type() const
		{
			return TypePlayer;
		}

		bool PlayerFighter::isDead() const
		{
			playerDataPtr d = player_mgr.getPlayer(_pid);	
			return d->KingDomWar->isDead(_army_id);
		}

		sBattlePtr PlayerFighter::getBattlePtr()
		{
			playerDataPtr d = player_mgr.getPlayer(_pid);
			return kingdomwar_sys.getBattlePtr(d, _army_id);
		}

		NpcFighter::NpcFighter(const mongo::BSONElement& obj)
		{
		}

		NpcFighter::NpcFighter(int id)
		{
		}

		mongo::BSONObj NpcFighter::toBSON() const
		{
			return BSON("t" << (int)TypeNpc);
		}

		void NpcFighter::getInfo(qValue& q) const
		{
		
		}

		int NpcFighter::type() const
		{
			return TypeNpc;
		}

		bool NpcFighter::isDead() const
		{
			if (_hp <= 0)
				return true;
			ForEachC(std::vector<int>, it, _man_hp)
			{
				if ((*it) > 0)
					return false;
			}
			return true;
		}

		sBattlePtr NpcFighter::getBattlePtr()
		{
			return sBattlePtr();
		}

		BattleField::BattleField(CityPtr& ptr)
			: _city(ptr)
		{
			_state = Closed;	
			_last_battle_time = 0;
			_next_battle_time.assign(QueueNum, 0);
			_queue_state.assign(QueueNum, Free);
			_attacker_queue.assign(QueueNum, FighterQueue());
			_defender_queue.assign(QueueNum, FighterQueue());
		}

		void BattleField::loadDB()
		{
			mongo::BSONObj key = BSON("ci" << _city->id());
			mongo::BSONObj obj = db_mgr.FindOne(DBN::dbKingdomWarCityBattle, key);
			if (obj.isEmpty())
				return;
			
			_state = obj["st"].Int();
			if (_state == Closed)
				return;
			_last_battle_time = obj["lbt"].Int();
			_first_battle_nation = obj["fbn"].Int();
			{
				std::vector<mongo::BSONElement> ele = obj["qst"].Array();
				for (unsigned i = 0; i < ele.size(); ++i)
					_queue_state[i] = ele[i].Int();
			}
			{
				std::vector<mongo::BSONElement> ele = obj["nbt"].Array();
				for (unsigned i = 0; i < ele.size(); ++i)
					_next_battle_time[i] = ele[i].Int();
			}
			{
				std::vector<mongo::BSONElement> ele = obj["aq"].Array();
				for (unsigned i = 0; i < ele.size(); ++i)
				{
					std::vector<mongo::BSONElement> ele2 = ele[i].Array();
					for (unsigned j = 0; j < ele2.size(); ++j)
						_attacker_queue[i].push_back(Creator<PlayerFighter>::Create(ele2[j]));
				}
			}
			{
				std::vector<mongo::BSONElement> ele = obj["dq"].Array();
				for (unsigned i = 0; i < ele.size(); ++i)
				{
					std::vector<mongo::BSONElement> ele2 = ele[i].Array();
					for (unsigned j = 0; j < ele2.size(); ++j)
					{
						int type = ele2[j]["t"].Int();
						if (type == TypeNpc)
							_defender_queue[i].push_back(Creator<NpcFighter>::Create(ele2[j]));
						else
							_defender_queue[i].push_back(Creator<PlayerFighter>::Create(ele2[j]));
					}
				}
			}
			{
				std::vector<mongo::BSONElement> ele = obj["ws"].Array();
				for (unsigned i = 0; i < Kingdom::nation_num; ++i)
					_wins[i] = ele[i].Int();
			}


			// set timer
			if (_state == Opened)
			{
				for (unsigned i = 0; i < QueueNum; ++i)
				{
					if (_queue_state[i] == Busy)
						setBattleTimer(_next_battle_time[i], i);
				}
			}
			if (_state == AttackerWait)
				setAttackerWaitTimer(_last_battle_time + AttackerWaitTime);
			if (_state == DefenderWait)
				setDefenderWaitTimer(_last_battle_time + DefenderWaitTime);
		}

		int BattleField::unfilledAttackerQueue()
		{
			int min = QueueMemNum;
			int id = -1;
			for (unsigned i = 0; i < QueueNum; ++i)
			{
				if (_queue_state[i] == Free)
					return i;
				if (_attacker_queue[i].size() < min)
				{
					min = _attacker_queue[i].size();
					id = i;
				}
			}
			return id;
		}

		int BattleField::unfilledDefenderQueue()
		{
			int min = QueueMemNum;
			int id = -1;
			for (unsigned i = 0; i < QueueNum; ++i)
			{
				if (_queue_state[i] == Free)
					continue;
				if (_queue_state[i] == Wait)
					return i;
				if (_defender_queue[i].size() < min)
				{
					min = _defender_queue[i].size();
					id = i;
				}
			}
			return id;
		}

		bool BattleField::tryGetNpcDefender(int queue_id)
		{
			FighterPtr ptr = _city->getNpcDefender();
			if (ptr)
			{
				_defender_queue[queue_id].push_back(ptr);
				return true;
			}
			return false;
		}
		
		void BattleField::tryGetAttacker()
		{
			while (true)
			{
				int queue_id = unfilledAttackerQueue();
				if (queue_id == -1)
					break;
				FighterPtr ptr = _city->getAttacker();	
				if (!ptr)
					break;
				_attacker_queue[queue_id].push_back(ptr);
			}
		}

		void BattleField::tryGetDefender()
		{
			while (true)
			{
				int queue_id = unfilledDefenderQueue();
				if (queue_id == -1)
					break;
				FighterPtr ptr = _city->getDefender();	
				if (!ptr)
					break;
				_defender_queue[queue_id].push_back(ptr);
			}
		}

		void BattleField::tick()
		{
			update();
		}

		void BattleField::handleAddAttacker(unsigned tick_time)
		{
			tryGetAttacker();
			tryGetDefender();

			if (_state == Closed)
			{
				_state = Opened;
				_first_battle_nation = boost::dynamic_pointer_cast<PlayerFighter>(_attacker_queue[0u].front())->nation();
			}
		
			if (_state == DefenderWait)
				_state = Opened;

			for (unsigned i = 0; i < QueueNum; ++i)
			{
				if (!_attacker_queue[i].empty() && _queue_state[i] == Free)
				{
					_queue_state[i] = Busy;
					setBattleTimer(tick_time + AttackInterval, i);
				}
			}
		}

		void BattleField::handleAddDefender(unsigned tick_time)
		{
			tryGetDefender();
			
			if (_state == AttackerWait)
				_state = Opened;
			
			for (unsigned i = 0; i < QueueNum; ++i)
			{
				if (!_defender_queue[i].empty() && _queue_state[i] == Wait)			
				{
					_queue_state[i] = Busy;
					tickBattle(tick_time, i);
				}
			}
		}

		void BattleField::handleAddNpc(unsigned tick_time)
		{
			if (_state == AttackerWait)
				_state = Opened;

			for (unsigned i = 0; i < QueueNum; ++i)
			{
				if (_queue_state[i] == Wait)
				{
					tryGetNpcDefender(i);
					if (!_defender_queue[i].empty())
					{
						_queue_state[i] = Busy;
						tickBattle(tick_time, i);
						return;
					}
				}
			}
		}

		void BattleField::setBattleTimer(unsigned tick_time, int id)
		{
			kingdomwar_sys.addTimer(tick_time, boostBind(BattleField::tickBattle, this, tick_time, id));
		}

		void BattleField::setAttackerWaitTimer(unsigned tick_time)
		{
			kingdomwar_sys.addTimer(tick_time, boostBind(BattleField::tickAttackerWait, this, tick_time));
		}

		void BattleField::setDefenderWaitTimer(unsigned tick_time)
		{
			kingdomwar_sys.addTimer(tick_time, boostBind(BattleField::tickDefenderWait, this, tick_time));
		}

		void BattleField::tickBattle(unsigned tick_time, int id)
		{
			if (_defender_queue[id].empty() &&
				!tryGetNpcDefender(id))
			{
				_queue_state[id] = Wait;	
				for (unsigned i = 0; i < QueueNum; ++i)
				{
					if (_queue_state[i] == Wait)
					{
						setAttackerWaitTimer(_last_battle_time + AttackerWaitTime);
						break;
					}
				}
				return;
			}
			
			FighterPtr atk_ptr = _attacker_queue[id].front();
			FighterPtr def_ptr = _defender_queue[id].front();
			sBattlePtr atk = atk_ptr->getBattlePtr();
			sBattlePtr def = def_ptr->getBattlePtr();
			O2ORes resultB = battle_sys.One2One(atk, def, typeBattle::kingdomwar);
			if (resultB.res == resBattle::atk_win)
				++_wins[atk->playerNation];
			_last_battle_time = tick_time;
			if (atk_ptr->isDead())
				_attacker_queue[id].pop_front();
			if (def_ptr->isDead())
				_defender_queue[id].pop_front();
			tryGetAttacker();
			tryGetDefender();
			
			if (_attacker_queue[id].empty())
			{
				_queue_state[id] = Free;
				releaseDefender(id, tick_time);
				if (noAttackers())
					setDefenderWaitTimer(_last_battle_time + DefenderWaitTime);
				return;
			}

			setBattleTimer(tick_time + AttackInterval, id);
		}

		int BattleField::getWinNation()
		{
			int win_nation = 0;
			int win_max = _wins[0];
			for (unsigned i = 1; i < Kingdom::nation_num; ++i)
			{
				if (_wins[i] > win_max)
				{
					win_max = _wins[i];
					win_nation = i;
				}
			}
			for (unsigned i = 0; i < Kingdom::nation_num; ++i)
			{
				if (i == win_nation)
					continue;
				if (_wins[i] == win_max)
					return _first_battle_nation;
			}
			return win_nation;
		}

		void BattleField::tickAttackerWait(unsigned tick_time)
		{
			if (_state == AttackerWait)
			{
				for (unsigned i = 0; i < QueueNum; ++i)
				{
					ForEach(FighterQueue, it, _attacker_queue[i])
						_city->releaseAttacker(*it, tick_time);
					_attacker_queue[i].clear();
				}
				int win_nation = getWinNation();
				_city->handleBattleResult(tick_time, true, win_nation);
				tickClose();
			}
		}

		void BattleField::tickDefenderWait(unsigned tick_time)
		{
			if (_state == DefenderWait)
			{
				_city->handleBattleResult(tick_time, false);
				tickClose();
			}
		}
		
		void BattleField::tickClose()
		{
			_state = Closed;
			for (unsigned i = 0; i < QueueNum; ++i)
				_queue_state[i] = Free;
			for (unsigned i = 0; i < Kingdom::nation_num; ++i)
				_wins[i] = 0;
		}

		void BattleField::releaseDefender(int id, unsigned tick_time)
		{
			ForEach(FighterQueue, it, _defender_queue[id])
				_city->releaseDefender(*it, tick_time);
			_defender_queue[id].clear();
		}

		bool BattleField::_auto_save()
		{
			mongo::BSONObj key = BSON("ci" << _city->id());
			mongo::BSONObjBuilder obj;
			obj << "ci" << _city->id() << "st" << _state;
			if (_state != Closed)
			{
				obj << "lbt" << _last_battle_time << "fbn" << _first_battle_nation;
				{
					mongo::BSONArrayBuilder b1, b2, b3, b4;
					for (unsigned i = 0; i < QueueNum; ++i)
					{
						b1.append(_queue_state[i]);
						b2.append(_next_battle_time[i]);
						mongo::BSONArrayBuilder b01;
						ForEach(FighterQueue, it, _attacker_queue[i])
							b01.append((*it)->toBSON());
						b3.append(b01.arr());
						mongo::BSONArrayBuilder b02;
						ForEach(FighterQueue, it, _defender_queue[i])
							b02.append((*it)->toBSON());
						b4.append(b02.arr());
					}
					obj << "qst" << b1.arr() << "nbt" << b2.arr()
						<< "aq" << b3.arr() << "dq" << b4.arr();
				}
				{
					mongo::BSONArrayBuilder b;
					for (unsigned i = 0; i < Kingdom::nation_num; ++i)
						b.append(_wins[i]);
					obj << "ws" << b.arr();
				}
			}
			return db_mgr.SaveMongo(DBN::dbKingdomWarCityBattle, key, obj.obj());
		}

		inline bool BattleField::noAttackers() const
		{
			for (unsigned i = 0; i < QueueNum; ++i)
			{
				if (!_attacker_queue[i].empty())
					return false;
			}
			return true;
		}

		void BattleField::update()
		{
			_main_info.toArray();
			for (unsigned i = 0; i < QueueNum; ++i)
			{
				qValue q;
				qValue atk_q;
				qValue def_q;
				ForEach(FighterQueue, it, _attacker_queue[i])
				{
					qValue tmp;
					(*it)->getInfo(tmp);
					atk_q.append(tmp);
				}
				ForEach(FighterQueue, it, _defender_queue[i])
				{
					qValue tmp;
					(*it)->getInfo(tmp);
					def_q.append(tmp);
				}
				q.append(atk_q);
				q.append(def_q);
				_main_info.append(q);
			}
		}

		City::City(const Json::Value& info)
		{
			_id = info["id"].asInt();

			if (_id == MainCity[Kingdom::wei])
				_nation = Kingdom::wei;
			else if (_id == MainCity[Kingdom::shu])
				_nation = Kingdom::shu;
			else if (_id == MainCity[Kingdom::wu])
				_nation = Kingdom::wu;
			else
				_nation = Kingdom::nation_num;

		}

		int City::state()
		{
			return _battle_field->state() == Closed? 0 : 1;
		}

		void City::loadDB()
		{
			CityPtr ptr = boost::dynamic_pointer_cast<City>(shared_from_this());
			_battle_field = Creator<BattleField>::Create(ptr);
			_battle_field->loadDB();

			mongo::BSONObj key = BSON("ci" << _id);
			mongo::BSONObj obj = db_mgr.FindOne(DBN::dbKingdomWarCityBase, key);
			if (obj.isEmpty())
				return;

			_nation = obj["nt"].Int();
			checkNotEoo(obj["nl"])
			{
				std::vector<mongo::BSONElement> ele = obj["nl"].Array();
				for (unsigned i = 0; i < ele.size(); ++i)
					_npc_list.push_back(Creator<NpcFighter>::Create(ele[i]));
			}
		}

		bool City::_auto_save()
		{
			mongo::BSONObj key = BSON("ci" << _id);
			mongo::BSONObjBuilder obj;
			obj << "ci" << _id << "nt" << _nation;
			if (!_npc_list.empty())
			{
				mongo::BSONArrayBuilder b;
				ForEach(NpcFighterList, it, _npc_list)
					b.append((*it)->toBSON());
				obj << "nl" << b.arr();
			}
			return db_mgr.SaveMongo(DBN::dbKingdomWarCityBase, key, obj.obj());
		}
		
		int City::enter(unsigned time, playerDataPtr d, int army_id)
		{
			if (_nation == d->Info->Nation())
			{
				_defender_backup.push_back(Creator<PlayerFighter>::Create(d, army_id));
				if (_battle_field->state() != Closed)
					noticeAddDefender(time);
				d->KingDomWar->setPosition(army_id, PosCity, _id, time);
			}
			else if (_battle_field->state() == Closed && _defender_backup.empty() && _npc_list.empty())
			{
				_defender_backup.push_back(Creator<PlayerFighter>::Create(d, army_id));
				_nation = d->Info->Nation();
				d->KingDomWar->setPosition(army_id, PosCity, _id, time);
			}
			else
			{
				_attacker_backup.push_back(Creator<PlayerFighter>::Create(d, army_id));
				noticeAddAttacker(time);
				d->KingDomWar->setPosition(army_id, PosSiege, _id, time);
			}
			return res_sucess;
		}

		int City::leave(unsigned time, playerDataPtr d, int army_id, int to_city_id)
		{
			PositionPtr pos = d->KingDomWar->getPosition(army_id);
			bool find = false;
			if (pos->_type == PosSiege)
			{
				ForEach(PlayerFighterList, it, _attacker_backup)
				{
					if ((*it)->pid() == d->ID() &&
						(*it)->armyId() == army_id)
					{
						_attacker_backup.erase(it);		
						find = true;
						break;
					}
				}
			}
			else
			{
				ForEach(PlayerFighterList, it, _defender_backup)
				{
					if ((*it)->pid() == d->ID() &&
						(*it)->armyId() == army_id)
					{
						_defender_backup.erase(it);
						find = true;
						break;
					}
				}
			}
			if (!find)
				return err_illedge;

			ForEach(std::vector<PathPtr>, it, _paths)
			{
				if ((*it)->access(_id, to_city_id))
					return (*it)->enter(time, d, army_id, to_city_id);
			}
			return err_illedge;
		}

		void City::noticeAddAttacker(unsigned tick_time)
		{
			_battle_field->handleAddAttacker(tick_time);
		}

		void City::noticeAddDefender(unsigned tick_time)
		{
			_battle_field->handleAddDefender(tick_time);
		}

		void City::noticeAddNpc(unsigned tick_time)
		{
			_battle_field->handleAddNpc(tick_time);
		}

		FighterPtr City::getAttacker()
		{
			if (_attacker_backup.empty())
				return FighterPtr();
			FighterPtr ptr = _attacker_backup.back();
			_attacker_backup.pop_back();
			return ptr;
		}

		FighterPtr City::getDefender()
		{
			if (_defender_backup.empty())
				return FighterPtr();
			FighterPtr ptr = _defender_backup.back();
			_defender_backup.pop_back();
			return ptr;
		}

		FighterPtr City::getNpcDefender()
		{
			if (_npc_list.empty())
				return FighterPtr();
			FighterPtr ptr = _npc_list.back();
			_npc_list.pop_back();
			return ptr;
		}

		void City::releaseDefender(FighterPtr& ptr, unsigned tick_time)
		{
			_defender_backup.push_back(boost::dynamic_pointer_cast<PlayerFighter>(ptr));
			noticeAddDefender(tick_time);
		}

		void City::releaseAttacker(FighterPtr& ptr, unsigned tick_time)
		{
			_attacker_backup.push_back(boost::dynamic_pointer_cast<PlayerFighter>(ptr));
		}

		void City::handleBattleResult(unsigned tick_time, bool result, int nation)
		{
			if (result)
			{
				_nation = nation;
				ForEach(PlayerFighterList, it, _attacker_backup)
				{
					playerDataPtr d = player_mgr.getPlayer((*it)->pid());
					if (d)
					{
						if (d->Info->Nation() == _nation)
						{
							_defender_backup.push_back(*it);
							d->KingDomWar->setPosition((*it)->armyId(), PosCity, _id, tick_time);
						}
						else
						{
							kingdomwar_sys.goBackMainCity(tick_time, d, (*it)->armyId());
						}
					}
				}
				_attacker_backup.clear();
			}
		}

		PathList::PathList()
		{
		}

		PathPtr PathList::getPath(int id)
		{
			PathMap::iterator it = _paths.find(id);
			return it == _paths.end()? PathPtr() : it->second;
		}

		void PathList::push(const PathPtr& ptr)
		{
			_paths.insert(make_pair(ptr->id(), ptr));
		}

		void PathList::update(bool is_first)
		{
			_main_info.toArray(); 
			_update_info.toArray();
			ForEach(PathMap, it, _paths)
			{
				qValue tmp;
				it->second->getUpdateInfo(tmp);
				if (!tmp.isEmpty())
					_update_info.append(tmp);
			}
			if (is_first || !_update_info.isEmpty())
			{
				ForEach(PathMap, it, _paths)
				{
					qValue tmp;
					it->second->getMainInfo(tmp);
					if (!tmp.isEmpty())
						_main_info.append(tmp);
				}
			}
		}

		CityList::CityList()
		{
		}

		CityPtr CityList::getCity(int id)
		{
			if (id < 1 || id > _citys.size())
				return CityPtr();
			return _citys[id - 1];
		}

		void CityList::push(const CityPtr& ptr)
		{
			while(ptr->id() > _citys.size())
				_citys.push_back(CityPtr());
			_citys[ptr->id() - 1] = ptr;
		}
		
		void CityList::update(bool is_first)
		{
			if (is_first)
			{
				LogI << "city size: " << _citys.size() << LogEnd;
				_main_state_info.toArray();
				_main_nation_info.toArray();
				for (unsigned i = 0; i < _citys.size(); ++i)
				{
					_states.push_back(_citys[i]->state());
					_nations.push_back(_citys[i]->nation());
					_main_state_info.append(_states[i]);
					_main_nation_info.append(_nations[i]);
				}
			}
			else
			{
				_update_state_info.toArray();
				_update_nation_info.toArray();
				for (unsigned i = 0; i < _citys.size(); ++i)
				{
					if (_states[i] != _citys[i]->state())
					{
						_states[i] = _citys[i]->state();
						qValue tmp;
						tmp.toArray();
						tmp.append(_citys[i]->id());
						tmp.append(_states[i]);
						_update_state_info.append(tmp);
					}
					if (_nations[i] != _citys[i]->nation())
					{
						_nations[i] = _citys[i]->nation();
						qValue tmp;
						tmp.toArray();
						tmp.append(_citys[i]->id());
						tmp.append(_nations[i]);
						_update_nation_info.append(tmp);
					}
				}
				if (!_update_state_info.isEmpty())
				{
					_main_state_info.toArray();
					for (unsigned i = 0; i < _citys.size(); ++i)
						_main_state_info.append(_states[i]);
				}
				if (!_update_nation_info.isEmpty())
				{
					_main_nation_info.toArray();
					for (unsigned i = 0; i < _citys.size(); ++i)
						_main_nation_info.append(_nations[i]);
				}
			}
		}
		
		void CityList::tick()
		{
			ForEach(Citys, it, _citys)
				(*it)->tick();
		}

		void CityList::loadDB()
		{
			ForEach(Citys, it, _citys)
				(*it)->loadDB();
		}
	}
}

