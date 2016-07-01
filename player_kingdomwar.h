#pragma once

#include "kingdomwar_def.h"
#include "kingdomwar_helper.h"
#include "auto_base.h"
#include "mongoDB.h"

namespace gg
{
	class playerMan;

	namespace KingdomWar
	{
		class Report
		{
			public:
				Report(int army_id, int city_id, int state, int target_nation, const std::string& target_name, unsigned exploit, const std::string& rep)
					: _army_id(army_id), _city_id(city_id), _state(state), _target_nation(target_nation), _target_name(target_name), _exploit(exploit), _rep_id(rep){}
				Report(const mongo::BSONElement& obj);

				mongo::BSONObj toBSON() const;
				void getInfo(qValue& q);

			private:
				int _army_id;
				int _city_id;
				int _state;
				int _target_nation;
				std::string _target_name;
				unsigned _exploit;
				std::string _rep_id;
		};

		SHAREPTR(Report, ReportPtr);
	}

	class playerKingdomWar
		: public _auto_player
	{
		public:
			playerKingdomWar(playerData* const own);

			bool inited() const;
			void initData();
			void loadDB();
			virtual bool _auto_save();
			virtual void _auto_update();

			void updateReport();
			int manHp(int man_id) const;
			void setManHp(int man_id, int hp);
			
			bool isDead(int army_id);
			int armyHp(int army_id);
			std::string addReport(int army_id, int city_id, int state, int target_nation, const std::string& target_name, unsigned exploit);
			int useHpItem(int army_id, int num);
			int buyHpItem(int num);

			int getExploit();
			int getTotalExploit() const { return _total_exploit; }
			int alterExploit(int num);
			int alterArmyHp(int army_id, int num);

		private:
			STDMAP(int, int, Man2HpMap);
			Man2HpMap _man_hp;
			STDVECTOR(int, ArmyHp);
			ArmyHp _army_hp;

			typedef std::deque<KingdomWar::ReportPtr> ReportList;
			ReportList _report_list;
			unsigned _report_id;

			int _exploit;
			int _total_exploit;
			unsigned _clear_time;
	};
}
