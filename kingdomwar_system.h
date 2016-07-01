#pragma once

#include "kingdomwar_data.h"
#include "dbDriver.h"
#include "timer2.h"

#define kingdomwar_sys (*gg::kingdomwar_system::_Instance)

namespace gg
{
	class mapDataConfig;

	class kingdomwar_system
		: public KingdomWar::Observer
	{
		public:
			static kingdomwar_system* const _Instance;

			typedef boost::function<void()> Func;
			STDVECTOR(Func, FuncList);

			SHAREPTR(mapDataConfig, MapDataCfgPtr);
			UNORDERMAP(int, MapDataCfgPtr, MapDataMap);

			kingdomwar_system();

			void initData();

			KingdomWar::CityPtr getCity(int id);
			KingdomWar::PathPtr getPath(int id);
			KingdomWar::CityPtr getMainCity(int nation);

			int citySize() const { return _city_list.size(); }
			unsigned next5MinTime() const { return _every_5_min->nextTime(); }

			DeclareRegFunction(quitReq);
			DeclareRegFunction(mainInfoReq);
			DeclareRegFunction(moveReq);
			DeclareRegFunction(playerInfoReq);
			DeclareRegFunction(formationReq);
			DeclareRegFunction(setFormationReq);
			DeclareRegFunction(cityBattleInfoReq);
			DeclareRegFunction(retreatReq);
			DeclareRegFunction(reportInfoReq);
			DeclareRegFunction(rankInfoReq);
			DeclareRegFunction(buyHpItemReq);
			DeclareRegFunction(useHpItemReq);
			DeclareRegFunction(outputInfoReq);
			DeclareRegFunction(getOutputReq);
			
			void mainInfo(playerDataPtr d);
			virtual void tick();

			void goBackMainCity(unsigned time, playerDataPtr d, int army_id);

			void addTimer(unsigned tick_time, const Timer2::TickFunc& func);
			void addUpdater(const Func& func);
			void add5MinTicker(const Func& func);
			void updateRank(playerDataPtr d, int old_value);

			int getCostTime(int from_id, int to_id);
			sBattlePtr getBattlePtr(playerDataPtr d, int army_id);
			sBattlePtr getNpcBattlePtr(int id);

			const KingdomWar::LineInfo& getLine(int from_id, int to_id) const;

			Json::Value& outputRet() { return _output_ret; }
			int randomNpcID() const;

		private:
			void loadFile();
			void loadDB();
			void startTimer();
			void timerTick();
			void outputTick();
			void addTimer();
			void update();
			void tickEvery5Min();
			void resetRandomRange();

		private:
			KingdomWar::CityList _city_list;
			KingdomWar::PathList _path_list;
			
			KingdomWar::ShortestPath _shortest_path;
			KingdomWar::RankMgr _rank_mgr;

			FuncList _updaters;
			FuncList _5_min_tickers;
			SHAREPTR(KingdomWar::Every5Min, Every5MinPtr);
			Every5MinPtr _every_5_min;
			MapDataMap _npc_map;
			std::vector<KingdomWar::NpcRule> _npc_rule;

			Json::Value _output_ret;
			int _rand_begin;
			int _rand_end;
	};
}
