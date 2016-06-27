#pragma once

#include "kingdomwar_data.h"
#include "dbDriver.h"

#define kingdomwar_sys (*gg::kingdomwar_system::_Instance)

namespace gg
{
	class kingdomwar_system
		: public KingdomWar::Updater
	{
		public:
			static kingdomwar_system* const _Instance;

			kingdomwar_system();

			void initData();

			KingdomWar::CityPtr getCity(int id);
			KingdomWar::PathPtr getPath(int id);
			KingdomWar::CityPtr getMainCity(int nation);

			DeclareRegFunction(quitReq);
			DeclareRegFunction(mainInfoReq);
			DeclareRegFunction(moveReq);
			DeclareRegFunction(formationReq);
			DeclareRegFunction(setFormationReq);
			DeclareRegFunction(playerInfoReq);
			
			void mainInfo(playerDataPtr d);
			virtual void tick();

			void goBackMainCity(unsigned time, playerDataPtr d, int army_id);

			void addTimer(unsigned tick_time, const KingdomWar::Timer::TickFunc& func);

			int getCostTime(int from_id, int to_id);
			sBattlePtr getBattlePtr(playerDataPtr d, int army_id);

		private:
			void loadFile();
			void loadDB();
			void startTimer();
			void timerTick();

		private:
			KingdomWar::CityList _city_list;
			KingdomWar::PathList _path_list;
			
			KingdomWar::ShortestPath _shortest_path;
			KingdomWar::Timer _timer;
	};
}
