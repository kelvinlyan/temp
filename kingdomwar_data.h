#pragma once

#include "kingdomwar_def.h"
#include "kingdomwar_helper.h"
#include "battle_def.h"
#include "mongoDB.h"
#include "auto_base.h"

namespace gg
{
	namespace KingdomWar
	{
		enum
		{
			ServerError = 0x7FFFFFFF,

			// side
			Left = 0,
			Right,
			MaxSide,
		};

		const static int MainCity[] = {8, 21, 34};

		class PathItem
		{
			public:
				PathItem(unsigned time, int side, playerDataPtr d, int army_id);
				
				inline void getInfo(qValue& q) const;

				int pid() const { return _pid; }
				int nation() const { return _nation; }
				int armyId() const { return _army_id; }
				unsigned startTime() const { return _start_time; }
				int triggerTime() const { return _trigger_time; }
				int side() const { return _side; }

				void setTriggerTime(int time) { _trigger_time = time; }

			private:
				int _side;
				unsigned _start_time;
				int _pid;
				int _army_id;
				std::string _name;
				int _nation;
				int _trigger_time;
		};
		
		SHAREPTR(PathItem, PathItemPtr);
		STDLIST(PathItemPtr, PathItemList);
		typedef PathItemList::iterator PLIter;

		class PathItemSorter
		{
			public:
				bool getMin(PLIter& iter);
				inline void push(const PLIter& iter);
				void pop(const PLIter& iter);
				void clear() { _sort_list.clear(); }

			private:
				struct SortItem
				{
					SortItem(const PLIter& iter): _target(iter){}
					bool operator<(const SortItem& rhs) const
					{
						return (*_target)->triggerTime() < (*(rhs._target))->triggerTime();
					}
					bool operator==(const SortItem& rhs) const
					{
						return _target == rhs._target;
					}
					PLIter _target;
				};
				typedef multiset<SortItem> SortList;
				SortList _sort_list;
		};

		class UpdateItem
		{
			public:
				UpdateItem(int type, const PathItemPtr& ptr)
					: _type(type), _ptr(ptr){}

				int side() const { return _ptr->side(); }
				inline void getInfo(qValue& q) const;

			private:
				int _type;
				PathItemPtr _ptr;
		};


		class PathItemMgr
		{
			public:
				typedef boost::function<void(const PathItemPtr&)> Handler;

				PathItemMgr(const int& path_id);	
				void setPathType(int side, int path_type, int cost_time);
				bool access(int side) const { return _type[side] == 1; }

				void getMainInfo(qValue& q) const;
				void getUpdateInfo(qValue& q) const;

				bool empty() const;
				void push(const PathItemPtr& item);
				void pop(const PLIter& iter, int type);
				void swap(PLIter& lfs, PLIter& rhs);
				bool getMin(PLIter& iter);
				void clear();
				void run(const Handler& h);

				PLIter getNext(const PLIter& iter);
				const PLIter& leftEnd() const { return _left_end; }
				const PLIter& rightEnd() const { return _right_end; }

			private:
				PathItemList getInitedPathItemList();
				int getTriggerTime(const PLIter& lft, const PLIter& rhs);
				void setTriggerTime(PLIter& iter, int trigger_time);

			private:
				int _path_id;
				int _type[MaxSide];
				PathItemList _item_list;
				PathItemSorter _sorter;
				PLIter _left_end;
				PLIter _right_end;
				int _distance;
				int _speed[MaxSide];

				STDVECTOR(UpdateItem, UpdateItems);
				mutable UpdateItems _updates;
		};

		class City;
		SHAREPTR(City, CityPtr);

		class Path
		{
			public:
				Path(int path_id, int from_id, int to_id);

				int id() const { return _id; }

				void setPathType(int from_id, int to_id, int path_type, unsigned time);
				bool access(int from_id, int to_id) const;

				int enter(unsigned time, playerDataPtr d, int army_id, int to_city_id);
				int arrive(int timer_id, PLIter& iter);
				int meet(int timer_id, PLIter& lhs, PLIter& rhs);
				int attack(int timer_id, PLIter& lhs, PLIter& rhs);

				void getMainInfo(qValue& q) { _manager.getMainInfo(q); }
				void getUpdateInfo(qValue& q) { _manager.getUpdateInfo(q); }

			private:
				inline bool timerOverTime(int timer_id);
				void resetTimer();
				void setTimer(const PLIter& iter);

			private:
				int _id;
				CityPtr _linked_city[MaxSide];
				int _timer_id;
				PathItemMgr _manager;
		};

		SHAREPTR(Path, PathPtr);

		class Fighter
		{
			public:
				virtual mongo::BSONObj toBSON() const = 0;
				virtual void getInfo(qValue& q) const = 0;

				virtual int type() const = 0;
				virtual bool isDead() const = 0;
				virtual sBattlePtr getBattlePtr() = 0;
		};

		SHAREPTR(Fighter, FighterPtr);
		typedef std::deque<FighterPtr> FighterQueue;
		STDVECTOR(FighterPtr, FighterList);

		class PlayerFighter
			: public Fighter
		{
			public:
				PlayerFighter(const mongo::BSONElement& obj);
				PlayerFighter(playerDataPtr d, int army_id);
				
				virtual mongo::BSONObj toBSON() const;
				virtual void getInfo(qValue& q) const;

				virtual int type() const;
				virtual bool isDead() const;
				virtual sBattlePtr getBattlePtr();
				
				int nation() const { return _nation; }
				int pid() const { return _pid; }
				int armyId() const { return _army_id; }

			private:
				int _pid;
				std::string _name;
				int _army_id;
				int _nation;
		};

		SHAREPTR(PlayerFighter, PlayerFighterPtr);
		STDLIST(PlayerFighterPtr, PlayerFighterList);

		class NpcFighter
			: public Fighter
		{
			public:
				NpcFighter(const mongo::BSONElement& obj);
				NpcFighter(int id);

				virtual mongo::BSONObj toBSON() const;
				virtual void getInfo(qValue& q) const;

				virtual int type() const;
				virtual bool isDead() const;
				virtual sBattlePtr getBattlePtr();

			private:
				int _id;
				int _hp;
				std::vector<int> _man_hp;
		};

		SHAREPTR(NpcFighter, NpcFighterPtr);
		STDVECTOR(NpcFighterPtr, NpcFighterList);

		class BattleField
			: public _auto_meta, public KingdomWar::Updater
		{
			public:
				BattleField(CityPtr& ptr);

				void loadDB();

				int state() const { return _state; }

				void handleAddAttacker(unsigned tick_time);
				void handleAddDefender(unsigned tick_time);
				void handleAddNpc(unsigned tick_time);

				inline void getMainInfo(qValue& q) const { q = _main_info.Copy(); }
				void getUpdateInfo(qValue& q) const;

				void update();

				virtual void tick();

			private:
				virtual bool _auto_save();

				void tryGetAttacker();
				void tryGetDefender();
				bool tryGetNpcDefender(int queue_id);

				void setBattleTimer(unsigned tick_time, int queue_id);
				void setAttackerWaitTimer(unsigned tick_time);
				void setDefenderWaitTimer(unsigned tick_time);

				void tickBattle(unsigned tick_time, int queue_id);
				void tickAttackerWait(unsigned tick_time);
				void tickDefenderWait(unsigned tick_time);

				void releaseDefender(int queue_id, unsigned tick_time);

				int unfilledAttackerQueue();
				int unfilledDefenderQueue();
				inline bool noAttackers() const;

				int getWinNation();
				void tickClose();

			private:
				CityPtr _city;
				int _state;
				unsigned _last_battle_time;
				std::vector<int> _queue_state;
				std::vector<unsigned> _next_battle_time;
				std::vector<FighterQueue> _defender_queue;
				std::vector<FighterQueue> _attacker_queue;
				int _first_battle_nation;
				int _wins[3];

				mutable qValue _main_info;
		};

		SHAREPTR(BattleField, BattleFieldPtr);

		class City
			: public _auto_meta
		{
			public:
				friend class BattleField;

				City(const Json::Value& info);

				int id() const { return _id; }
				int state();

				void loadDB();
				void addPath(const PathPtr& ptr) { _paths.push_back(ptr); }
				int nation() const { return _nation; }

				int enter(unsigned time, playerDataPtr d, int army_id);
				int leave(unsigned time, playerDataPtr d, int army_id, int to_city_id);

				void attach(playerDataPtr d) { _battle_field->attach(d); }
				void detach(playerDataPtr d) { _battle_field->detach(d); }

				void tick() { _battle_field->tick(); }
			
			private:
				virtual bool _auto_save();
				
				void noticeAddAttacker(unsigned tick_time);
				void noticeAddDefender(unsigned tick_time);
				void noticeAddNpc(unsigned tick_time);
				void handleBattleResult(unsigned tick_time, bool result, int nation = -1);

				FighterPtr getAttacker();
				FighterPtr getDefender();
				FighterPtr getNpcDefender();

				void releaseAttacker(FighterPtr& ptr, unsigned tick_time);
				void releaseDefender(FighterPtr& ptr, unsigned tick_time);

			private:
				int _id;
				int _nation;
				std::vector<PathPtr> _paths;

				NpcFighterList _npc_list;
				PlayerFighterList _defender_backup;
				PlayerFighterList _attacker_backup;
				BattleFieldPtr _battle_field;
		};

		class PathList
		{
			public:
				PathList();

				PathPtr getPath(int id);
				void push(const PathPtr& ptr);
				void update(bool is_first = false);

				void getMainInfo(qValue& q) { q = _main_info.Copy(); }
				void getUpdateInfo(qValue& q) { q = _update_info; } 

			private:
				STDMAP(int, PathPtr, PathMap);
				PathMap _paths;

				qValue _main_info;
				qValue _update_info;
		};

		class CityList
		{
			public:
				CityList();

				void loadDB();

				CityPtr getCity(int id);
				void push(const CityPtr& ptr);
				void update(bool is_first = false);	
				void tick();

				void getStateInfo(qValue& q) { q = _main_state_info.Copy(); }
				void getUpStateInfo(qValue& q) { q = _update_state_info; } 
				void getNationInfo(qValue& q) { q = _main_nation_info.Copy(); }
				void getUpNationInfo(qValue& q) { q = _update_nation_info; }

			private:
				STDVECTOR(CityPtr, Citys);
				Citys _citys;

				std::vector<int> _states;
				std::vector<int> _nations;
 
				qValue _main_state_info;
				qValue _update_state_info;
				qValue _main_nation_info;
				qValue _update_nation_info;
		};
	}
}
